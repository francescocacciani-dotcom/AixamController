#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "state.h"

Adafruit_NeoPixel ceilStrip(NEOPIXEL_CEIL_COUNT, NEOPIXEL_CEIL_PIN, NEO_GRB + NEO_KHZ800);

static bool lastBtn = HIGH;

// Palette selezionabile da Nextion (indice 0..5), stessa idea
// dell'originale ma isolata in un modulo dedicato.
void ceilingLight_setColorIndex(uint8_t idx) {
  car.ceilingColorIndex = idx % 6;
  switch (car.ceilingColorIndex) {
    case 0: car.r = 255; car.g = 255; car.b = 255; break; // bianco
    case 1: car.r = 255; car.g = 0;   car.b = 0;   break; // rosso
    case 2: car.r = 0;   car.g = 255; car.b = 0;   break; // verde
    case 3: car.r = 0;   car.g = 0;   car.b = 255; break; // blu
    case 4: car.r = 194; car.g = 0;   car.b = 252; break; // viola
    case 5: car.r = 0;   car.g = 251; car.b = 255; break; // azzurro
  }
  ceilingLight_render();
}

void ceilingLight_toggle() {
  car.ceilingLightOn = !car.ceilingLightOn;
  ceilingLight_render();
}

void ceilingLight_render() {
  for (int i = 0; i < NEOPIXEL_CEIL_COUNT; i++) {
    if (car.ceilingLightOn) {
      ceilStrip.setPixelColor(i, car.r, car.g, car.b);
    } else {
      ceilStrip.setPixelColor(i, 0, 0, 0);
    }
  }
  ceilStrip.setBrightness(255);
  ceilStrip.show();
}

void ceilingLight_setup() {
  ceilStrip.begin();
  ceilStrip.setBrightness(255);
  ceilStrip.show();
  pinMode(BTN_CEILING_LIGHT, INPUT_PULLUP);
}

void ceilingLight_update() {
  bool btn = digitalRead(BTN_CEILING_LIGHT);
  if (btn == LOW && lastBtn == HIGH) {
    ceilingLight_toggle();
  }
  lastBtn = btn;
}
