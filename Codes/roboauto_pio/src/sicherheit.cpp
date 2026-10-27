#include <Arduino.h>
#include "config.h"
#include "sicherheit.h"
#include "gyro.h"
#include "einstellungen.h"

static SicherheitZustand zustand = SICHER_OK;
static unsigned long crashZeit = 0;
static unsigned long schraegSeit = 0;     // seit wann ist das Auto stark schraeg?
static unsigned long geradeSeit = 0;      // seit wann steht es wieder richtig?
static int  stossZaehler = 0;             // Messungen hintereinander ueber der Grenze
static int  letzteL = 0, letzteR = 0;
static unsigned long anfahrZeit = 0;      // letzter grosser Sprung bei den Motorwerten
static float maxStoss = 0;                // fuer die Anzeige im Monitor

SicherheitZustand sicherheitZustand()  { return zustand; }
bool sicherheitFahrenErlaubt()         { return zustand == SICHER_OK; }

int sicherheitUpdate(bool aktiv, bool stickInMitte, int links, int rechts) {
  if (!einst().crash || !gyroBereit()) return 0;
  unsigned long jetzt = millis();

  if (!aktiv) {                            // niemand steuert -> alles zuruecksetzen
    if (zustand != SICHER_OK) { zustand = SICHER_OK; return 3; }
    schraegSeit = 0;
    return 0;
  }

  // Anfahren, Bremsen, Richtungswechsel: grosser Sprung bei den Motorwerten -> kurz nicht pruefen
  if (abs(links - letzteL) > 40 || abs(rechts - letzteR) > 40) anfahrZeit = jetzt;
  letzteL = links; letzteR = rechts;
  bool anfahrPhase = jetzt - anfahrZeit < CRASH_ANFAHR_MS;

  float aufrecht = gyroAufrecht();
  static uint32_t letzteMessung = 0;
  bool neueMessung = gyroMessNummer() != letzteMessung;
  letzteMessung = gyroMessNummer();
  float stoss = gyroStossWaagerecht();

  switch (zustand) {
    case SICHER_OK:
      // Umgekippt: laenger als UMGEKIPPT_MS stark schraeg
      if (aufrecht < UMGEKIPPT_AUFRECHT) {
        if (schraegSeit == 0) schraegSeit = jetzt;
        if (jetzt - schraegSeit > UMGEKIPPT_MS) {
          zustand = SICHER_UMGEKIPPT;
          geradeSeit = 0;
          Serial.printf(">> UMGEKIPPT! (aufrecht=%.2f)\n", aufrecht);
          return 2;
        }
      } else {
        schraegSeit = 0;
      }
      // Aufprall: waagerechter Stoss ueber der Grenze, mehrere Messungen hintereinander
      // (einzelne Vibrations-Spitzen der Motoren reichen nicht), nicht direkt beim Anfahren
      if (neueMessung) {
        if (stoss > maxStoss) maxStoss = stoss;
        if (stoss > einst().crashG && !anfahrPhase) stossZaehler++;
        else                                       stossZaehler = 0;
      }
      if (stossZaehler >= CRASH_MESSUNGEN) {
        stossZaehler = 0;
        zustand = SICHER_CRASH;
        crashZeit = jetzt;
        Serial.printf(">> AUFPRALL erkannt! (%.1f g waagerecht)\n", stoss);
        return 1;
      }
      return 0;

    case SICHER_CRASH:
      // Nach der Sperrzeit weiter, sobald der Stick einmal losgelassen wurde
      if (jetzt - crashZeit > CRASH_SPERRE_MS && stickInMitte) {
        zustand = SICHER_OK;
        Serial.println(">> Nach Aufprall: wieder fahrbereit");
        return 3;
      }
      return 0;

    case SICHER_UMGEKIPPT:
      // Wieder aufgestellt und eine halbe Sekunde ruhig?
      if (aufrecht > 0.8f) {
        if (geradeSeit == 0) geradeSeit = jetzt;
        if (jetzt - geradeSeit > 500 && stickInMitte) {
          zustand = SICHER_OK;
          schraegSeit = 0;
          Serial.println(">> Wieder aufgestellt: fahrbereit");
          return 3;
        }
      } else {
        geradeSeit = 0;
      }
      return 0;
  }
  return 0;
}

// Groesster waagerechter Stoss seit dem letzten Abfragen (fuer den Monitor, zum Einstellen)
float sicherheitMaxStoss() { float m = maxStoss; maxStoss = 0; return m; }
