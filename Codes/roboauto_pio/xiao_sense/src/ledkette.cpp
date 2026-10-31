#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "ledkette.h"

// Die Kette selbst laeuft immer mit voller Helligkeit - jeder Bereich wird beim Setzen
// mit seiner eigenen Helligkeit verrechnet (sonst gaebe es nur einen Regler fuer alles).
static Adafruit_NeoPixel kette(LED_KETTE_ANZAHL, LED_KETTE_PIN, NEO_GRB + NEO_KHZ800);
static bool geaendert = false;
static unsigned long letzteAusgabe = 0;

void ketteStarten() {
  kette.begin();
  kette.setBrightness(255);
  kette.clear();
  kette.show();
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
  if (!geaendert) return;
  unsigned long jetzt = millis();
  if (!sofort && jetzt - letzteAusgabe < 5) return;      // 12 LEDs brauchen nur ca. 0,4 ms
  letzteAusgabe = jetzt;
  geaendert = false;
  kette.show();
}
