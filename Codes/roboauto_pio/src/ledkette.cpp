#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <driver/rmt_tx.h>
#include <driver/rmt_encoder.h>
#include "config.h"
#include "ledkette.h"

// Alle LEDs hintereinander an einem Pin. Die Kette selbst laeuft immer mit voller Helligkeit -
// jeder Bereich wird beim Setzen mit seiner eigenen Helligkeit verrechnet
// (sonst gaebe es nur einen Regler fuer alles).
static Adafruit_NeoPixel kette(LED_KETTE_ANZAHL, LED_KETTE_PIN, NEO_GRB + NEO_KHZ800);
static bool geaendert = false;
static unsigned long letzteAusgabe = 0;

// Senden per RMT mit DMA statt ueber Adafruit show(): die Bibliothek fuellt den kleinen RMT-Speicher
// waehrend des Sendens per Interrupt nach. Sind Kamera/WLAN gerade beschaeftigt, kommt das zu spaet -
// die Luecke wirkt wie "Ende der Daten", und nur die erste LED uebernimmt ihre Farbe.
// Mit DMA liegt das ganze Bild vorher im Speicher und geht ohne Pause raus.
static rmt_channel_handle_t rmtKanal = nullptr;
static rmt_encoder_handle_t rmtKodierer = nullptr;

static bool dmaStarten() {
  rmt_tx_channel_config_t k = {};
  k.gpio_num = (gpio_num_t)LED_KETTE_PIN;
  k.clk_src = RMT_CLK_SRC_DEFAULT;
  k.resolution_hz = 10000000;                 // 0,1 us pro Takt
  k.mem_block_symbols = 512;                  // mit DMA: Puffer fuer ein ganzes Bild (12 LEDs = 288 Bits)
  k.trans_queue_depth = 2;
  k.flags.with_dma = 1;
  if (rmt_new_tx_channel(&k, &rmtKanal) != ESP_OK) { rmtKanal = nullptr; return false; }
  rmt_bytes_encoder_config_t b = {};
  b.bit0 = {3, 1, 9, 0};                      // 0: 0,3 us an, 0,9 us aus
  b.bit1 = {9, 1, 3, 0};                      // 1: 0,9 us an, 0,3 us aus
  b.flags.msb_first = 1;
  if (rmt_new_bytes_encoder(&b, &rmtKodierer) != ESP_OK || rmt_enable(rmtKanal) != ESP_OK) {
    rmt_del_channel(rmtKanal); rmtKanal = nullptr; return false;
  }
  return true;
}

static void ausgeben() {
  if (!rmtKanal) { kette.show(); return; }    // DMA ging nicht -> ueber die Bibliothek
  rmt_transmit_config_t t = {};
  rmt_tx_wait_all_done(rmtKanal, 10);         // letztes Bild fertig (+ Pause = Uebernahme)
  rmt_transmit(rmtKanal, rmtKodierer, kette.getPixels(), kette.numPixels() * 3, &t);
}

void ketteStarten() {
  bool dma = dmaStarten();
  Serial.printf(">> LED-Kette: %s\n", dma ? "Ausgabe per DMA" : "DMA geht nicht - Ausgabe ueber die Bibliothek");
  if (!rmtKanal) kette.begin();               // mit DMA gehoert der Pin dem eigenen RMT-Kanal
  kette.setBrightness(255);
  kette.clear();
  ausgeben();
}

void ketteSetze(int nr, uint8_t r, uint8_t g, uint8_t b, uint8_t hell) {
  if (nr < 0 || nr >= LED_KETTE_ANZAHL) return;
  uint16_t h = hell + 1;                                  // wie Adafruit setBrightness
  uint32_t c = Adafruit_NeoPixel::Color((r * h) >> 8, (g * h) >> 8, (b * h) >> 8);
  if (kette.getPixelColor(nr) != c) {
    kette.setPixelColor(nr, c);
    geaendert = true;
  }
}

void ketteZeigen(bool sofort) {
  unsigned long jetzt = millis();
  if (!sofort && jetzt - letzteAusgabe < 5) return;      // 12 LEDs brauchen nur ca. 0,4 ms
  if (!geaendert) return;
  geaendert = false;
  ausgeben();
  letzteAusgabe = jetzt;
}
