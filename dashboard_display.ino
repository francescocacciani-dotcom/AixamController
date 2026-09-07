#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "state.h"

Adafruit_NeoPixel dashStrip(NEOPIXEL_DASH_COUNT, NEOPIXEL_DASH_PIN, NEO_GRB + NEO_KHZ800);
extern uint8_t dashBrightness; // impostata da Nextion (spie luminosità quadro)

void dashboard_setup() {
  dashStrip.begin();
  dashStrip.setBrightness(255);
  dashStrip.show();
}

/* Mappa pixel -> stato (personalizzabile liberamente, qui una
   proposta di partenza che riprende la logica dell'originale):
   0 = setpoint temperatura in modifica (bianco lampeggiante)
   1 = sub attivo (rosso)
   2 = luci baffi/led manuali attivi (verde)
   3 = occhi/specchietti attivi (blu)
   4 = inverter attivo (magenta)
   5 = ventole frontali attive (verde)
   6 = ventilazione climatizzazione attiva (ciano) */
void dashboard_update() {
  if (car.courtesySequenceActive) return; // il fade ha priorità (door_courtesy.ino)
  if (!car.ignitionOn) {
    for (int i = 0; i < NEOPIXEL_DASH_COUNT; i++) dashStrip.setPixelColor(i, 0, 0, 0);
    dashStrip.show();
    return;
  }

  dashStrip.setPixelColor(1, car.subOn ? 255 : 0, 0, 0);
  dashStrip.setPixelColor(2, 0, car.baffiOn ? 255 : 0, 0);
  dashStrip.setPixelColor(3, 0, 0, car.mirrorBrightness > 0 ? 150 : 0);
  dashStrip.setPixelColor(4, car.inverterOn ? 255 : 0, 0, car.inverterOn ? 255 : 0);
  dashStrip.setPixelColor(5, 0, car.fanSpeed > 0 ? 255 : 0, 0);
  dashStrip.setPixelColor(6, 0, car.tempSetpoint > 0 ? 100 : 0, car.tempSetpoint > 0 ? 100 : 0);

  dashStrip.setBrightness(dashBrightness);
  dashStrip.show();
}
