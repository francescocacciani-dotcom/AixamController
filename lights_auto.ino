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
static const unsigned long LOW_BEAM_OFF_DELAY_MS = 5000;
static const unsigned long LOW_BEAM_FADE_STEP_MS = 20;
static const uint8_t LOW_BEAM_FADE_STEP = 5;
static uint8_t lowBeamBrightness = 0;
static unsigned long lowBeamFadeChangedAt = 0;
static unsigned long lowBeamOffRequestedAt = 0;
static bool lowBeamOffDelayActive = false;

void lowBeamFade_update() {
  unsigned long now = millis();
  bool requestedOn = car.lightsAutoMode && car.lowBeamOn &&
                    car.ignitionOn && car.engineRunning;
  bool carAndEngineOff = !car.ignitionOn && !car.engineRunning;

  if (requestedOn) {
    lowBeamOffDelayActive = false;
  } else if (carAndEngineOff && lowBeamBrightness > 0) {
    if (!lowBeamOffDelayActive) {
      lowBeamOffRequestedAt = now;
      lowBeamOffDelayActive = true;
    }
    if (now - lowBeamOffRequestedAt < LOW_BEAM_OFF_DELAY_MS) return;
  } else if (car.lowBeamOn && lowBeamBrightness > 0) {
    // Wait for both shutdown signals before starting the delayed fade-off.
    return;
  } else {
    lowBeamOffDelayActive = false;
  }

  if (now - lowBeamFadeChangedAt < LOW_BEAM_FADE_STEP_MS) return;
  lowBeamFadeChangedAt = now;

  if (requestedOn) {
    lowBeamBrightness = min(255, lowBeamBrightness + LOW_BEAM_FADE_STEP);
  } else {
    lowBeamBrightness = (lowBeamBrightness > LOW_BEAM_FADE_STEP)
                          ? lowBeamBrightness - LOW_BEAM_FADE_STEP : 0;
  }

  analogWrite(LOW_BEAM_LEFT_PIN, lowBeamBrightness);
  analogWrite(LOW_BEAM_RIGHT_PIN, lowBeamBrightness);
}

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

