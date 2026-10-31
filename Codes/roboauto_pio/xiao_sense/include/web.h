#pragma once

// WLAN + Webseite starten (einmal in setup).
// Ohne gespeicherte Zugangsdaten (oder wenn das WLAN nicht erreichbar ist) oeffnet das Auto
// sein eigenes WLAN "Roboauto-Setup" mit einer Einrichtungsseite fuers Handy.
void webStarten();

// In jedem loop-Durchlauf aufrufen (WLAN-Verbindung pruefen, Anfragen beantworten)
void webUpdate();

// true, solange die Webseite im Browser offen ist (fragt regelmaessig den Status ab)
bool webVerbunden();

// Meldet einmalig, wie der WLAN-Start ausging: 0 = nichts Neues,
// 1 = gerade erfolgreich verbunden, 2 = nach WLAN_TIMEOUT_MS nicht verbunden,
// 3 = Einrichtungs-WLAN geoeffnet (noch keine Zugangsdaten)
int webEreignis();

// true, solange das Einrichtungs-WLAN offen ist
bool webEinrichtung();

// true, solange das Auto mit einem WLAN (mit Internet) verbunden ist
bool webWlanDa();

// Gespeicherte WLAN-Daten loeschen und im Einrichtungsmodus neu starten
void webWlanVergessen();

// true, solange ueber die Webseite gefahren wird (Taste/Joystick gerade aktiv)
bool webFaehrt();
int  webX();   // -7..7 (rechts +)
int  webY();   // -7..7 (vor +)

// Gehaltene Tasten auf der Webseite (Bits, 0 wenn nichts gedrueckt oder Seite weg)
constexpr int WEB_BLINK_L = 1, WEB_BLINK_R = 2, WEB_HUPE = 4, WEB_TANK = 8;
int webTasten();

// Einmal-Aktionen von der Webseite (Licht, Fernlicht, Warnblinker, Wende).
// Liefert die naechste offene Aktion einmalig, sonst 0.
constexpr int WEB_AKTION_LICHT = 1, WEB_AKTION_FERNLICHT = 2, WEB_AKTION_WARN = 3, WEB_AKTION_WENDE = 4,
              WEB_AKTION_MODUS_WEITER = 5;
// Aktion 10..15 = Modus direkt waehlen (10 + Modus-Nummer)
constexpr int WEB_AKTION_MODUS_DIREKT = 10;
// Aktion 20..22 = Figur fahren (20 + Figur-Nummer)
constexpr int WEB_AKTION_FIGUR = 20;
constexpr int WEB_AKTION_AUFNAHME = 30, WEB_AKTION_ABSPIELEN = 31, WEB_AKTION_BALANCE = 32,
              WEB_AKTION_KALIBRIEREN = 33, WEB_AKTION_MOTORTEST = 34;   // 34 = links, 35 = rechts
constexpr int WEB_AKTION_MUSIK_STOPP = 40, WEB_AKTION_MUSIK = 41;          // 41 + Liednummer
// XIAO Sense: Lernen an/aus, Foto auf die SD-Karte, Klatschsteuerung an/aus
constexpr int WEB_AKTION_LERNEN = 70, WEB_AKTION_FOTO = 71, WEB_AKTION_KLATSCHEN = 72;

// Modus fuer die Statusanzeige auf der Webseite mitteilen (siehe enum Modus in config.h)
void webSetzeModus(int modus);
// Klatschsteuerung an/aus fuer die Anzeige auf der Webseite
void webSetzeKlatschen(bool an);
int webAktion();
