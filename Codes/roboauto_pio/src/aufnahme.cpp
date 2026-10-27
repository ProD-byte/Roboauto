#include <Arduino.h>
#include <Preferences.h>
#include "config.h"
#include "aufnahme.h"
#include "buzzer.h"

// Ein Eintrag = "so lange (dauer) diese Steuerwerte (x, y)"
struct Eintrag {
  uint16_t dauer;   // ms
  int8_t   x, y;    // -7..7
};

static Eintrag spur[AUFNAHME_MAX_EINTRAEGE];
static int anzahl = 0;
static uint32_t gesamtMs = 0;

static bool nimmtAuf = false;
static bool spielt = false;

// Aufnahme
static bool begonnen = false;           // erst ab der ersten Bewegung mitschreiben
static int8_t letzteX = 0, letzteY = 0;
static unsigned long abschnittStart = 0;

// Abspielen
static int abspielPos = 0;
static unsigned long eintragStart = 0;
static unsigned long abspielStart = 0;

bool  aufnahmeLaeuft()    { return nimmtAuf; }
bool  abspielenLaeuft()   { return spielt; }
bool  aufnahmeVorhanden() { return anzahl > 0; }
float aufnahmeSekunden()  { return gesamtMs / 1000.0f; }
float abspielenSekunden() { return spielt ? (millis() - abspielStart) / 1000.0f : 0; }

static void speichern() {
  Preferences p;
  p.begin("aufnahme", false);
  p.putInt("anzahl", anzahl);
  p.putBytes("spur", spur, anzahl * sizeof(Eintrag));
  p.end();
}

void aufnahmeStarten() {
  Preferences p;
  p.begin("aufnahme", true);
  anzahl = constrain(p.getInt("anzahl", 0), 0, AUFNAHME_MAX_EINTRAEGE);
  if (anzahl > 0) p.getBytes("spur", spur, anzahl * sizeof(Eintrag));
  p.end();
  gesamtMs = 0;
  for (int i = 0; i < anzahl; i++) gesamtMs += spur[i].dauer;
  if (anzahl > 0) Serial.printf(">> Gespeicherte Aufnahme geladen: %.1f s\n", gesamtMs / 1000.0f);
}

// Laufenden Abschnitt abschliessen und in die Liste schreiben
static void abschnittSichern(unsigned long jetzt) {
  unsigned long dauer = jetzt - abschnittStart;
  while (dauer > 0 && anzahl < AUFNAHME_MAX_EINTRAEGE) {
    uint16_t teil = min(dauer, 60000UL);          // sehr lange Abschnitte aufteilen
    spur[anzahl++] = { teil, letzteX, letzteY };
    gesamtMs += teil;
    dauer -= teil;
  }
  abschnittStart = jetzt;
}

static void aufnahmeBeenden() {
  if (!nimmtAuf) return;
  nimmtAuf = false;
  if (begonnen) abschnittSichern(millis());
  // Stillstand am Ende weglassen
  while (anzahl > 0 && spur[anzahl - 1].x == 0 && spur[anzahl - 1].y == 0) {
    gesamtMs -= spur[anzahl - 1].dauer;
    anzahl--;
  }
  speichern();
  Serial.printf(">> AUFNAHME gespeichert: %.1f s (%d Abschnitte)\n", gesamtMs / 1000.0f, anzahl);
  buzzerAus();
}

void aufnahmeUmschalten() {
  if (nimmtAuf) { aufnahmeBeenden(); return; }
  if (spielt) spielt = false;
  nimmtAuf = true;
  begonnen = false;
  anzahl = 0;
  gesamtMs = 0;
  Serial.println(">> AUFNAHME laeuft - jetzt losfahren");
  buzzerEin();
}

static void abspielenBeenden(const char* grund) {
  if (!spielt) return;
  spielt = false;
  Serial.printf(">> ABSPIELEN beendet (%s)\n", grund);
}

void abspielenUmschalten() {
  if (spielt) { abspielenBeenden("gestoppt"); return; }
  if (nimmtAuf) aufnahmeBeenden();
  if (anzahl == 0) {
    Serial.println(">> Keine Aufnahme vorhanden");
    buzzerWarnung();
    return;
  }
  spielt = true;
  abspielPos = 0;
  eintragStart = abspielStart = millis();
  Serial.printf(">> ABSPIELEN: %.1f s\n", gesamtMs / 1000.0f);
  buzzerEin();
}

void aufnahmeUpdate(int& x, int& y) {
  unsigned long jetzt = millis();

  if (nimmtAuf) {
    bool bewegt = abs(x) > TOTZONE || abs(y) > TOTZONE;
    if (!begonnen) {
      if (!bewegt) return;                      // Warten am Anfang nicht mitschreiben
      begonnen = true;
      abschnittStart = jetzt;
      letzteX = x; letzteY = y;
    }
    if (x != letzteX || y != letzteY) {         // Steuerwert hat sich geaendert
      abschnittSichern(jetzt);
      letzteX = x; letzteY = y;
    }
    if (anzahl >= AUFNAHME_MAX_EINTRAEGE || jetzt - abschnittStart + gesamtMs > AUFNAHME_MAX_MS) {
      Serial.println(">> Aufnahme voll");
      aufnahmeBeenden();
    }
    return;
  }

  if (spielt) {
    // Fahrer greift ein -> abbrechen, Fahrer hat Vorrang
    if (abs(x) > TOTZONE || abs(y) > TOTZONE) { abspielenBeenden("Joystick bewegt"); return; }

    while (abspielPos < anzahl && jetzt - eintragStart >= spur[abspielPos].dauer) {
      eintragStart += spur[abspielPos].dauer;
      abspielPos++;
    }
    if (abspielPos >= anzahl) {
      abspielenBeenden("fertig");
      buzzerAus();
      x = 0; y = 0;
      return;
    }
    x = spur[abspielPos].x;
    y = spur[abspielPos].y;
  }
}
