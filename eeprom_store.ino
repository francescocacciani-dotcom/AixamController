#include <EEPROM.h>
#include "config.h"
#include "state.h"

/* ============================================================
    Layout EEPROM interna, address map. Un indirizzo per
   ogni impostazione persistente: aggiungerne di nuove = aggiungere
   una riga qui, MAI riordinare quelle esistenti (rompe i profili
   già salvati).
   ============================================================ */
#define EE_REPROGRAMMABLE_PUSH 0
#define EE_SENSE_POSITION      1
#define EE_SENSE_LOWBEAM       2
#define EE_COURTESY_DURATION_S 3   // secondi, 1 byte (max 255s)
#define EE_FAN_AUTO            4
#define EE_TEMP_SETPOINT       5
#define EE_SUB_ON              6
#define EE_OCCHI_AUTO_ON       7

static const unsigned long SETTINGS_SAVE_DELAY_MS = 15000;
static int lastSensPosition;
static int lastSensLowBeam;
static unsigned long lastCourtesyDurationMs;
static uint8_t lastReprogrammablePush;
static bool lastSubAutoMode;
static uint8_t lastOcchiAutoOn;
static bool lastFanAutoMode;
static int lastTempSetpoint;
static unsigned long sensPositionChangedAt;
static unsigned long sensLowBeamChangedAt;
static unsigned long courtesyDurationChangedAt;
static unsigned long reprogrammablePushChangedAt;
static unsigned long subAutoModeChangedAt;
static unsigned long occhiAutoOnChangedAt;
static unsigned long fanAutoModeChangedAt;
static unsigned long tempSetpointChangedAt;
static bool sensPositionSavePending = false;
static bool sensLowBeamSavePending = false;
static bool courtesyDurationSavePending = false;
static bool reprogrammablePushSavePending = false;
static bool subAutoModeSavePending = false;
static bool occhiAutoOnSavePending = false;
static bool fanAutoModeSavePending = false;
static bool tempSetpointSavePending = false;
static bool settingsPersistenceInitialized = false;

void eepromWrite(byte addr, byte value) {
  EEPROM.update(addr, value);
}

byte eepromRead(byte addr) {
  return EEPROM.read(addr);
}

void settings_load() {
  car.reprogrammablePush  = eepromRead(EE_REPROGRAMMABLE_PUSH);
  car.occhiAutoOn         = eepromRead(EE_OCCHI_AUTO_ON);
  car.sensPosition        = eepromRead(EE_SENSE_POSITION) * 4; // 0..255 -> 0..1020
  car.sensLowBeam         = eepromRead(EE_SENSE_LOWBEAM) * 4;   // 0..255 -> 0..1020
  byte cs = eepromRead(EE_COURTESY_DURATION_S);
  car.courtesyDurationMs  = (cs == 0 ? 30 : cs) * 1000UL;
  car.fanAutoMode         = eepromRead(EE_FAN_AUTO);
  car.tempSetpoint        = eepromRead(EE_TEMP_SETPOINT);
  car.subAutoMode         = eepromRead(EE_SUB_ON);
}

void settings_update() {
  unsigned long now = millis();

  if (!settingsPersistenceInitialized) {
    lastSensPosition = car.sensPosition;
    lastSensLowBeam = car.sensLowBeam;
    lastCourtesyDurationMs = car.courtesyDurationMs;
    lastReprogrammablePush = car.reprogrammablePush;
    lastSubAutoMode = car.subAutoMode;
    lastOcchiAutoOn = car.occhiAutoOn;
    lastFanAutoMode = car.fanAutoMode;
    lastTempSetpoint = car.tempSetpoint;
    settingsPersistenceInitialized = true;
    return;
  }

  if (car.sensPosition != lastSensPosition) {
    lastSensPosition = car.sensPosition;
    sensPositionChangedAt = now;
    sensPositionSavePending = true;
  } else if (sensPositionSavePending &&
             now - sensPositionChangedAt >= SETTINGS_SAVE_DELAY_MS) {
    settings_savePositionSense();
    sensPositionSavePending = false;
  }

  if (car.sensLowBeam != lastSensLowBeam) {
    lastSensLowBeam = car.sensLowBeam;
    sensLowBeamChangedAt = now;
    sensLowBeamSavePending = true;
  } else if (sensLowBeamSavePending &&
             now - sensLowBeamChangedAt >= SETTINGS_SAVE_DELAY_MS) {
    settings_saveLowBeamSense();
    sensLowBeamSavePending = false;
  }

  if (car.courtesyDurationMs != lastCourtesyDurationMs) {
    lastCourtesyDurationMs = car.courtesyDurationMs;
    courtesyDurationChangedAt = now;
    courtesyDurationSavePending = true;
  } else if (courtesyDurationSavePending &&
             now - courtesyDurationChangedAt >= SETTINGS_SAVE_DELAY_MS) {
    settings_saveCourtesyDuration();
    courtesyDurationSavePending = false;
  }

  if (car.reprogrammablePush != lastReprogrammablePush) {
    lastReprogrammablePush = car.reprogrammablePush;
    reprogrammablePushChangedAt = now;
    reprogrammablePushSavePending = true;
  } else if (reprogrammablePushSavePending &&
             now - reprogrammablePushChangedAt >= SETTINGS_SAVE_DELAY_MS) {
    settings_save_reprogrammablePush();
    reprogrammablePushSavePending = false;
  }

  if (car.subAutoMode != lastSubAutoMode) {
    lastSubAutoMode = car.subAutoMode;
    subAutoModeChangedAt = now;
    subAutoModeSavePending = true;
  } else if (subAutoModeSavePending &&
             now - subAutoModeChangedAt >= SETTINGS_SAVE_DELAY_MS) {
    settings_saveSubAuto();
    subAutoModeSavePending = false;
  }

  if (car.occhiAutoOn != lastOcchiAutoOn) {
    lastOcchiAutoOn = car.occhiAutoOn;
    occhiAutoOnChangedAt = now;
    occhiAutoOnSavePending = true;
  } else if (occhiAutoOnSavePending &&
             now - occhiAutoOnChangedAt >= SETTINGS_SAVE_DELAY_MS) {
    settings_saveOcchiAuto();
    occhiAutoOnSavePending = false;
  }

  if (car.fanAutoMode != lastFanAutoMode) {
    lastFanAutoMode = car.fanAutoMode;
    fanAutoModeChangedAt = now;
    fanAutoModeSavePending = true;
  } else if (fanAutoModeSavePending &&
             now - fanAutoModeChangedAt >= SETTINGS_SAVE_DELAY_MS) {
    settings_saveFanAuto();
    fanAutoModeSavePending = false;
  }

  if (car.tempSetpoint != lastTempSetpoint) {
    lastTempSetpoint = car.tempSetpoint;
    tempSetpointChangedAt = now;
    tempSetpointSavePending = true;
  } else if (tempSetpointSavePending &&
             now - tempSetpointChangedAt >= SETTINGS_SAVE_DELAY_MS) {
    settings_saveTempSetpoint();
    tempSetpointSavePending = false;
  }
}

void settings_saveCourtesyDuration() {
  byte s = (byte)constrain(car.courtesyDurationMs / 1000UL, 5, 255);
  eepromWrite(EE_COURTESY_DURATION_S, s);
}

void settings_save_reprogrammablePush(){
  eepromWrite(EE_REPROGRAMMABLE_PUSH, car.reprogrammablePush);
}

void settings_savePositionSense(){
  eepromWrite(EE_SENSE_POSITION, car.sensPosition/4); // 0..1023 -> 0..255
}

void settings_saveLowBeamSense(){
  eepromWrite(EE_SENSE_LOWBEAM, car.sensLowBeam/4); // 0..1023 -> 0..255
}

void settings_saveSubAuto() {
  eepromWrite(EE_SUB_ON, car.subAutoMode);
}

void settings_saveOcchiAuto() {
  eepromWrite(EE_OCCHI_AUTO_ON, car.occhiAutoOn);
}

void settings_saveFanAuto() {
  eepromWrite(EE_FAN_AUTO, car.fanAutoMode);
}

void settings_saveTempSetpoint() {
  eepromWrite(EE_TEMP_SETPOINT, (byte)car.tempSetpoint);
}
