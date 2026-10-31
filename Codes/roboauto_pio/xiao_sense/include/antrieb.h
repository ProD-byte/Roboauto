#pragma once

// Motoren vorbereiten (einmal in setup aufrufen)
void antriebStarten();

// Beide Motoren sofort aus
void antriebStopp();

// Joystickwerte (-7..+7) in Motorbewegung umsetzen
void antriebFahren(int x, int y);

// Zuletzt gesetzte Werte (-255..+255), z.B. fuer Debug oder Lichter
int antriebLinks();
int antriebRechts();

// Aktuelle Gyro-Korrektur beim Geradeausfahren (0 = keine), fuer Debug
int antriebKorrektur();

// 180-Grad-Wende links herum starten (braucht das Gyro). false = nicht moeglich
bool antriebWendeStarten();

// true, solange die Wende laeuft. Joystick bewegen bricht sie ab.
bool antriebWendeAktiv();

// Motoren direkt setzen (-255..255), ohne Joystick-Logik - fuer Figuren und Aufnahmen
void antriebDirekt(int links, int rechts);

// Motortest fuer den Assistenten: eine Seite kurz vorwaerts (0 = links, 1 = rechts)
void antriebTestStarten(int seite);
int  antriebTestSeite();                 // -1 = kein Test, sonst 0/1
void antriebTestUpdate();                // im loop statt antriebFahren, solange der Test laeuft
