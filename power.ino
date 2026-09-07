#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include "config.h"
#include "state.h"

/* ============================================================
   CORREZIONE rispetto alla versione precedente di questo file:
   avevo scritto che un carattere in arrivo su Serial3 (dal modulo
   BLE) sveglia il Mega dal power-down. Non è vero, ed è importante
   che tu lo sappia: in SLEEP_MODE_PWR_DOWN si fermano TUTTI i clock
   interni, incluso quello della USART. Senza clock la USART non può
   campionare i bit in arrivo - non è un interrupt mancante da
   collegare, è un limite fisico del chip. Qualunque cosa l'ESP32
   mandi mentre il Mega dorme viene semplicemente persa.

   La ISR serial3ISR() che c'era prima non era mai collegata a
   nessun interrupt reale (dead code) - l'ho tolta.

   FIX ADOTTATO: il Watchdog Timer ha un oscillatore interno TUTTO
   SUO, indipendente dal clock principale, e continua a contare
   anche in power-down. Lo uso per svegliare la CPU periodicamente
   (~8s) SENZA resettarla (modalità solo-interrupt, non reset).
   Ad ogni risveglio la USART torna viva e loop() gira normalmente.

   Questo NON risolve da solo lo sblocco per prossimità mentre
   l'auto è chiusa e ferma: risolve solo il fatto che il Mega non
   resti sordo per sempre. Per la prossimità serve, lato ESP32,
   un vero protocollo a richiesta/risposta (il Mega si sveglia,
   chiede "c'è qualcuno vicino?", l'ESP32 risponde) oppure un pin
   dedicato di wake pilotato dall'ESP32 - lo vediamo quando
   scriviamo lo sketch del modulo BLE.
   ============================================================ */

volatile bool wakeFlag = false;

void ignitionISR() {
  wakeFlag = true;
}

ISR(WDT_vect) {
  wakeFlag = true; // qui il watchdog è solo un timer di risveglio, non un reset
}

void power_setup() {
  pinMode(PIN_IGNITION, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_IGNITION), ignitionISR, RISING);
}

// Riarma il watchdog in modalità SOLO interrupt (WDE=0, WDIE=1): sveglia
// la CPU ma non la resetta. Va richiamato prima di ogni sleep_cpu(),
// perché l'hardware disattiva da solo il bit WDIE dopo ogni scatto -
// se non lo si riarma, il timeout successivo farebbe un reset vero.
static void power_armWatchdogWake() {
  noInterrupts();
  wdt_reset();
  MCUSR &= ~(1 << WDRF);
  WDTCSR |= (1 << WDCE) | (1 << WDE);
  WDTCSR = (1 << WDIE) | (1 << WDP3) | (1 << WDP0); // ~8s, solo interrupt
  interrupts();
}

// Da chiamare quando: chiave OFF, nessuna sequenza di cortesia attiva.
// Ritorna appena si verifica un wake event (chiave girata O timeout
// watchdog periodico).
void power_enterSleepIfIdle() {
  if (car.ignitionOn) return;
  if (car.courtesySequenceActive) return;
  if (car.proximityDetected) return; // restiamo svegli finché serve

  wakeFlag = false;
  /*power_armWatchdogWake();

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  noInterrupts();
  sleep_enable();
  interrupts();
  sleep_cpu();

  // --- risveglio qui: o chiave girata, o watchdog scattato ---
  sleep_disable();
  wdt_disable(); // rete di sicurezza: se per qualche motivo il loop
                  // impiegasse più di ~8s a tornare al prossimo sleep,
                  // evita che un secondo timeout causi un reset vero*/
}
