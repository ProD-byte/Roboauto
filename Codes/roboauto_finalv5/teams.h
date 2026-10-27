#ifndef TEAMS_H
#define TEAMS_H

#include <Arduino.h>
#include "globals.h"

// Hilfsvariable für den Takt
extern long rainbowCycle;

// Modus 3: Team BLAU (Drehender Fade rein in Blautönen)
inline void updateTeamBlau(unsigned long currentMillis) {
  if (currentMillis - previousMillis >= 25) {
    previousMillis = currentMillis;
    for (int i = 0; i < NUM_LEDS; i++) {
      uint16_t hueBlau = 40000 + (sin((rainbowCycle + (i * 360 / NUM_LEDS)) * M_PI / 180.0) * 4000);
      ring.setPixelColor(i, ring.gamma32(ring.ColorHSV(hueBlau, 255, 200)));
    }
    ring.show();
    rainbowCycle += 2; if (rainbowCycle >= 360) rainbowCycle = 0;
  }
}

// Modus 4: Team GRÜN (Drehender Fade rein in Grüntönen)
inline void updateTeamGruen(unsigned long currentMillis) {
  if (currentMillis - previousMillis >= 20) { 
    previousMillis = currentMillis;
    for (int i = 0; i < NUM_LEDS; i++) {
      uint16_t hueGruen = 21000 + (sin((rainbowCycle + (i * 360 / NUM_LEDS)) * M_PI / 180.0) * 7000);
      ring.setPixelColor(i, ring.gamma32(ring.ColorHSV(hueGruen, 255, 220)));
    }
    ring.show();
    rainbowCycle += 3; if (rainbowCycle >= 360) rainbowCycle = 0;
  }
}

#endif
