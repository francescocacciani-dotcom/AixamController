#include <TinyDHT.h>
#include "config.h"
#include "state.h"

DHT dhtExt(DHT_EXTERNAL, DHT11);
DHT dhtInt1(DHT_INTERNAL_1, DHT11);
DHT dhtInt2(DHT_INTERNAL_2, DHT11);

static unsigned long lastTempRead = 0;
static const unsigned long TEMP_READ_MS = 15000;

/* ============================================================
   Encoder letto via interrupt hardware, non a polling. Prima
   versione: interrupt solo su CLK (singolo fronte) + lettura di
   DT - soluzione ingenua, sensibile al rimbalzo meccanico dei
   contatti (bouncing): ogni scatto reale genera diverse
   transizioni spurie, che con un singolo fronte letto una volta
   sola diventano conteggi persi o doppi ("a scatti").

   Questa versione usa la decodifica quadrature completa: interrupt
   su ENTRAMBI i pin (CLK e DT, fronte CHANGE), stato a 4 bit
   (2 bit precedenti + 2 bit correnti) indicizzato in una tabella
   di Gray code. Le transizioni non valide (rimbalzo) producono 0
   nella tabella e vengono scartate automaticamente, senza bisogno
   di debounce software che rallenterebbe la lettura.

   La maggior parte degli encoder meccanici genera 4 transizioni
   valide per scatto ("detent"): encoderRaw accumula le transizioni
   grezze, encoderDelta (quello che legge climate_readEncoder())
   avanza di 1 solo ogni 4 transizioni nella stessa direzione.
   ============================================================ */
volatile int8_t encoderDelta = 0;   // in "scatti", consumato da climate_readEncoder()
volatile int8_t encoderRaw = 0;     // transizioni grezze all'interno di uno scatto
volatile uint8_t encoderState = 0;  // ultimi 2 bit A/B + 2 bit precedenti

// Tabella di transizione quadrature standard (Gray code a 2 bit)
static const int8_t QUAD_TABLE[16] = {
  0, -1,  1,  0,
  1,  0,  0, -1,
 -1,  0,  0,  1,
  0,  1, -1,  0
};

void encoderISR() {
  uint8_t a = digitalRead(ENCODER_CLK);
  uint8_t b = digitalRead(ENCODER_DT);
  uint8_t current = (a << 1) | b;
  encoderState = ((encoderState << 2) | current) & 0x0F;
  encoderRaw += QUAD_TABLE[encoderState];

  if (encoderRaw >= 4)      { encoderDelta++; encoderRaw = 0; }
  else if (encoderRaw <= -4) { encoderDelta--; encoderRaw = 0; }
}

void climate_setup() {
  dhtExt.begin();
  dhtInt1.begin();
  dhtInt2.begin();
  pinMode(SW_FAN_POS1, INPUT_PULLUP);
  pinMode(SW_FAN_POS2, INPUT_PULLUP);
  pinMode(SW_FAN_POS3, INPUT_PULLUP);
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_DT), encoderISR, CHANGE);
}

void climate_readTemps() {
  if (millis() - lastTempRead < TEMP_READ_MS) return;
  lastTempRead = millis();

  int tOut = dhtExt.readTemperature();
  int t1 = dhtInt1.readTemperature();
  int t2 = dhtInt2.readTemperature();
  car.tempOutside = tOut==-999 ? 0:tOut; // se il dht non legge il valore salvo 0;

  // media dei due sensori interni, con fallback se uno fallisce
  bool ok1 = (t1 != -999);
  bool ok2 = (t2 != -999);
  if (ok1 && ok2) car.tempInside = (t1 + t2) / 2;
  else if (ok1)   car.tempInside = t1;
  else if (ok2)   car.tempInside = t2;
  // se entrambi falliscono, tiene l'ultimo valore valido
}

void climate_readEncoder() {
  noInterrupts();
  int8_t delta = encoderDelta;
  encoderDelta = 0;
  interrupts();

  if (delta == 0) return;

  car.tempSetpoint = constrain(car.tempSetpoint + delta, 10, 30);
}

void climate_update() {
  climate_readTemps();
  climate_readEncoder();

  int manualSpeed = 0;
  if (digitalRead(SW_FAN_POS1) == LOW) manualSpeed = 2;
  else if (digitalRead(SW_FAN_POS2) == LOW) manualSpeed = 1;
  else if (digitalRead(SW_FAN_POS3) == LOW) manualSpeed = 3;

  if (car.fanAutoMode) {
    if (car.tempInside > car.tempSetpoint) {
      car.fanSpeed = manualSpeed; // la velocità resta scelta dal selettore fisico,
                                   // l'automatismo decide SE farla girare
    } else {
      car.fanSpeed = 0;
    }
  } else {
    car.fanSpeed = manualSpeed;
  }

  digitalWrite(RELAY_FAN1, car.fanSpeed == 1 ? LOW : HIGH);
  digitalWrite(RELAY_FAN2, car.fanSpeed == 2 ? LOW : HIGH);
  digitalWrite(RELAY_FAN3, car.fanSpeed == 3 ? LOW : HIGH);
}
