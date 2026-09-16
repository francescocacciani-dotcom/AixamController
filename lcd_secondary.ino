#include <LiquidCrystal_I2C.h>
#include "config.h"
#include "state.h"

LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);

static bool lastBtnLcd = HIGH;
static const uint8_t NUM_PAGES = 5;
static unsigned long lastRender = 0;

void lcdSecondary_setup() {
  lcd.init();
  lcd.backlight();
  pinMode(BTN_LCD_MODE, INPUT_PULLUP);
  lcd.noBacklight();
}

void lcdSecondary_nextPage() {
  car.lcdPage = (car.lcdPage + 1) % NUM_PAGES;
  lcd.clear();
}


/**
* @brief Clears the display and turns off the backlight.
*/
void lcdSecondary_sleep(){
  car.lcdBacklightOn = false;
  lcd.clear();
  lcd.noBacklight();
}

/**
* @brief Turns on the backlight.
*/
void lcdSecondary_wakeup(){
  car.lcdBacklightOn=true;
  lcd.backlight();
}

void lcdSecondary_update() {
  bool btn = digitalRead(BTN_LCD_MODE);
  if (btn == LOW && lastBtnLcd == HIGH) {
    lcdSecondary_nextPage();
  }
  lastBtnLcd = btn;

  if (millis() - lastRender < 500) return; // no bisogno di refresh continuo
  lastRender = millis();

  if(car.lcdPage==0&&car.lcdBacklightOn)lcdSecondary_sleep();
  else if(!car.lcdBacklightOn&&car.lcdPage!=0)lcdSecondary_wakeup();

  switch (car.lcdPage) {
    // case 0 gestito fuori
    case 1: // temperature interne
      lcd.setCursor(0, 0); lcd.print("Int: "); lcd.print(car.tempInside); lcd.print("C   ");
      lcd.setCursor(0, 1); lcd.print("Set: "); lcd.print(car.tempSetpoint); lcd.print("C   ");
      break;
    case 2: // temperature esterne
      lcd.setCursor(0, 0); lcd.print("Est: "); lcd.print(car.tempOutside); lcd.print("C   ");
      break;
    case 3: // batteria
      lcd.setCursor(0, 0); lcd.print("Batt: "); lcd.print(car.batteryVoltage, 2); lcd.print("V ");
      //lcd.setCursor(0, 1); lcd.print(car.batteryCharging ? "In carica    " : "Non in carica");
      break;
    case 4: // luci
      lcd.setCursor(0, 0); lcd.print("Luci: "); lcd.print(car.lightsAutoMode ? "AUTO" : "MAN ");
      lcd.setCursor(12, 0); lcd.print(car.ambientLight);
      lcd.setCursor(0, 1); lcd.print("Pos:"); lcd.print(car.positionLightsOn ? "ON " : "OFF");
      lcd.print(" Anab:"); lcd.print(car.lowBeamOn ? "ON " : "OFF");
      break;
  }
}
