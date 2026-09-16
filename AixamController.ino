#include <Wire.h>
#include <Nextion.h>
#include "config.h"
#include "state.h"

CarState car;

// Dichiarato/definito in nextion_ui.ino, ma essendo un array globale
// (non una funzione) l'auto-generazione dei prototipi di Arduino non
// lo vede: va dichiarato extern esplicitamente qui, prima dell'uso in loop().
extern NexTouch *nex_listen_list[];

static bool lastBtnBaffi = HIGH;
static bool lastBtnLock = HIGH;
static bool lastBatteryOk = false; // usato per avviare il countdown del blocco per batteria bassa
static unsigned long lastVoltageRead = 0;
static unsigned long lastVoltageReadUndervoltage = 0; // usato per controllare la batteria bassa
static unsigned long lastPeriodicTasks = 0;
static const unsigned long PERIODIC_MS = 500;

void setup() {
  Serial.begin(9600);   // debug USB
  Serial2.begin(9600);  // Nextion
  Serial3.begin(9600);  // link ESP32/BLE presenza

  Wire.begin();

  // Relè: tutti OFF (attivo LOW) all'avvio
  int relayPins[] = {RELAY_SUB, RELAY_INVERTER, RELAY_LED, RELAY_MIRROR,
                      RELAY_FAN1, RELAY_FAN2, RELAY_FAN3, RELAY_FRONT,
                      RELAY_LOCK, RELAY_UNLOCK};
  for (int p : relayPins) { pinMode(p, OUTPUT); digitalWrite(p, HIGH); }

  pinMode(PWM_AUX, OUTPUT);
  pinMode(PWM_OCCHI, OUTPUT);
  pinMode(PWM_DOOR_LED, OUTPUT);

  pinMode(LED_MODE_AUTO, OUTPUT);
  pinMode(LED_MODE_MANUAL, OUTPUT);
  
  pinMode(TRANSISTOR_UTILITIES, OUTPUT);
  
  pinMode(BTN_BAFFI_MANUAL, INPUT_PULLUP);
  pinMode(BTN_CAR_LOCK_TOGGLE, INPUT_PULLUP);
  pinMode(SW_LIGHTS_AUTO_MANUAL, INPUT_PULLUP);
  pinMode(PIN_ENGINE_RUNNING, INPUT);
  
  pinMode(PIN_DOOR_SENSE, INPUT_PULLUP);

  settings_load();
  power_setup();
  log_setup();
  centralLocking_setup();
  climate_setup();
  ceilingLight_setup();
  dashboard_setup();
  lcdSecondary_setup();
  nextionUI_setup();

  log_write(LOG_INFO, "BOOT", car.sdAvailable ? "SD pronta" : "SD assente, solo log seriale");
  Serial.println(F("Aixam controller v2 pronto"));
}

void loop() {
  car.ignitionOn = (digitalRead(PIN_IGNITION) == HIGH); // TODO: verificare polarità reale dopo optoisolatore
  car.engineRunning = digitalRead(PIN_ENGINE_RUNNING);
  //car.batteryCharging = digitalRead(PIN_BATTERY_CHARGE);

  handleIgnitionEdge();
  handleDoorSense();
  handleBaffiButton();
  handleLockButton();
  bleLink_update();
  centralLocking_update();
  courtesy_update();

  if (car.ignitionOn) {
    lightsAuto_update();
    climate_update();
    dashboard_update();
    lcdSecondary_update();
    nexLoop(nex_listen_list);
    settings_update();
    handle_runningEngine();
  }

  ceilingLight_update(); // disponibile anche a chiave spenta

  if (millis() - lastPeriodicTasks > PERIODIC_MS) {
    lastPeriodicTasks = millis();
    applyOutputs();
    if (car.ignitionOn) nextionUI_sendUpdate();
    bleLink_sendStatus();
    log_flushBuffer();
    readBatteryVoltage();
  }

  power_enterSleepIfIdle();
}

void handleIgnitionEdge() {
  if (car.ignitionOn && !car.lastIgnitionOn) {
    log_write(LOG_INFO, "IGN", "Chiave ON");
    //lcdSecondary_wakeup();
    car.fanSpeed = 0;
    car.powerAccessories = true;
    car.powerFrontCar = true;
    if(car.subAutoMode){ car.subOn=true; }
    if(car.occhiAutoOn == 1){ car.occhiOn = true; }
  } else if (!car.ignitionOn && car.lastIgnitionOn) {
    log_write(LOG_INFO, "IGN", "Chiave OFF - spengo tutti gli accessori");
    lcdSecondary_sleep();
    lightsAuto_sleep();

    car.powerAccessories = false;
    car.powerFrontCar = false;

    car.fanSpeed = 0;
    car.baffiOn = false;
    car.occhiOn = false;
    car.subOn = false;
    car.inverterOn = false;
    car.mirrorBrightness = 0;
    car.frontFanOn = false;
  }
  car.lastIgnitionOn = car.ignitionOn;
}

void handle_runningEngine() {
  if (car.engineRunning && !car.lastEngineRunning) {
    log_write(LOG_INFO, "ENG", "Motore acceso");
    if(car.occhiAutoOn == 2){ car.occhiOn = true; }
  } else if (!car.engineRunning && car.lastEngineRunning) {
    log_write(LOG_INFO, "ENG", "Motore spento");
  }
  car.lastEngineRunning = car.engineRunning;
}

static unsigned long doorLastChangeMs = 0;
static bool doorRawLast = HIGH;
static const unsigned long DOOR_DEBOUNCE_MS = 50;

void handleDoorSense() {
  bool raw = digitalRead(PIN_DOOR_SENSE); // LOW = porta aperta (pull-up, contatto a massa)

  if (raw != doorRawLast) {
    doorLastChangeMs = millis();
    doorRawLast = raw;
  }

  // Il debounce si limita a "aspetta che il segnale sia stabile per
  // DOOR_DEBOUNCE_MS prima di crederci" - non blocca il loop, si limita
  // a non aggiornare car.doorOpen finché il contatto sta ancora rimbalzando.
  if (millis() - doorLastChangeMs < DOOR_DEBOUNCE_MS) return;

  car.doorOpen = (raw == LOW);

  if (car.doorOpen && !car.lastDoorOpen) {
    log_write(LOG_INFO, "DOOR", "Portiera aperta");
    /*if (!car.ignitionOn)*/ 
    if(car.doorCurtesyEnabled)courtesy_start();
  } else if (!car.doorOpen && car.lastDoorOpen) {
    log_write(LOG_INFO, "DOOR", "Portiera chiusa");
    courtesy_stop();
  }
  car.lastDoorOpen = car.doorOpen;
}

void handleBaffiButton() { // tasto multifunzione
  bool btn = digitalRead(BTN_BAFFI_MANUAL);
  if (btn == LOW && lastBtnBaffi == HIGH) {
    switch(car.reprogrammablePush){
      case 0:
        car.subOn = !car.subOn;
        break;
      case 1:
        car.ledOn = !car.ledOn;
        break;
      case 2:
        car.occhiOn = !car.occhiOn;
    }
  }
  lastBtnBaffi = btn;
}

void handleLockButton() {
  bool btn = digitalRead(BTN_CAR_LOCK_TOGGLE);
  if (btn == LOW && lastBtnLock == HIGH) {
    centralLocking_pulse(!car.carLocked);
  }
  lastBtnLock = btn;
}

void readBatteryVoltage() {
  if (millis() - lastVoltageRead < 2000) return;
  lastVoltageRead = millis();
  int raw = analogRead(PIN_BATTERY_VOLTAGE);
  car.batteryVoltage = map(raw, 0, 1023, 0, 15500) / 1000.0 + car.voltageSensitivity * 0.1; // TODO: verificare rapporto partitore reale
  if(car.batteryVoltage<car.batteryVoltageProtection){// procedura di blocco se batteria scarica
    if(lastBatteryOk){
      log_write(LOG_WARN, "BATT", "Picco bassa: " + String(car.batteryVoltage, 2) + "V");
      lastVoltageReadUndervoltage=millis();
      lastBatteryOk = false;
    }
    if(millis() - lastVoltageReadUndervoltage > 15000){
      lastVoltageReadUndervoltage = millis();
      while(car.batteryVoltage<car.batteryVoltageProtection && !car.voltageOverride){
        car.batteryVoltage = map(analogRead(PIN_BATTERY_VOLTAGE), 0, 1023, 0, 15500) / 1000.0 + car.voltageSensitivity * 0.1;
        car.ignitionOn = (digitalRead(PIN_IGNITION) == HIGH);
        if(car.ignitionOn){
          nexLoop(nex_listen_list);
          nextionUI_sendUpdate();
          delay(1000);
        }else{
          delay(10000); //TODO deepsleep di arduino da risvegliare sotto interrupt chiave
        }
        car.batteryLow=true;
        log_write(LOG_WARN, "BATT", "Tensione bassa: " + String(car.batteryVoltage, 2) + "V");
      }
    }
  }else{
    lastBatteryOk = true;
    car.voltageOverride = false;
    car.batteryLow = false;
  }
  /*
  bool wasLow = car.batteryLow;
  car.batteryLow = car.batteryVoltage < 11.6 && !car.voltageOverride;
  if (car.batteryLow && !wasLow) {
    log_write(LOG_WARN, "BATT", "Tensione bassa: " + String(car.batteryVoltage, 2) + "V");
  }*/
}

void applyOutputs() {
  digitalWrite(TRANSISTOR_UTILITIES, car.powerAccessories ? LOW : HIGH);
  digitalWrite(RELAY_FRONT, car.powerFrontCar ? LOW : HIGH);
  if(car.lightsAutoMode && car.engineRunning && car.ignitionOn){// TODO aggiungere cortesia agli anabbaglianti quando si accendono
    digitalWrite(RELAY_ANABBAGLIANTI, car.lowBeamOn ? LOW : HIGH);
    digitalWrite(RELAY_POSIZIONI, car.positionLightsOn ? LOW : HIGH);
  }else{
    digitalWrite(RELAY_ANABBAGLIANTI, HIGH);
    digitalWrite(RELAY_POSIZIONI, HIGH);
  }

  digitalWrite(RELAY_SUB, car.subOn ? LOW : HIGH);
  digitalWrite(RELAY_INVERTER, car.inverterOn ? LOW : HIGH);
  digitalWrite(RELAY_MIRROR, car.mirrorBrightness > 0 ? LOW : HIGH);


  if (!car.courtesySequenceActive) {
    analogWrite(PWM_OCCHI, car.occhiOn?255:0);
  }

  // TODO aggiungere controllo occhi
  digitalWrite(RELAY_LED, car.ledOn ? LOW:HIGH); // controllo led interni
  analogWrite(PWM_AUX, car.frontFanOn ? map(car.frontFanSpeed, 0, 100, 0, 255) : 0);
}
