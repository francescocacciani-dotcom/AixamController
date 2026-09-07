#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

/* ============================================================
   AIXAM A721 - CONTROLLER V2 - MAPPA PIN (Arduino Mega 2560)
   ============================================================
   Riservati dall'hardware, NON riassegnare:
   0,1   = Serial0 (USB debug)
   14,15 = Serial3 (link ESP32/BLE presenza)
   16,17 = Serial2 (Nextion)
   18,19 = encoder rotativo (CLK/DT), scelti qui apposta perché
           sono tra i pochi pin con interrupt hardware sul Mega
           (gli altri sono 2,3,20,21, tutti già occupati)
   20,21 = I2C SDA/SCL (EEPROM esterna 0x50 + LCD I2C)
   ============================================================ */

// ---------- Wake / ignition (via optoisolatore consigliato) ----------
#define PIN_IGNITION        2   // INT0 - "chiave" - usato anche per wake da sleep
#define PIN_ENGINE_RUNNING   31  // "motore"
#define PIN_BATTERY_CHARGE   45  // sensore carica batteria

// ---------- PWM dimmer (MOSFET lato basso) ----------
#define PWM_AUX              7   // dimming ventole frontali ("vent" Nextion, era ambiguo nell'originale)
#define PWM_OCCHI            6  // dimming led occhi/specchietti (spostato da pin 4: libero per SD CS)
#define PWM_DOOR_LED         5   // dimming led "baffi"/cortesia porta

// ---------- Encoder rotativo (setpoint temperatura) ----------
// CLK su interrupt hardware vero (Mega: 2,3,18,19,20,21 sono gli unici
// pin con interrupt esterno). 18/19 sono liberi (Serial1 non è usato
// in questo progetto) - se in futuro ti serve Serial1, andranno spostati.
#define ENCODER_CLK          18  // INT3 - lettura per interrupt, non più a polling
#define ENCODER_DT           19  // solo lettura digitale, non serve interrupt
#define ENCODER_SW           52  // opzionale, invariato

// ---------- LED indicatori modalità luci marcia ----------
#define LED_MODE_AUTO         8
#define LED_MODE_MANUAL       9

// ---------- Shield ufficiale Arduino con lettore SD (Ethernet Shield 2 o simile) ----------
#define SD_CS_PIN             4   // chip select SD, standard su shield ufficiali Arduino
#define ETH_CS_PIN           10   // chip select W5500 (se presente sulla shield) - va tenuto HIGH se non usato
#define LOG_FILENAME          "aixamlog.csv"

// ---------- Relè (board attiva LOW tipica) ----------
#define RELAY_SUB            22
#define RELAY_INVERTER       23
#define RELAY_LED            24  // Led
#define RELAY_MIRROR         25  // alimentazione led sotto specchietti/occhi
#define RELAY_FAN1           26
#define RELAY_FAN2           27
#define RELAY_FAN3           28
#define RELAY_SPARE          29

// ---------- Selettore velocità ventola (3 posizioni, contatti esistenti) ----------
#define SW_FAN_POS1           32
#define SW_FAN_POS2           33
#define SW_FAN_POS3           34

// ---------- Sensori temperatura DHT11 ----------
#define DHT_EXTERNAL          35
#define DHT_INTERNAL_1        36
#define DHT_INTERNAL_2        42

// ---------- NeoPixel ----------
#define NEOPIXEL_DASH_PIN     37  // display console centrale
#define NEOPIXEL_DASH_COUNT   10
#define NEOPIXEL_CEIL_PIN     30  // luce interna superiore
#define NEOPIXEL_CEIL_COUNT   8

// ---------- Pulsanti fisici (4 totali, INPUT_PULLUP) ----------
#define BTN_BAFFI_MANUAL      38  // toggle manuale baffi/specchietti
#define BTN_CAR_LOCK_TOGGLE   39  // apri/chiudi auto
#define BTN_LCD_MODE          40  // cambia pagina LCD secondario
#define BTN_CEILING_LIGHT     43  // toggle luce interna neopixel

// ---------- Interruttore 2 posizioni luci marcia ----------
#define SW_LIGHTS_AUTO_MANUAL 44  // 1 = auto, 0 = manuale

// ---------- Chiusura centralizzata ----------
#define RELAY_LOCK            46
#define RELAY_UNLOCK           47
#define SENSE_LOCK             49  // via optoisolatore, digitale pulito
#define SENSE_UNLOCK            48  // via optoisolatore, digitale pulito
#define SENSE_EXTRA_CONTACT      50  // 3° contatto centralina/telecomando (uso futuro), via optoisolatore

// ---------- Sensore porta ----------
#define PIN_DOOR_SENSE        41 

// ---------- Analogici ----------
#define PIN_BATTERY_VOLTAGE   A1   // partitore tensione batteria
#define PIN_LDR_AMBIENT       A2   // fotoresistenza per anabbaglianti/posizione automatici

// ---------- I2C ----------
#define EEPROM_I2C_ADDR       0x50
#define LCD_I2C_ADDR          0x27
#define LCD_COLS              20
#define LCD_ROWS              4

// ---------- Log (dichiarati qui, non in log_sd.ino, per evitare il
//            classico bug di Arduino: il generatore automatico dei
//            prototipi non vede tipi custom definiti in un altro tab) ----------
enum LogLevel { LOG_INFO, LOG_WARN, LOG_ERROR, LOG_CRITICAL };
void log_setup();
void log_write(LogLevel lvl, const char *tag, const String &msg);
void log_flushBuffer();

#endif
