#pragma once

// Tasten, die fuer die Lichter wichtig sind
struct LichtTasten {
  bool dreieck;   // 1x = Licht an/aus, 2x = Fernlicht an/aus
  bool viereck;   // gedrueckt halten = Blinker links
  bool kreis;     // gedrueckt halten = Blinker rechts, 2x = Warnblinker an/aus
};

// Front- und Ruecklichter vorbereiten (einmal in setup aufrufen)
void lichtStarten();

// In jedem loop-Durchlauf aufrufen.
//  verbunden    : ohne Verbindung sind alle Lichter aus
//  tasten       : aktueller Tastenzustand
//  links/rechts : aktuelle Motorwerte (-255..255) fuer Brems- und Rueckfahrlicht
//  eco          : Akku fast leer -> Fernlicht aus und gesperrt
// Befehle (z.B. von der Webseite). Werden beim naechsten lichtUpdate ausgefuehrt.
enum LichtBefehl { LICHT_UMSCHALTEN = 1, FERNLICHT_UMSCHALTEN = 2, WARNBLINKER_UMSCHALTEN = 3,
                   WARNBLINKER_AUTO_AN = 4,    // automatisch an (z.B. nach Aufprall)
                   WARNBLINKER_AUTO_AUS = 5 }; // wieder aus - aber nur, wenn er automatisch anging
void lichtBefehl(LichtBefehl befehl);

// Aktueller Zustand (fuer die Anzeige auf der Webseite)
bool lichtIstAn();
bool lichtIstFernlicht();
bool lichtIstWarnblinker();

// Polizei-Blitzer vorne (abwechselnd links/rechts), solange true
void lichtBlitzer(bool an);

// Disco-Lichter zur Musik: an = Musik laeuft, hz = aktueller Ton, note = Notenzaehler, viertel = Taktposition
void lichtDisco(bool an, int hz, int note, float viertel);

void lichtUpdate(bool verbunden, const LichtTasten& tasten, int links, int rechts, bool eco);

// Aktuelle Farbe einer Lampe (0 = vorne links, 1 = vorne rechts, 2 = hinten links, 3 = hinten rechts)
uint32_t lichtAnzeigeFarbe(int n);
