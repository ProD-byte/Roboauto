#pragma once
#include <Arduino.h>
#include "config.h"

// Erkennt Einfach- und Doppelklicks einer Taste.
// pruefe() in jedem loop-Durchlauf aufrufen.
// Rueckgabe: 0 = nichts, 1 = Einfachklick (kommt erst nach DOPPELKLICK_ZEIT), 2 = Doppelklick
struct KlickTaste {
  bool vorher = false;
  bool wartet = false;           // erster Klick erkannt, warte auf evtl. zweiten
  unsigned long ersterKlick = 0;

  int pruefe(bool gedrueckt, unsigned long jetzt) {
    int ergebnis = 0;
    if (gedrueckt && !vorher) {
      if (wartet && jetzt - ersterKlick < DOPPELKLICK_ZEIT) {
        wartet = false;
        ergebnis = 2;
      } else {
        wartet = true;
        ersterKlick = jetzt;
      }
    }
    vorher = gedrueckt;
    if (wartet && jetzt - ersterKlick >= DOPPELKLICK_ZEIT) {
      wartet = false;
      ergebnis = 1;
    }
    return ergebnis;
  }

  void reset() { vorher = false; wartet = false; }
};
