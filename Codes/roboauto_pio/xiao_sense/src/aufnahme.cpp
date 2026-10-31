#include <Arduino.h>
#include <Preferences.h>
#include <SD.h>
#include "config.h"
#include "aufnahme.h"
#include "buzzer.h"
#include "karte.h"

// Mit SD-Karte liegt der Fahrweg in /aufnahme.bin (bis 10 min), sonst im internen Speicher (bis 2 min)
static const char* AUFNAHME_DATEI = "/aufnahme.bin";

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

static int maxEintraege()        { return karteDa() ? AUFNAHME_MAX_EINTRAEGE : AUFNAHME_MAX_INTERN; }
static unsigned long maxDauer()  { return karteDa() ? AUFNAHME_MAX_MS : AUFNAHME_MAX_MS_INTERN; }

static void speichern() {
  if (karteDa()) {
    File f = SD.open(AUFNAHME_DATEI, FILE_WRITE);
    if (f) { f.write((const uint8_t*)spur, anzahl * sizeof(Eintrag)); f.close(); return; }
    Serial.println(">> Aufnahme: SD-Karte geht nicht - speichere intern");
  }
  Preferences p;
  p.begin("aufnahme", false);
  int n = min(anzahl, AUFNAHME_MAX_INTERN);
  p.putInt("anzahl", n);
  p.putBytes("spur", spur, n * sizeof(Eintrag));
  p.end();
}

void aufnahmeStarten() {
  anzahl = 0;
  File f = karteDa() ? SD.open(AUFNAHME_DATEI) : File();
  if (f) {
    anzahl = min((int)(f.size() / sizeof(Eintrag)), AUFNAHME_MAX_EINTRAEGE);
    f.read((uint8_t*)spur, anzahl * sizeof(Eintrag));
    f.close();
  } else {
    Preferences p;
    p.begin("aufnahme", true);
    anzahl = constrain(p.getInt("anzahl", 0), 0, AUFNAHME_MAX_INTERN);
    if (anzahl > 0) p.getBytes("spur", spur, anzahl * sizeof(Eintrag));
    p.end();
  }
  gesamtMs = 0;
  for (int i = 0; i < anzahl; i++) gesamtMs += spur[i].dauer;
  if (anzahl > 0) Serial.printf(">> Gespeicherte Aufnahme geladen: %.1f s\n", gesamtMs / 1000.0f);
}

// Laufenden Abschnitt abschliessen und in die Liste schreiben
static void abschnittSichern(unsigned long jetzt) {
  unsigned long dauer = jetzt - abschnittStart;
  while (dauer > 0 && anzahl < maxEintraege()) {
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
    if (anzahl >= maxEintraege() || jetzt - abschnittStart + gesamtMs > maxDauer()) {
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
