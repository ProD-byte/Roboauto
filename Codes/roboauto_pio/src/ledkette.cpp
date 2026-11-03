#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "ledkette.h"

// Die Ketten selbst laufen immer mit voller Helligkeit - jeder Bereich wird beim Setzen
// mit seiner eigenen Helligkeit verrechnet (sonst gaebe es nur einen Regler fuer alles).
#ifdef ROBOAUTO_XIAO
// XIAO: alle LEDs hintereinander an einem Pin
static Adafruit_NeoPixel ketten[] = {
  Adafruit_NeoPixel(LED_KETTE_ANZAHL, LED_KETTE_PIN, NEO_GRB + NEO_KHZ800),
};
static int kettenNr(int& nr) { return 0; }
#else
// Super Mini: Ring, Front- und Ruecklicht an drei eigenen Pins - nach aussen trotzdem eine Kette
static Adafruit_NeoPixel ketten[] = {
  Adafruit_NeoPixel(LED_RING_ANZAHL,  LED_RING_PIN,  NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(FRONT_LED_ANZAHL, FRONT_LED_PIN, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(REAR_LED_ANZAHL,  REAR_LED_PIN,  NEO_GRB + NEO_KHZ800),
};
static int kettenNr(int& nr) {   // Platz in der Kette -> welcher Pin, nr wird zur LED-Nummer dort
  if (nr >= LED_KETTE_REAR)  { nr -= LED_KETTE_REAR;  return 2; }
  if (nr >= LED_KETTE_FRONT) { nr -= LED_KETTE_FRONT; return 1; }
  nr -= LED_KETTE_RING; return 0;
}
#endif
constexpr int KETTEN = sizeof(ketten) / sizeof(ketten[0]);
static bool geaendert[KETTEN] = {};
static unsigned long letzteAusgabe = 0;

void ketteStarten() {
  for (auto& k : ketten) {
    k.begin();
    k.setBrightness(255);
    k.clear();
    k.show();
  }
}

void ketteSetze(int nr, uint8_t r, uint8_t g, uint8_t b, uint8_t hell) {
  if (nr < 0 || nr >= LED_KETTE_ANZAHL) return;
  int k = kettenNr(nr);
  uint16_t h = hell + 1;                                  // wie Adafruit setBrightness
  uint32_t c = Adafruit_NeoPixel::Color((r * h) >> 8, (g * h) >> 8, (b * h) >> 8);
  if (ketten[k].getPixelColor(nr) != c) {
    ketten[k].setPixelColor(nr, c);
    geaendert[k] = true;
  }
}

void ketteZeigen(bool sofort) {
  unsigned long jetzt = millis();
  if (!sofort && jetzt - letzteAusgabe < 5) return;      // 12 LEDs brauchen nur ca. 0,4 ms
  bool gezeigt = false;
  for (int k = 0; k < KETTEN; k++) {
    if (!geaendert[k]) continue;
    geaendert[k] = false;
    ketten[k].show();
    gezeigt = true;
  }
  if (gezeigt) letzteAusgabe = jetzt;
}
