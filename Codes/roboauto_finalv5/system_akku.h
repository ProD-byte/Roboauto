#ifndef SYSTEM_AKKU_H
#define SYSTEM_AKKU_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "globals.h"

// Das Onboard-LED-Objekt aus dem Hauptsketch bekannt machen
extern Adafruit_NeoPixel onboardLed;

// Die ausgelagerte, intelligente Onboard-Tankanzeige
inline void updateOnboardTankanzeige() {
  int akkuWert = analogRead(BATTERIE_MESS_PIN);
  
  if (akkuWert > 2700) { 
    onboardLed.setPixelColor(0, onboardLed.Color(0, 0, 255));  // Blau = Voll/Lädt
  } 
  else if (akkuWert > 2000) { 
    onboardLed.setPixelColor(0, onboardLed.Color(0, 255, 0));  // Grün = Gesund
  } 
  else if (akkuWert > 1850) { 
    onboardLed.setPixelColor(0, onboardLed.Color(150, 100, 0)); // Gelb = Warnung
  } 
  else {
    // Rot blinken bei kritischem Akkustand
    if (millis() % 200 < 100) {
      onboardLed.setPixelColor(0, onboardLed.Color(255, 0, 0)); // Rot
    } else {
      onboardLed.setPixelColor(0, onboardLed.Color(0, 0, 0));   // Aus
    }
  }
  onboardLed.show();
}

#endif
