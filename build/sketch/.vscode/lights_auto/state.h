#line 1 "C:\\Users\\franc\\OneDrive\\Documenti\\Arduino\\AixamController\\.vscode\\lights_auto\\state.h"
#ifndef STATE_H
#define STATE_H
#include <Arduino.h>

/* ============================================================
   Stato macchina globale. Un solo posto dove vive la verità,
   così ogni modulo legge/scrive qui invece di variabili sparse.
   ============================================================ */

struct CarState {
  // Ignition / power
  bool ignitionOn = false;
  bool lastIgnitionOn = false;
  bool engineRunning = false;
  bool lastEngineRunning =false;
  //bool batteryCharging = false;
  float batteryVoltage = 0;
  float batteryVoltageProtection = 11.5;
  bool batteryLow = false;
  bool voltageOverride = false;
  
  // Power controll
  bool powerAccessories = false;
  bool powerFrontCar = false;

  // Porte / avvicinamento
  bool doorCurtesyEnabled = true;
  bool doorOpen = false;
  bool lastDoorOpen = false;
  bool courtesySequenceActive = false;
  unsigned long courtesyStartMs = 0;
  unsigned long courtesyDurationMs = 30000; // regolabile da Nextion

  // Chiusura centralizzata
  bool carLocked = true;
  bool lockCommandPending = false;
  bool proximityDetected = false; // da modulo BLE esterno, già con HMAC+isteresi lato ESP32

  // 3° contatto centralina/telecomando, uso non ancora deciso
  bool extraContactActive = false;
  bool lastExtraContactActive = false;
  unsigned long extraContactLastMs = 0;

  // Luci automatiche marcia
  bool lightsAutoMode = true;     // da interruttore 2 posizioni
  bool positionLightsOn = false;
  bool lowBeamOn = false;
  bool highBeamRequested = false; // riservato a futuro modulo camera
  int ambientLight = 0;

  // Baffi / specchietti (relè + PWM)
  bool baffiOn = false;
  bool baffiManualOverride = false;
  uint8_t mirrorBrightness = 0;

  // Occhi d'angelo
  bool occhiOn = false;
  uint8_t occhiAutoOn = 0; //0 = non si accendono da soli, 1=si accendono con il primo scatto della chiave, 2=si accendono con l'accenzione del motore

  // Luce interna a soffitto (neopixel)
  bool ceilingLightOn = false;
  uint8_t ceilingColorIndex = 0;

  // Climatizzazione
  bool fanAutoMode = true;
  int fanSpeed = 0;          // 0..3
  int tempSetpoint = 20;     // gradi
  int tempInside = 0;
  int tempOutside = 0;

  // Sub / inverter (impianto audio/accessori)
  bool subOn = false;
  bool inverterOn = false;
  bool subAutoMode = false;

  // led interni
  bool ledOn = false;

  // Reprogrammable front pushbutton
  uint8_t reprogrammablePush= 2; // 0 = sub, 1 = led, 2 = occhi d'angelo

  // LCD paging
  uint8_t lcdPage = 0;
  bool lcdBacklightOn = 0;

  // Colore luce interna corrente (RGB)
  uint8_t r = 255, g = 255, b = 255;

  // --- Sistema di log su SD ---
  bool sdAvailable = false;
  uint16_t logDropped = 0; // righe perse se il buffer si riempie (SD assente/lenta)

  // --- Campi per compatibilità con la vecchia interfaccia Nextion ---
  // (funzioni presenti nel vecchio sketch, qui portate 1:1 per permetterti
  // di testare con lo stesso progetto .HMI; alcune non hanno ancora una
  // logica di controllo dietro, sono solo lette/scritte per ora)
  bool positionLightsAuto = true;   // FLPa / FLPoff / FLPon
  bool lowBeamAuto = true;          // FLa / FLoff
  bool fogLightsOn = false;         // FFon / FFoff
  bool fogLightsAuto = false;       // FFa
  int  sensPosition = 500;          // LPs - sensibilità luci posizione
  int  sensLowBeam = 250;           // Ls  - sensibilità anabbaglianti
  int  sensFog = 300;               // Fs  - sensibilità fendinebbia
  bool mirrorAuto = false;          // FOCOa - occhi/specchietti auto
  bool dashIndicatorsOn = true;     // SpieOn/SpieOff - "spie" quadro
  bool frontFanOn = false;          // FventOn/FventOff - ventole frontali
  uint8_t frontFanSpeed = 0;        // FventV - velocità ventole frontali (0-100 da slider)
  int  voltageSensitivity = 0;      // Fp01/Fm01 - offset di taratura tensione
       // FOverride
};

extern CarState car;

#endif
