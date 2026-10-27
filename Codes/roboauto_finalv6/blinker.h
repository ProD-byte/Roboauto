#ifndef BLINKER_H
#define BLINKER_H

#include <Arduino.h>
#include "globals.h"

// Kräftiges Blinker-Orange (= ring.Color(230, 60, 0), jetzt als feste Konstante)
const uint32_t BLINKER_ORANGE = 0xE63C00;

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

// Gemeinsame Logik für alle Blinker: links/rechts gibt an, welche Seite orange wird
inline void setzeBlinker(bool links, bool rechts, bool blinkerStatus) {
  uint32_t normalLicht = getAktuelleHintergrundLichtFarbe();
  uint32_t normalRueck = getAktuellesRuecklichtFarbe();

  bool linksAn  = blinkerStatus && links;
  bool rechtsAn = blinkerStatus && rechts;

  frontLights.setPixelColor(0, linksAn  ? BLINKER_ORANGE : normalLicht);  // Vorne Links
  frontLights.setPixelColor(1, rechtsAn ? BLINKER_ORANGE : normalLicht);  // Vorne Rechts
  rearLights.setPixelColor(0,  linksAn  ? BLINKER_ORANGE : normalRueck);  // Hinten Links
  rearLights.setPixelColor(1,  rechtsAn ? BLINKER_ORANGE : normalRueck);  // Hinten Rechts
  frontLights.show();
  rearLights.show();
}

// Linker Blinker (Viereck)
inline void blinkenLinks(bool blinkerStatus)  { setzeBlinker(true,  false, blinkerStatus); }

// Rechter Blinker (Kreis)
inline void blinkenRechts(bool blinkerStatus) { setzeBlinker(false, true,  blinkerStatus); }

// Warnblinklicht (Doppelklick Kreis) - Alle 4 Ecken blinken synchron!
inline void warnblinken(bool blinkerStatus)   { setzeBlinker(true,  true,  blinkerStatus); }

#endif
