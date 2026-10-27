#ifndef TESTMODUS_H
#define TESTMODUS_H

#include <Arduino.h>
#include "globals.h"

// Der klassische Regenbogen-Testmodus für den LED-Ring im Stillstand
inline void updateTestmodusLicht(unsigned long currentMillis) {
  static unsigned long letzterSchritt = 0;   // V6: eigener Takt
  static uint16_t farbton = 0;               // V6: läuft bei 65535 von selbst über
  if (currentMillis - letzterSchritt >= 20) {
    letzterSchritt = currentMillis;
    for (int i = 0; i < NUM_LEDS; i++) {
      // Berechnet den kompletten Farbkreis für alle 8 LEDs
      ring.setPixelColor(i, ring.gamma32(ring.ColorHSV(farbton + (i * 65536L / NUM_LEDS))));
    }
    ring.show();
    
    // Dreht den Regenbogen weiter
    farbton += 256; 
  }
}

#endif
