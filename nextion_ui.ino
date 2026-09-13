#include <Nextion.h>
#include "config.h"
#include "state.h"

/* ============================================================
   VECCHIA CONFIGURAZIONE NEXTION, portata 1:1 (stessa pagina,
   stesso id, stesso nome componente del tuo sketch originale)
   così puoi testare subito con lo stesso file .HMI, senza
   dover rifare l'interfaccia grafica adesso.

   Alcuni pulsanti (LPon, Lon) erano dichiarati nell'originale
   ma MAI collegati con attachPop - probabile dimenticanza nel
   vecchio codice. Qui li collego, se non li vuoi funzionanti
   ancora togli semplicemente la riga attachPop corrispondente.
   ============================================================ */

NexButton SubOn   = NexButton(1, 1, "b0");
NexButton SubOff  = NexButton(1, 2, "b1");
NexButton SubFav  = NexButton(1, 6, "b3");
NexButton AOS     = NexButton(1, 3, "tAOS");

NexButton tauto   = NexButton(2, 4, "tauto");

NexButton ventOn  = NexButton(15, 5, "b2");
NexButton ventOff = NexButton(15, 6, "b3");
NexSlider ventV   = NexSlider(15, 4, "h0");

NexButton LedOn   = NexButton(3, 2, "b1");
NexButton LedOff  = NexButton(3, 3, "b2");
NexButton LedFav  = NexButton(3, 5, "b3");

NexSlider LPs     = NexSlider(5, 4, "h0");
NexButton LPa     = NexButton(5, 2, "b1");
NexButton LPoff   = NexButton(5, 3, "b2");
NexButton LPon    = NexButton(5, 7, "b3");

NexSlider Ls      = NexSlider(6, 6, "h0");
NexButton La      = NexButton(6, 3, "b1");
NexButton Loff    = NexButton(6, 4, "b2");
NexButton Lon     = NexButton(6, 7, "b3");

NexSlider Fs      = NexSlider(7, 6, "h0");
NexButton Fa      = NexButton(7, 3, "b1");
NexButton Foff    = NexButton(7, 4, "b2");
NexButton Fon     = NexButton(7, 2, "b3");

NexButton OCOn    = NexButton(8, 2, "b1");
NexButton OCOff   = NexButton(8, 3, "b2");
NexButton OCOfav  = NexButton(8, 5, "b3");
NexButton OCOa    = NexButton(8, 6, "tAutoOcchi");

NexButton IOn     = NexButton(9, 2, "b1");
NexButton IOff    = NexButton(9, 3, "b2");

NexButton LSa     = NexButton(11, 3, "b2");
NexButton LSs     = NexButton(11, 4, "b3");
NexButton LSm     = NexButton(11, 7, "b4");
NexButton LSp     = NexButton(11, 8, "b5");

NexButton SpieOn  = NexButton(12, 3, "b2");
NexButton SpieOff = NexButton(12, 4, "b3");
NexSlider SpieS   = NexSlider(12, 5, "h0");

NexButton LuceiOn  = NexButton(13, 3, "b2");
NexButton LuceiOff = NexButton(13, 4, "b3");
NexButton LuceiC   = NexButton(13, 5, "tLIC");

NexButton m01      = NexButton(14, 3, "b1");
NexButton p01      = NexButton(14, 4, "b2");
NexButton Override = NexButton(14, 6, "b3");

uint8_t dashBrightness = 200;

NexTouch *nex_listen_list[] = {
  &SubOn, &SubOff, &SubFav, &AOS,
  &tauto,
  &ventOn, &ventOff, &ventV,
  &LedOn, &LedOff, &LedFav,
  &LPs, &LPa, &LPoff, &LPon,
  &Ls, &La, &Loff, &Lon,
  &Fs, &Fa, &Foff, &Fon,
  &OCOn, &OCOff, &OCOfav, &OCOa,
  &IOn, &IOff,
  &LSa, &LSs, &LSm, &LSp,
  &SpieOn, &SpieOff, &SpieS,
  &LuceiOn, &LuceiOff, &LuceiC,
  &m01, &p01, &Override,
  NULL
};

// ---------- Sub ----------
void nex_SubOn(void *ptr)  { car.subOn = true; }
void nex_SubOff(void *ptr) { car.subOn = false; }
void nex_SubFav(void *ptr) { car.reprogrammablePush = 0; }
void nex_AOS(void *ptr)    { car.subAutoMode = !car.subAutoMode; }

// ---------- Modalità clima (auto/man) ----------
void nex_tauto(void *ptr) { car.fanAutoMode = !car.fanAutoMode; }

// ---------- Ventole frontali (PWM_AUX) ----------
void nex_ventOn(void *ptr)  { car.frontFanOn = true; }
void nex_ventOff(void *ptr) { car.frontFanOn = false; }
void nex_ventV(void *ptr) {
  uint32_t v = 0;
  ventV.getValue(&v);
  car.frontFanSpeed = (uint8_t)v; // 0-100 dallo slider
}

// ---------- led porta manuali ----------
void nex_LedOn(void *ptr)  { car.ledOn = true; }
void nex_LedOff(void *ptr) { car.ledOn = false; }
void nex_LedFav(void *ptr) { car.reprogrammablePush=1; }

// ---------- Luci posizione ----------
void nex_LPsCall(void *ptr) {
  uint32_t v = 0;
  LPs.getValue(&v);
  car.sensPosition = v;
}
void nex_LPa(void *ptr)   { car.positionLightsAuto = true; }
void nex_LPoff(void *ptr) { car.positionLightsAuto = false; car.positionLightsOn = false; }
void nex_LPon(void *ptr)  { car.positionLightsAuto = false; car.positionLightsOn = true; }

// ---------- Anabbaglianti ----------
void nex_LsCall(void *ptr) {
  uint32_t v = 0;
  Ls.getValue(&v);
  car.sensLowBeam = v;
}
void nex_La(void *ptr)   { car.lowBeamAuto = true; }
void nex_Loff(void *ptr) { car.lowBeamAuto = false; car.lowBeamOn = false; }
void nex_Lon(void *ptr)  { car.lowBeamAuto = false; car.lowBeamOn = true; }

// ---------- Fendinebbia ----------
void nex_FsCall(void *ptr) {
  uint32_t v = 0;
  Fs.getValue(&v);
  car.sensFog = v;
}
void nex_Fa(void *ptr)   { car.fogLightsAuto = true; }
void nex_Foff(void *ptr) { car.fogLightsAuto = false; car.fogLightsOn = false; }
void nex_Fon(void *ptr)  { car.fogLightsAuto = false; car.fogLightsOn = true; }

// ---------- Occhi  ----------
void nex_OCOn(void *ptr)   { car.occhiOn = true; }
void nex_OCOff(void *ptr)  { car.occhiOn = false; }
void nex_OCOfav(void *ptr) { car.reprogrammablePush = 2; }
void nex_OCOa(void *ptr)   { if(++car.occhiAutoOn>=3)car.occhiAutoOn = 0;}//occhi auto on

// ---------- Inverter ----------
void nex_IOn(void *ptr)  { car.inverterOn = true; }
void nex_IOff(void *ptr) { car.inverterOn = false; }

// ---------- Luci sportello (cortesia) ----------
void nex_LSa(void *ptr) { 
  car.doorCurtesyEnabled = true; 
  log_write(LOG_INFO, "NEXTION", "Door curtesy enabled");
}
void nex_LSs(void *ptr) { 
  car.doorCurtesyEnabled = false; 
  courtesy_stop(); 
  log_write(LOG_INFO, "NEXTION", "Door curtesy disabled");
}
void nex_LSm(void *ptr) {
  car.courtesyDurationMs = constrain((long)car.courtesyDurationMs - 5000, 5000, 120000);
  log_write(LOG_INFO, "NEXTION", "Durata cortesia -5s: " + String(car.courtesyDurationMs / 1000) + "s");
}
void nex_LSp(void *ptr) {
  car.courtesyDurationMs = constrain((long)car.courtesyDurationMs + 5000, 5000, 120000);
  log_write(LOG_INFO, "NEXTION", "Durata cortesia +5s: " + String(car.courtesyDurationMs / 1000) + "s");
}

// ---------- Spie quadro (dashboard neopixel) ----------
void nex_SpieOn(void *ptr)  { car.dashIndicatorsOn = true; }
void nex_SpieOff(void *ptr) { car.dashIndicatorsOn = false; }
void nex_SpieS(void *ptr) {
  uint32_t v = 0;
  SpieS.getValue(&v);
  dashBrightness = map(v, 0, 100, 5, 255);
}

// ---------- Luce interna a soffitto ----------
void nex_LuceiOn(void *ptr)  { car.ceilingLightOn = true;  ceilingLight_render(); }
void nex_LuceiOff(void *ptr) { car.ceilingLightOn = false; ceilingLight_render(); }
void nex_LuceiC(void *ptr)   { ceilingLight_setColorIndex(car.ceilingColorIndex + 1); }

// ---------- Taratura tensione batteria ----------
void nex_m01(void *ptr) { car.batteryVoltageProtection -= 0.1; }
void nex_p01(void *ptr) { car.batteryVoltageProtection += 0.1; }
void nex_Override(void *ptr) {
  car.voltageOverride = !car.voltageOverride;
  log_write(LOG_WARN, "NEXTION", car.voltageOverride ? "Override tensione ATTIVATO" : "Override tensione disattivato");
}

void nextionUI_setup() {
  nexInit();
  SubOn.attachPop(nex_SubOn, &SubOn);
  SubOff.attachPop(nex_SubOff, &SubOff);
  SubFav.attachPop(nex_SubFav, &SubFav);
  AOS.attachPop(nex_AOS, &AOS);
  tauto.attachPop(nex_tauto, &tauto); // auto occhi
  ventOn.attachPop(nex_ventOn, &ventOn);
  ventOff.attachPop(nex_ventOff, &ventOff);
  ventV.attachPop(nex_ventV, &ventV);
  LedOn.attachPop(nex_LedOn, &LedOn);
  LedOff.attachPop(nex_LedOff, &LedOff);
  LedFav.attachPop(nex_LedFav, &LedFav);
  LPs.attachPop(nex_LPsCall);
  LPa.attachPop(nex_LPa, &LPa);
  LPoff.attachPop(nex_LPoff, &LPoff);
  LPon.attachPop(nex_LPon, &LPon);
  Ls.attachPop(nex_LsCall);
  La.attachPop(nex_La, &La);
  Loff.attachPop(nex_Loff, &Loff);
  Lon.attachPop(nex_Lon, &Lon);
  Fs.attachPop(nex_FsCall);
  Fa.attachPop(nex_Fa, &Fa);
  Foff.attachPop(nex_Foff, &Foff);
  Fon.attachPop(nex_Fon, &Fon);
  OCOn.attachPop(nex_OCOn, &OCOn);
  OCOff.attachPop(nex_OCOff, &OCOff);
  OCOfav.attachPop(nex_OCOfav, &OCOfav);
  OCOa.attachPop(nex_OCOa, &OCOa);
  IOn.attachPop(nex_IOn, &IOn);
  IOff.attachPop(nex_IOff, &IOff);
  LSa.attachPop(nex_LSa, &LSa);
  LSs.attachPop(nex_LSs, &LSs);
  LSm.attachPop(nex_LSm, &LSm);
  LSp.attachPop(nex_LSp, &LSp);
  SpieOn.attachPop(nex_SpieOn, &SpieOn);
  SpieOff.attachPop(nex_SpieOff, &SpieOff);
  SpieS.attachPop(nex_SpieS, &SpieS);
  LuceiOn.attachPop(nex_LuceiOn, &LuceiOn);
  LuceiOff.attachPop(nex_LuceiOff, &LuceiOff);
  LuceiC.attachPop(nex_LuceiC, &LuceiC);
  m01.attachPop(nex_m01, &m01);
  p01.attachPop(nex_p01, &p01);
  Override.attachPop(nex_Override, &Override);
}

void nextionUI_endCmd() {
  Serial2.write(0xFF); Serial2.write(0xFF); Serial2.write(0xFF);
}


void nextionUI_sendUpdate() {
  static char buf[48];
  static char fbuf[10];
  snprintf(buf, sizeof(buf), "tin.txt=\"T.in = %d\"", car.tempInside);
  Serial2.print(buf); nextionUI_endCmd();
  snprintf(buf, sizeof(buf), "tout.txt=\"T.out = %d\"", car.tempOutside);
  Serial2.print(buf); nextionUI_endCmd();
  snprintf(buf, sizeof(buf), "tset.txt=\"T.set = %d\"", car.tempSetpoint);
  Serial2.print(buf); nextionUI_endCmd();
  snprintf(buf, sizeof(buf), "tLS.txt=\"%d\"", car.courtesyDurationMs/1000); // curtesy duration
  Serial2.print(buf); nextionUI_endCmd();
  Serial2.print(car.subAutoMode ? "tAOS.txt=\"ON\"" : "tAOS.txt=\"OFF\""); //sub auto on on key
  nextionUI_endCmd();
  switch(car.occhiAutoOn){
      case 0:
        Serial2.print("tAutoOcchi.txt=\"Off\"");
        break;
      case 1:
        Serial2.print("tAutoOcchi.txt=\"Chiave\"");
        break;
      case 2:
        Serial2.print("tAutoOcchi.txt=\"Motore\"");
        break;
  }
  nextionUI_endCmd();
  dtostrf(car.batteryVoltage, 4, 2, fbuf); // AVR: snprintf non supporta %f senza libprintf_flt
  snprintf(buf, sizeof(buf), "tvolt.txt=\"V= %s\"", fbuf);
  Serial2.print(buf); nextionUI_endCmd();
  dtostrf(car.batteryVoltageProtection, 4, 2, fbuf); // AVR: snprintf non supporta %f senza libprintf_flt
  snprintf(buf, sizeof(buf), "tVolt.txt=\"V= %s\"", fbuf);
  Serial2.print(buf); nextionUI_endCmd();
  if(car.voltageOverride) Serial2.print("tStatoVolt.txt=\"override\"");
  else Serial2.print("tStatoVolt.txt=\"batteria OK\"");
  nextionUI_endCmd();
  Serial2.print(car.fanAutoMode ? F("tauto.txt=\"Automatico\"") : F("tauto.txt=\"Manuale\""));
  nextionUI_endCmd();
}
