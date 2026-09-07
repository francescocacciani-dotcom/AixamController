#include "config.h"
#include "state.h"

/* ============================================================
   Apertura/chiusura tramite centralina aftermarket: impulso sui
   2 relè, stato letto dai 2 sensori (ora digitali via optoisolatore,
   non più analogRead). Alla chiusura si spegne tutto (courtesy_stop).
   All'apertura (telecomando O prossimità) si comporta come
   l'apertura sportello: courtesy_start().
   ============================================================ */

static bool lastSenseLock = HIGH;
static bool lastSenseUnlock = HIGH;
static unsigned long pulseStart = 0;
static bool pulseActive = false;
static const unsigned long PULSE_MS = 500;

void centralLocking_setup() {
  pinMode(SENSE_LOCK, INPUT_PULLUP);
  pinMode(SENSE_UNLOCK, INPUT_PULLUP);
  pinMode(SENSE_EXTRA_CONTACT, INPUT_PULLUP);
  digitalWrite(RELAY_LOCK, LOW);
  digitalWrite(RELAY_UNLOCK, LOW);
}

// Hook vuoto: qui in futuro colleghi il comportamento che deciderai
// per il 3° contatto (es. lampeggio conferma, evento verso MQTT/HA,
// chiusura vetri...). Per ora logga soltanto.
void centralLocking_onExtraContact() {
  Serial.print(F("Contatto extra centralina attivato @"));
  Serial.println(millis());
  // TODO: azione futura
}

void centralLocking_pulse(bool lock) {
  if (pulseActive) return; // un impulso alla volta
  digitalWrite(lock ? RELAY_LOCK : RELAY_UNLOCK, HIGH);
  pulseStart = millis();
  pulseActive = true;
}

void centralLocking_update() {
  // Fine impulso relè, non bloccante
  if (pulseActive && millis() - pulseStart > PULSE_MS) {
    digitalWrite(RELAY_LOCK, LOW);
    digitalWrite(RELAY_UNLOCK, LOW);
    pulseActive = false;
  }

  bool senseLock = digitalRead(SENSE_LOCK);
  bool senseUnlock = digitalRead(SENSE_UNLOCK);

  if (senseUnlock == LOW && lastSenseUnlock == HIGH) {
    // La centralina ha eseguito uno sblocco (telecomando o noi)
    car.carLocked = false;
    log_write(LOG_INFO, "LOCK", "Auto sbloccata");
    courtesy_start();
  }
  if (senseLock == LOW && lastSenseLock == HIGH) {
    car.carLocked = true;
    log_write(LOG_INFO, "LOCK", "Auto chiusa");
    courtesy_stop();
  }
  lastSenseLock = senseLock;
  lastSenseUnlock = senseUnlock;

  // 3° contatto (dirty contact) — solo lettura/log per ora
  car.extraContactActive = (digitalRead(SENSE_EXTRA_CONTACT) == LOW);
  if (car.extraContactActive && !car.lastExtraContactActive) {
    car.extraContactLastMs = millis();
    centralLocking_onExtraContact();
  }
  car.lastExtraContactActive = car.extraContactActive;

  // Apertura per prossimità: se il modulo BLE segnala presenza
  // continua (isteresi già applicata in ble_link.ino) e l'auto è
  // ancora chiusa, sblocchiamo noi.
  if (car.proximityDetected && car.carLocked && !car.ignitionOn) {
    log_write(LOG_INFO, "LOCK", "Sblocco per prossimità BLE");
    centralLocking_pulse(false);
  }
}
