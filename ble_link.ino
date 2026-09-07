#include "state.h"

/* ============================================================
   Protocollo verso il modulo esterno (ESP32 in deep-sleep con
   BLE scan periodico, e in futuro eventualmente una OpenMV per
   gli abbaglianti). Formato riga: "CHIAVE:VALORE\n"
   Molto più robusto del parsing per indici di carattere
   dell'originale (che si rompeva con un solo dato mancante).
   Esempi:
     PROX:1        -> presenza rilevata con isteresi già applicata
     PROX:0
     HIGHBEAM:1     -> richiesta abbagliante da futuro modulo camera
   ============================================================ */

static String rxBuffer;

void bleLink_update() {
  while (Serial3.available()) {
    char c = Serial3.read();
    if (c == '\n') {
      bleLink_parseLine(rxBuffer);
      rxBuffer = "";
    } else if (c != '\r') {
      rxBuffer += c;
      if (rxBuffer.length() > 40) rxBuffer = ""; // anti-garbage
    }
  }
}

void bleLink_parseLine(const String &line) {
  int sep = line.indexOf(':');
  if (sep < 0) return;
  String key = line.substring(0, sep);
  String val = line.substring(sep + 1);

  if (key == "PROX") {
    car.proximityDetected = (val.toInt() == 1);
  } else if (key == "HIGHBEAM") {
    car.highBeamRequested = (val.toInt() == 1);
  }
}

void bleLink_sendStatus() {
  // Info utile al modulo esterno (es. per non svegliarsi a chiave inserita)
  Serial3.print("IGN:");
  Serial3.println(car.ignitionOn ? 1 : 0);
  Serial3.print("LOCK:");
  Serial3.println(car.carLocked ? 1 : 0);
}
