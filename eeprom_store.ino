#include <Wire.h>
#include "config.h"
#include "state.h"

/* ============================================================
   Layout EEPROM (24LC-style, address map). Un indirizzo per
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
static unsigned long sensPositionChangedAt;
static unsigned long sensLowBeamChangedAt;
static bool sensPositionSavePending = false;
static bool sensLowBeamSavePending = false;
static bool settingsPersistenceInitialized = false;

void eepromWrite(byte addr, byte value) {
  Wire.beginTransmission(EEPROM_I2C_ADDR);
  Wire.write(addr);
  Wire.write(value);
  Wire.endTransmission();
  delay(5); // tempo di scrittura pagina, da datasheet
}

byte eepromRead(byte addr) {
  Wire.beginTransmission(EEPROM_I2C_ADDR);
  Wire.write(addr);
  Wire.endTransmission();
  delay(5);
  Wire.requestFrom((int)EEPROM_I2C_ADDR, 1);
  if (Wire.available()) return Wire.read();
  return 0;
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

void settings_saveCeilingColor() {
  eepromWrite(EE_CEILING_COLOR, car.ceilingColorIndex);
}

void settings_saveLightsAuto() {
  eepromWrite(EE_LIGHTS_AUTO, car.lightsAutoMode);
}

void settings_saveFanAuto() {
  eepromWrite(EE_FAN_AUTO, car.fanAutoMode);
}

void settings_saveTempSetpoint() {
  eepromWrite(EE_TEMP_SETPOINT, (byte)car.tempSetpoint);
}
