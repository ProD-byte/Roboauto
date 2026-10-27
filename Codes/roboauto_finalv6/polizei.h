#ifndef POLIZEI_H
#define POLIZEI_H

#include <Arduino.h>
#include "globals.h"

static int policeStage = 0; 
static int yelpFrequenz = 600;       
static bool yelpRichtungHoch = true; 

inline void updatePolizeiLicht(unsigned long currentMillis) {
  static unsigned long letzterSchritt = 0;   // V6: eigener Takt statt globalem previousMillis
  if (currentMillis - letzterSchritt >= 50) {
    letzterSchritt = currentMillis;
    clearRingNoShow();
    policeStage = (policeStage + 1) % 12;
    
    if (policeStage == 0 || policeStage == 2 || policeStage == 4) {
      ring.setPixelColor(5, ring.Color(0, 0, 255)); 
      ring.setPixelColor(6, ring.Color(0, 0, 255)); 
      ring.setPixelColor(7, ring.Color(0, 0, 255));
    } 
    else if (policeStage == 6 || policeStage == 8 || policeStage == 10) {
      ring.setPixelColor(1, ring.Color(255, 0, 0)); 
      ring.setPixelColor(2, ring.Color(255, 0, 0)); 
      ring.setPixelColor(3, ring.Color(255, 0, 0));
    }
    ring.show();
  }
}

inline void spielePolizeiSirene() {
  if (yelpRichtungHoch) {
    yelpFrequenz += 25; 
    if (yelpFrequenz >= 1400) yelpRichtungHoch = false;
  } else {
    yelpFrequenz -= 25; 
    if (yelpFrequenz <= 600) yelpRichtungHoch = true;
  }
  tone(BUZZER_PIN, yelpFrequenz);
  delay(2); 
}

#endif
