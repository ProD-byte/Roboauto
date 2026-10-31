#pragma once
#include <Arduino.h>

// ============================================================
//  Einstellungen, die man auf der Webseite aendern kann.
//  Sie werden im Speicher des ESP32 gesichert (bleiben nach Ausschalten und Updates erhalten).
//  Die Standardwerte kommen aus config.h.
// ============================================================
enum Toene { TOENE_STUMM = 0, TOENE_WICHTIG = 1, TOENE_ALLE = 2 };

struct Einstellungen {
  int  tempo;            // Hoechstgeschwindigkeit (PWM 40..255)
  int  lenkung;          // Lenkstaerke waehrend der Fahrt
  int  lenkungStand;     // Lenkstaerke beim Drehen auf der Stelle
  bool geradeaus;        // Gyro-Geradeauslauf
  bool crash;            // Crash-/Umkipp-Erkennung
  float crashG;          // Empfindlichkeit (g)
  int  ringHell;         // Helligkeit LED-Ring (5..255)
  int  lichtHell;        // Helligkeit Front-/Ruecklichter (5..255)
  int  toene;            // siehe enum Toene
  int  figurGroesse;     // Figuren in Prozent (50..200)
  bool motorLInv;        // linke Seite dreht falsch herum -> umkehren
  bool motorRInv;        // rechte Seite dreht falsch herum -> umkehren
  bool autoUpdate;
  bool disco;            // Lichter blitzen bei Musik im Takt       // automatisch neue Versionen von GitHub installieren
  int  lautstaerke;      // Buzzer-Lautstaerke in Prozent (5..100)
  bool motorTausch;      // Motoren an den falschen Ausgaengen -> links/rechts tauschen
};

void einstellungenLaden();                   // einmal in setup (vor allem anderen)
const Einstellungen& einst();                // aktuelle Werte lesen

// Einen Wert setzen (Name wie im JSON). Rueckgabe false = unbekannter Name.
bool einstellungSetzen(const String& name, const String& wert);
void einstellungenSpeichern();
void einstellungenStandard();                // alles ausser Motoren auf Standard (und speichern)
void einstellungenMotorStandard();           // nur Motor-Umkehr/Tausch auf Standard (und speichern)
String einstellungenJson();

// Wird aufgerufen, wenn sich eine Helligkeit aendert (setzen die Module selbst)
void ringHelligkeit(int h);
void lichtHelligkeit(int h);
