#ifndef TESTMODUS_H
#define TESTMODUS_H

#include <Arduino.h>
#include "globals.h"

// Hilfsvariable für die Regenbogen-Drehung (aus dem Hauptsketch übernommen)
extern long rainbowCycle;

// Der klassische Regenbogen-Testmodus für den LED-Ring im Stillstand
inline void updateTestmodusLicht(unsigned long currentMillis) {
  if (currentMillis - previousMillis >= 20) {
    previousMillis = currentMillis;
    for (int i = 0; i < NUM_LEDS; i++) {
      // Berechnet den kompletten Farbkreis für alle 8 LEDs
      ring.setPixelColor(i, ring.gamma32(ring.ColorHSV(rainbowCycle + (i * 65536 / NUM_LEDS))));
    }
    ring.show();
    
    // Dreht den Regenbogen weiter
    rainbowCycle += 256; 
    if (rainbowCycle >= 5 * 65536) rainbowCycle = 0; 
  }
}

#endif
