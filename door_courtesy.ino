#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "state.h"

extern Adafruit_NeoPixel dashStrip;

/* ============================================================
   Sequenza di cortesia: luci interne (mosfet), posizione+baffi
   (relè), attivata da:
     - apertura sportello (a chiave spenta)
     - sblocco auto da telecomando/prossimità (stesso comportamento
       richiesto: "quando si apre deve avere lo stesso comportamento
       dell'avvicinamento")
   Si spegne dopo car.courtesyDurationMs oppure quando la porta si
   richiude, quello che avviene per primo, oppure subito se l'auto
   viene richiusa da telecomando.
   Fade software su PWM per un effetto meno brusco (come nell'
   originale) ma non bloccante: niente delay(), avanza a ogni loop().
   ============================================================ */

static uint8_t fadeLevel = 0;
static int8_t fadeDirection = 0; // +1 = accensione, -1 = spegnimento
static unsigned long lastFadeStep = 0;
static const unsigned long FADE_STEP_MS = 8;

void courtesy_start() {
  if (car.courtesySequenceActive && fadeDirection >= 0) return;
  car.courtesySequenceActive = true;
  car.courtesyStartMs = millis();
  fadeDirection = 1;
}

void courtesy_stop() {
  if (!car.courtesySequenceActive && fadeDirection <= 0) return;
  fadeDirection = -1;
}

void courtesy_update() {
  // Timeout automatico
  if (car.courtesySequenceActive && fadeDirection == 0 &&
      millis() - car.courtesyStartMs > car.courtesyDurationMs) {
    courtesy_stop();
  }

  if (fadeDirection == 0) return;
  if (millis() - lastFadeStep < FADE_STEP_MS) return;
  lastFadeStep = millis();

  fadeLevel = constrain((int)fadeLevel + fadeDirection * 5, 0, 255);

  analogWrite(PWM_DOOR_LED, fadeLevel);

  for (int i = 0; i < NEOPIXEL_DASH_COUNT; i++) {
    dashStrip.setPixelColor(i, car.r, car.g, car.b);
  }
  dashStrip.setBrightness(fadeLevel);
  dashStrip.show();

  if (fadeDirection > 0 && fadeLevel == 255) {
    fadeDirection = 0; // fade-in completato, resta acceso
  }
  if (fadeDirection < 0 && fadeLevel == 0) {
    fadeDirection = 0;
    car.courtesySequenceActive = false;
  }
}
