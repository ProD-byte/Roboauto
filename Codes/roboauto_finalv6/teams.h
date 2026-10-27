#ifndef TEAMS_H
#define TEAMS_H

#include <Arduino.h>
#include "globals.h"

// V6: Jeder Modus hat jetzt seinen eigenen Takt und Zähler
// (vorher teilten sich alle Effekte previousMillis/rainbowCycle mit unterschiedlichen Wertebereichen)

// Modus 3: Team BLAU (Drehender Fade rein in Blautönen)
inline void updateTeamBlau(unsigned long currentMillis) {
  static unsigned long letzterSchritt = 0;
  static int winkel = 0;
  if (currentMillis - letzterSchritt >= 25) {
    letzterSchritt = currentMillis;
    for (int i = 0; i < NUM_LEDS; i++) {
      uint16_t hueBlau = 40000 + (sin((winkel + (i * 360 / NUM_LEDS)) * M_PI / 180.0) * 4000);
      ring.setPixelColor(i, ring.gamma32(ring.ColorHSV(hueBlau, 255, 200)));
    }
    ring.show();
    winkel += 2; if (winkel >= 360) winkel = 0;
  }
}

// Modus 4: Team GRÜN (Drehender Fade rein in Grüntönen)
inline void updateTeamGruen(unsigned long currentMillis) {
  static unsigned long letzterSchritt = 0;
  static int winkel = 0;
  if (currentMillis - letzterSchritt >= 20) { 
    letzterSchritt = currentMillis;
    for (int i = 0; i < NUM_LEDS; i++) {
      uint16_t hueGruen = 21000 + (sin((winkel + (i * 360 / NUM_LEDS)) * M_PI / 180.0) * 7000);
      ring.setPixelColor(i, ring.gamma32(ring.ColorHSV(hueGruen, 255, 220)));
    }
    ring.show();
    winkel += 3; if (winkel >= 360) winkel = 0;
  }
}

#endif
