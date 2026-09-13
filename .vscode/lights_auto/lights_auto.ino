#include "config.h"
#include "state.h"

/* ============================================================
   Anabbaglianti/posizione automatici da fotoresistenza.
   Isteresi a due soglie (accensione/spegnimento diverse) per
   evitare che sul crepuscolo le luci lampeggino avanti-indietro.
   TODO calibrazione: valori di partenza, da tarare sul campo.
   ============================================================ */
static const int TH_POSITION_ON  = car.sensPosition-50; // sotto questo valore -> luci posizione ON
static const int TH_POSITION_OFF = car.sensPosition+100; // sopra questo -> OFF
static const int TH_LOWBEAM_ON   = car.sensLowBeam-50; // buio pieno -> anabbaglianti ON
static const int TH_LOWBEAM_OFF  = car.sensLowBeam+100;

static unsigned long lastLightsSample = 0;
static const unsigned long LIGHTS_SAMPLE_MS = 500;

void lightsAuto_update() {
  if (millis() - lastLightsSample < LIGHTS_SAMPLE_MS) return;
  lastLightsSample = millis();

  car.ambientLight = map(analogRead(PIN_LDR_AMBIENT),0,1024,1024,0);

  digitalRead(SW_LIGHTS_AUTO_MANUAL) == HIGH ? car.lightsAutoMode = false
                                              : car.lightsAutoMode = true;

  digitalWrite(LED_MODE_AUTO,   car.lightsAutoMode ? HIGH : LOW);
  digitalWrite(LED_MODE_MANUAL, car.lightsAutoMode ? LOW  : HIGH);

  if (!car.lightsAutoMode) {
    // In manuale le luci le comanda l'utente da Nextion / pulsante
    return;
  }

  // Isteresi posizione
  if (!car.positionLightsOn && car.ambientLight < TH_POSITION_ON) {
    car.positionLightsOn = true;
  } else if (car.positionLightsOn && car.ambientLight > TH_POSITION_OFF) {
    car.positionLightsOn = false;
  }

  // Isteresi anabbaglianti (si accendono solo se già in posizione)
  if (car.positionLightsOn) {
    if (!car.lowBeamOn && car.ambientLight < TH_LOWBEAM_ON) {
      car.lowBeamOn = true;
    } else if (car.lowBeamOn && car.ambientLight > TH_LOWBEAM_OFF) {
      car.lowBeamOn = false;
    }
  } else {
    car.lowBeamOn = false;
  }

  // car.highBeamRequested resta gestito da un futuro modulo esterno
  // (OpenMV/ESP32-CAM) via comando seriale su Serial3 - vedi ble_link.ino
}

/**
* @brief Shuts off the pannel leds when sleep.
*/
void lightsAuto_sleep(){
  digitalWrite(LED_MODE_AUTO, LOW);
  digitalWrite(LED_MODE_MANUAL, LOW);
}

