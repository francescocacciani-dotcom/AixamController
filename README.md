# Aixam A721 Controller v2 — Arduino Mega 2560

## Librerie richieste (Arduino IDE → Library Manager)
- Adafruit NeoPixel
- ITEAD Nextion (libreria `Nextion.h` usata anche nel progetto originale)
- LiquidCrystal I2C (es. quella di Frank de Brabander)
- TinyDHT
- SD (inclusa nell'IDE Arduino, nessuna installazione extra)

## Shield SD (Ethernet Shield 2 o compatibile)
- `SD_CS_PIN` = 4, `ETH_CS_PIN` = 10 — entrambi riservati anche se non usi
  l'Ethernet, altrimenti il bus SPI va in conflitto
- Su Mega lo SPI vero (MISO/MOSI/SCK) passa dall'header ICSP, quindi i pin
  50/51/52 restano liberi per altro (qui usati per NeoPixel soffitto ecc.)
- Per liberare i pin 4 e 10 ho spostato: `PWM_MIRROR_LED` da 4 a 11,
  `ENCODER_SW` da 10 a 12

## Formato log (`log_sd.ino`, file `aixamlog.csv` sulla SD)
Righe CSV: `t_ms,level,tag,message`. `t_ms` è `millis()` dal boot — **non è
un orario reale**, non c'è un RTC a bordo. Se vuoi timestamp veri basta
aggiungere un modulo RTC economico (es. DS3231, I2C, bus già disponibile)
e un modulo `rtc.ino` che sostituisca `millis()` nel formato riga.

Livelli: `INFO` (eventi normali: chiave, chiusura/apertura, ecc.),
`WARN` (es. tensione batteria bassa, override attivato), `ERROR`/`CRITICAL`
(scritti e flushati subito, senza bufferizzare, per non perderli). Gli
`INFO`/`WARN` restano in un buffer di 8 righe in RAM e vengono scritti
tutti insieme ogni 500ms — un solo open/close invece di uno per riga,
per non introdurre microstop nel loop.

Se la SD non è inserita o si guasta, il sistema continua a funzionare
normalmente: logga solo su seriale USB e imposta `car.sdAvailable = false`,
non c'è nessun blocco.

## Struttura sketch
Arduino IDE unisce automaticamente tutti i `.ino` nella stessa cartella:
non serve includere manualmente i moduli tra loro, solo `config.h` e `state.h`
dove servono i simboli.

- `config.h` — mappa pin, unica fonte di verità sul cablaggio
- `state.h` — struct `CarState`, stato condiviso
- `AixamController.ino` — setup/loop, dispatcher
- `power.ino` — sleep AVR / wake su chiave
- `log_sd.ino` — log eventi/errori su SD (shield ufficiale), livelli INFO/WARN/ERROR/CRITICAL
- `eeprom_store.ino` — persistenza impostazioni (EEPROM I2C 0x50)
- `lights_auto.ino` — anabbaglianti/posizione automatici con isteresi
- `door_courtesy.ino` — sequenza luci di cortesia (porta + sblocco auto)
- `central_locking.ino` — apertura/chiusura centralizzata
- `ble_link.ino` — protocollo verso modulo esterno BLE presenza / futura camera abbaglianti
- `climate.ino` — ventola 3 velocità auto/manuale + encoder setpoint
- `ceiling_light.ino` — luce interna a soffitto (neopixel dedicato)
- `dashboard_display.ino` — display neopixel console centrale
- `lcd_secondary.ino` — LCD I2C info secondarie, paginabile
- `nextion_ui.ino` — VECCHIA configurazione HMI (stessi id/pagine/nomi del tuo sketch originale), per testare subito senza rifare l'interfaccia

## Encoder rotativo (setpoint temperatura)
`ENCODER_CLK` (18) e `ENCODER_DT` (19) sono entrambi in interrupt hardware
(`CHANGE`), con una vera decodifica quadrature a tabella di Gray code
(non il solito "leggo DT quando CLK scende") — quel metodo semplificato
è sensibile al rimbalzo meccanico dei contatti e dà i sintomi che hai
visto (conteggi persi/doppi, "a scatti"). Con la tabella a 4 stati le
transizioni non valide vengono scartate automaticamente.

Gli unici pin con interrupt esterno sul Mega sono 2, 3, 18, 19, 20, 21 —
2 è la chiave, 3 il PWM ventole frontali, 20/21 l'I2C: restavano solo
18/19, usati apposta per questo (occupano anche i pin di `Serial1`, se
in futuro ti serve va spostato).

**Consiglio hardware complementare**: se il rimbalzo è pesante, aggiungi
2 condensatori ceramici 100nF tra CLK/GND e DT/GND, il più vicino
possibile all'encoder — riduce le transizioni spurie a monte, la tabella
software le scarterebbe comunque ma meno lavoro per la CPU e meno rischio
di doppio conteggio a rotazione molto veloce.

## Cose DA FARE prima di flashare (elenco TODO nel codice)
1. **Polarità PIN_IGNITION dopo l'optoisolatore** — verifica se HIGH = chiave
   inserita o il contrario col tuo circuito reale, in `AixamController.ino`.
2. **Soglie sensore porta** (`handleDoorSense`) — tarale sul tuo partitore
   resistivo reale, o sostituiscile con microswitch dedicati (consigliato).
3. **Rapporto del partitore di tensione batteria** (`readBatteryVoltage`) —
   il valore `0..15500` è ereditato dal progetto originale, verificalo con
   un multimetro.
4. **Soglie luci automatiche** (`lights_auto.ino`) — la fotoresistenza è
   nuova rispetto all'originale (prima leggevi la luminosità dall'ESP32 via
   seriale), calibra `TH_POSITION_*` e `TH_LOWBEAM_*` sul tuo LDR reale.
5. **Nomi componenti Nextion** — nel tuo nuovo progetto HMI crea i componenti
   con i nomi in `nextion_ui.ino`, oppure rinominali lì per farli combaciare.

## Sicurezza sblocco automatico (BLE)
Evitare un beacon puramente passivo (solo RSSI): usare connessione BLE GATT
con challenge-response (token HMAC-SHA256 su segreto condiviso + contatore/
timestamp anti-replay), validato lato ESP32 prima di mandare `PROX:1` al
Mega. Il BLE deve pilotare solo le serrature, mai l'accensione/immobilizer:
in caso di compromissione il danno resta contenuto all'apertura porte.

## Terzo contatto centralina/telecomando
Collegato su `SENSE_EXTRA_CONTACT` (via optoisolatore, come lock/unlock).
Per ora solo loggato in `central_locking.ino` (`centralLocking_onExtraContact`)
— nessun comportamento assegnato, pronto per quando deciderai l'uso.

## Modulo esterno per prossimità BLE
Non incluso in questo sketch (vive su un secondo microcontrollore, es. ESP32
in deep-sleep con scan BLE periodico). Deve mandare su UART verso Serial3
del Mega righe testuali tipo:
```
PROX:1
PROX:0
```
con isteresi già applicata lato ESP32 (es. N letture RSSI sotto soglia in
X secondi prima di mandare `PROX:1`) per evitare sblocchi accidentali.

## Lista componenti — sensing chiusura/apertura (sostituisce i 2 regolatori 5V)

**Opzione consigliata: optoisolatori** (isolamento galvanico dalle linee sporche 12V auto)
- 3x PC817 (o 1x modulo pronto "4 channel optocoupler isolation board", ~2-3€,
  copre lock+unlock+contatto extra con un canale di scorta)
- 3x resistenza 1.2–1.5kΩ 1/4W (limitazione corrente LED lato 12V — già
  presenti se usi il modulo pronto)
- 3x diodo 1N4148 in antiparallelo al LED di ogni opto (protezione da
  inversione di polarità/transitori negativi)
- 1x TVS unidirezionale (es. SMBJ15A) sulla linea +12V che alimenta i LED
  degli opto — protezione da load-dump, consigliato ma opzionale su questa
  parte specifica visto che gli opto isolano già bene il lato logico
- Pull-up: uso quelli interni dell'ATmega (`INPUT_PULLUP`), non servono
  resistenze esterne sul lato fototransistor
- 3x condensatore ceramico 100nF tra pin digitale e GND, vicino all'Arduino
  (filtro/debounce hardware)

**Opzione alternativa: partitore + zener** (più economica, meno isolamento)
- Per ogni linea: R_top = 6.8kΩ, R_bottom = 2.7kΩ (rapporto ~0.284 →
  3.4V a 12V, 4.1V a 14.4V)
- 1x zener 5.1V 0.5W in parallelo all'uscita del partitore, verso massa —
  è l'elemento che ti protegge davvero dagli spike, non il partitore da solo
- 1x condensatore ceramico 100nF in parallelo al zener

In entrambi i casi: **rimuovi i due regolatori 12V→5V** attuali dedicati al
solo sensing, non servono più — libera spazio e consumo statico inutile.
Se quei regolatori alimentano anche altro (Nextion, LCD...) tienili per
quello, ma non per il sensing lock/unlock/contatto extra.

## Note elettriche (vedi anche la chat)
- Interporre optoisolatori (PC817 o simili) su: chiave, motore, ricarica
  batteria, sensori lock/unlock — protezione + segnali digitali puliti.
- `PIN_IGNITION` è su D2 (INT0) apposta per il wake da sleep.
- Prevedere un relè/MOSFET high-side che stacchi fisicamente Nextion e
  strip LED dal +12V a chiave spenta e auto chiusa.
- Se il regolatore 12V→5V è lineare, sostituirlo con un buck a basso Iq
  per ridurre il consumo statico a riposo.
