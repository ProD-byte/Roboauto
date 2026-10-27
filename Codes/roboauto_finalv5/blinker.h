#ifndef BLINKER_H
#define BLINKER_H

#include <Arduino.h>
#include "globals.h"

// Kräftiges Blinker-Orange
const uint32_t BLINKER_ORANGE = ring.Color(230, 60, 0);

// Externe Licht-Zustände einbinden, damit der Blinker weiß, was das Fahrlicht macht
extern bool fahrlichtEnabled;
extern bool fernlichtAktiv;

// Hilfsfunktion: Ermittelt, welche Farbe die Scheinwerfer aktuell im Hintergrund haben sollten
inline uint32_t getAktuelleHintergrundLichtFarbe() {
  if (fernlichtAktiv)          return FERNLICHT_WEISS;
  else if (fahrlichtEnabled)   return GELB_STROM_SPAR;
  else                         return LICHT_AUS;
}

// Hilfsfunktion: Ermittelt, welche Farbe das Rücklicht aktuell im Hintergrund haben sollte
inline uint32_t getAktuellesRuecklichtFarbe() {
  if (fahrlichtEnabled || fernlichtAktiv) return ROT_GEDIMMT;
  else                                    return LICHT_AUS;
}

// Logik für den linken Blinker (Viereck) - Steuert NUR die Scheinwerfer-Ecken!
inline void blinkenLinks(bool blinkerStatus) {
  uint32_t normalLicht = getAktuelleHintergrundLichtFarbe();
  uint32_t normalRueck = getAktuellesRuecklichtFarbe();

  if (blinkerStatus) {
    frontLights.setPixelColor(0, BLINKER_ORANGE);  // Vorne Links AN
    frontLights.setPixelColor(1, normalLicht);     // Vorne Rechts (aktueller Fahrlicht-Zustand)
    rearLights.setPixelColor(0, BLINKER_ORANGE);   // Hinten Links AN
    rearLights.setPixelColor(1, normalRueck);      // Hinten Rechts (aktueller Rücklicht-Zustand)
  } else {
    frontLights.setPixelColor(0, normalLicht);     // Vorne Links zurück auf Fahrlicht-Zustand
    frontLights.setPixelColor(1, normalLicht);
    rearLights.setPixelColor(0, normalRueck);      // Hinten Links zurück auf Rücklicht-Zustand
    rearLights.setPixelColor(1, normalRueck);
  }
  frontLights.show();
  rearLights.show();
}

// Logik für den rechten Blinker (Kreis) - Steuert NUR die Scheinwerfer-Ecken!
inline void blinkenRechts(bool blinkerStatus) {
  uint32_t normalLicht = getAktuelleHintergrundLichtFarbe();
  uint32_t normalRueck = getAktuellesRuecklichtFarbe();

  if (blinkerStatus) {
    frontLights.setPixelColor(0, normalLicht);     // Vorne Links (aktueller Fahrlicht-Zustand)
    frontLights.setPixelColor(1, BLINKER_ORANGE);  // Vorne Rechts AN
    rearLights.setPixelColor(0, normalRueck);      // Hinten Links (aktueller Rücklicht-Zustand)
    rearLights.setPixelColor(1, BLINKER_ORANGE);   // Hinten Rechts AN
  } else {
    frontLights.setPixelColor(0, normalLicht);
    frontLights.setPixelColor(1, normalLicht);     // Vorne Rechts zurück auf Fahrlicht-Zustand
    rearLights.setPixelColor(0, normalRueck);
    rearLights.setPixelColor(1, normalRueck);      // Hinten Rechts zurück auf Rücklicht-Zustand
  }
  frontLights.show();
  rearLights.show();
}

// Logik für das Warnblinklicht - Alle 4 Ecken blinken synchron!
inline void warnblinken(bool blinkerStatus) {
  if (blinkerStatus) {
    frontLights.setPixelColor(0, BLINKER_ORANGE);  // Vorne Links AN
    frontLights.setPixelColor(1, BLINKER_ORANGE);  // Vorne Rechts AN
    rearLights.setPixelColor(0, BLINKER_ORANGE);   // Hinten Links AN
    rearLights.setPixelColor(1, BLINKER_ORANGE);   // Hinten Rechts AN
  } else {
    // Wenn der Warnblinker in der "AUS"-Phase ist, stellen wir den echten Lichtzustand wieder her
    uint32_t normalLicht = getAktuelleHintergrundLichtFarbe();
    uint32_t normalRueck = getAktuellesRuecklichtFarbe();

    frontLights.setPixelColor(0, normalLicht);
    frontLights.setPixelColor(1, normalLicht);
    rearLights.setPixelColor(0, normalRueck);
    rearLights.setPixelColor(1, normalRueck);
  }
  frontLights.show();
  rearLights.show();
}

#endif
