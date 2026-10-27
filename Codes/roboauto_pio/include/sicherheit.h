#pragma once

// Crash- und Umkipp-Erkennung mit dem Gyro/Beschleunigungssensor.
enum SicherheitZustand { SICHER_OK = 0, SICHER_CRASH = 1, SICHER_UMGEKIPPT = 2 };

// In jedem loop-Durchlauf aufrufen.
//  aktiv        : nur pruefen, wenn jemand steuert (sonst loest Herumtragen Alarm aus)
//  stickInMitte : nach einem Aufprall geht es erst weiter, wenn der Stick losgelassen wurde
//  links/rechts : aktuelle Motorwerte - beim Anfahren wird kurz nicht auf Aufprall geprueft
// Rueckgabe: 0 = nichts Neues, 1 = gerade Aufprall erkannt, 2 = gerade umgekippt, 3 = wieder alles ok
int sicherheitUpdate(bool aktiv, bool stickInMitte, int links, int rechts);

SicherheitZustand sicherheitZustand();
bool sicherheitFahrenErlaubt();

// Groesster waagerechter Stoss seit der letzten Abfrage (zum Einstellen von CRASH_STOSS_G)
float sicherheitMaxStoss();
