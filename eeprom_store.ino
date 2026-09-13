#include <Wire.h>
#include "config.h"
#include "state.h"

/* ============================================================
   Layout EEPROM (24LC-style, address map). Un indirizzo per
   ogni impostazione persistente: aggiungerne di nuove = aggiungere
   una riga qui, MAI riordinare quelle esistenti (rompe i profili
   già salvati).
   ============================================================ */
#define EE_LIGHTS_AUTO         0
#define EE_BAFFI_MANUAL        1
#define EE_CEILING_COLOR       2
#define EE_COURTESY_DURATION_S 3   // secondi, 1 byte (max 255s)
#define EE_FAN_AUTO             4
#define EE_TEMP_SETPOINT        5
#define EE_SUB_ON                6
#define EE_INVERTER_ON           7

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
  car.lightsAutoMode      = eepromRead(EE_LIGHTS_AUTO);
  car.baffiManualOverride = eepromRead(EE_BAFFI_MANUAL);
  car.ceilingColorIndex   = eepromRead(EE_CEILING_COLOR);
  byte cs = eepromRead(EE_COURTESY_DURATION_S);
  car.courtesyDurationMs  = (cs == 0 ? 30 : cs) * 1000UL;
  car.fanAutoMode         = eepromRead(EE_FAN_AUTO);
  car.tempSetpoint        = eepromRead(EE_TEMP_SETPOINT);
  car.subAutoMode         = eepromRead(EE_SUB_ON);
  car.inverterOn          = eepromRead(EE_INVERTER_ON);
}

void settings_saveCourtesyDuration() {
  byte s = (byte)constrain(car.courtesyDurationMs / 1000UL, 5, 255);
  eepromWrite(EE_COURTESY_DURATION_S, s);
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
