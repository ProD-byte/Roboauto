#pragma once
#include <Arduino.h>

// Uhrzeit fuer Dateinamen (Fotos/Videos nach Datum und Uhrzeit).
// Das Auto hat keine Uhr mit Batterie: im Heimnetz holt es die Zeit aus dem Internet (NTP),
// ausserdem schickt die Webseite/App beim Oeffnen die Uhrzeit des Handys (/zeit?t=...).
// Zeitzone: Deutschland mit Sommerzeit.

void zeitStarten();                   // einmal in setup (Zeitzone)
void zeitInternet();                  // sobald das Heim-WLAN verbunden ist
void zeitSetzen(uint32_t sekunden);   // Unix-Zeit in Sekunden (UTC), z.B. vom Browser
bool zeitBekannt();                   // false = noch keine Uhrzeit (dann "ohne-datum" als Ordner)

// Ordner und Dateiname fuer jetzt: "2026-11-03" und "15-42-07"
void zeitTag(char* puffer, size_t groesse);
void zeitUhr(char* puffer, size_t groesse);
