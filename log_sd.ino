#include <SPI.h>
#include <SD.h>
#include "config.h"
#include "state.h"

/* ============================================================
   Log su SD (shield ufficiale Arduino, SD_CS_PIN = 4).
   L'enum LogLevel e i prototipi sono in config.h, non qui:
   Arduino genera i prototipi delle funzioni in cima al file
   concatenato PRIMA di aver visto tipi custom definiti in altri
   tab, quindi vanno dichiarati in un header incluso da tutti.

   Due princìpi guida:
   1) NON BLOCCARE MAI IL LOOP: aprire/chiudere un file SD costa
      diversi millisecondi. Per gli eventi non critici bufferiamo
      in RAM e scriviamo tutto insieme nel task periodico (500ms),
      un solo open/close invece di uno per riga.
   2) NON PERDERE DATI CRITICI: per ERROR/CRITICAL scriviamo e
      flushiamo subito, anche se costa qualche ms in più - un
      errore grave vale la pena di pagarlo. La SD (FAT) può
      corrompersi se stacchi l'alimentazione a metà scrittura:
      per questo i log non critici restano bufferati il più
      possibile, così il file resta chiuso la maggior parte del
      tempo.
   ============================================================ */

static const uint8_t LOG_BUF_LINES = 8;
static String logBuffer[LOG_BUF_LINES];
static uint8_t logBufCount = 0;

static const char *levelStr(LogLevel lvl) {
  switch (lvl) {
    case LOG_INFO: return "INFO";
    case LOG_WARN: return "WARN";
    case LOG_ERROR: return "ERROR";
    case LOG_CRITICAL: return "CRITICAL";
  }
  return "?";
}

void log_setup() {
  pinMode(ETH_CS_PIN, OUTPUT);
  digitalWrite(ETH_CS_PIN, HIGH); // disattiva il chip Ethernet se presente ma non usato

  car.sdAvailable = SD.begin(SD_CS_PIN);
  if (!car.sdAvailable) {
    Serial.println(F("SD non trovata: log solo su seriale"));
    return;
  }
  File f = SD.open(LOG_FILENAME, FILE_WRITE);
  if (f) {
    f.print(F("t_ms,level,tag,message\n"));
    f.print(F("0,INFO,BOOT,avvio sistema (timestamp relativo all'accensione, no RTC)\n"));
    f.close();
  }
}

static void log_flushLine(const String &line) {
  if (!car.sdAvailable) return;
  File f = SD.open(LOG_FILENAME, FILE_WRITE);
  if (!f) {
    car.sdAvailable = false; // la SD ha smesso di rispondere, non insistiamo ogni ciclo
    return;
  }
  f.print(line);
  f.close();
}

void log_write(LogLevel lvl, const char *tag, const String &msg) {
  String line = String(millis()) + "," + levelStr(lvl) + "," + tag + "," + msg + "\n";

  // Sempre in chiaro su seriale, utile col cavo USB in banco prova
  Serial.print(line);

  if (lvl >= LOG_ERROR) {
    log_flushLine(line); // scrittura e flush immediati per eventi gravi
    return;
  }

  if (logBufCount < LOG_BUF_LINES) {
    logBuffer[logBufCount++] = line;
  } else {
    car.logDropped++; // buffer pieno: meglio saperlo che bloccare il loop
  }
}

// Da chiamare nel task periodico (ogni 500ms circa)
void log_flushBuffer() {
  if (logBufCount == 0 || !car.sdAvailable) {
    logBufCount = 0;
    return;
  }
  File f = SD.open(LOG_FILENAME, FILE_WRITE);
  if (!f) {
    car.sdAvailable = false;
    logBufCount = 0;
    return;
  }
  for (uint8_t i = 0; i < logBufCount; i++) f.print(logBuffer[i]);
  f.close();
  logBufCount = 0;
}
