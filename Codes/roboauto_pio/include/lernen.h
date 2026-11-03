#pragma once
#include <Arduino.h>

// ============================================================
//  Lernen: das Auto schreibt beim Fahren alles mit, was es sieht und tut.
//  Jede Fahrt kommt auf die SD-Karte nach /lernen/fahrt_0001/:
//    daten.csv  - 10x pro Sekunde: Steuerung, Motoren, Gyro, Akku, Lautstaerke, Bildnummer
//    bild_00001.jpg ... - 4x pro Sekunde ein Kamerabild
//  Am PC kann daraus ein Modell lernen: "bei diesem Bild hat der Fahrer so gelenkt".
// ============================================================

// Was das Auto gerade tut (fuellt main.cpp in jedem loop-Durchlauf)
struct LernDaten {
  int x, y;                 // Steuerung -7..7
  int links, rechts;        // Motoren -255..255
  float drehrate, winkel;   // Gyro
  float nicken, rollen;
  int akku;                 // %
  int pegel;                // Mikrofon 0..100
  int modus;
  bool licht;
};

// Aufgabe fuer das Schreiben auf die SD-Karte starten (einmal in setup, nach karteStarten)
void lernenStarten();

// In jedem loop-Durchlauf: aktuelle Werte uebergeben
void lernenDaten(const LernDaten& d);

// Mitschreiben an/aus. Rueckgabe: true = laeuft jetzt
bool lernenUmschalten();
bool lernenAktiv();
int  lernenFahrt();        // Nummer der laufenden (oder letzten) Fahrt
int  lernenBilder();       // so viele Bilder in der laufenden Fahrt
float lernenSekunden();

// Ein Foto auf die SD-Karte (/fotos/foto_0001.jpg). Liefert false ohne Karte/Kamera
bool lernenFoto();
int  lernenLetztesFoto();  // Nummer des zuletzt gespeicherten Fotos (0 = noch keins)
