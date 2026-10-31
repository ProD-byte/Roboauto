#include <Arduino.h>
#include <SD.h>
#include "config.h"
#include "lernen.h"
#include "karte.h"
#include "kamera.h"

// Schreiben auf die SD-Karte dauert mal 5, mal 50 ms - deshalb in einer eigenen Aufgabe auf
// Kern 0, damit Fahren und Lichter (loop auf Kern 1) davon nichts merken.

static LernDaten daten = {};
static portMUX_TYPE sperre = portMUX_INITIALIZER_UNLOCKED;   // daten wird von zwei Aufgaben benutzt

static volatile bool aktiv = false;
static volatile bool startWunsch = false, stoppWunsch = false, fotoWunsch = false;
static volatile int fahrt = 0, bilder = 0, letztesFoto = 0;
static unsigned long fahrtStart = 0;

void lernenDaten(const LernDaten& d) {
  portENTER_CRITICAL(&sperre);
  daten = d;
  portEXIT_CRITICAL(&sperre);
}

bool lernenAktiv()     { return aktiv; }
int  lernenFahrt()     { return fahrt; }
int  lernenBilder()    { return bilder; }
float lernenSekunden() { return aktiv ? (millis() - fahrtStart) / 1000.0f : 0; }
int  lernenLetztesFoto() { return letztesFoto; }

bool lernenUmschalten() {
  if (!karteDa()) { Serial.println(">> Lernen: keine SD-Karte"); return false; }
  if (aktiv) { stoppWunsch = true; return false; }
  startWunsch = true;
  return true;
}

bool lernenFoto() {
  if (!karteDa() || !kameraDa()) return false;
  fotoWunsch = true;
  return true;
}

static bool speichereBild(const char* pfad) {
  camera_fb_t* fb = kameraBild();
  if (!fb) return false;
  File f = SD.open(pfad, FILE_WRITE);
  bool ok = f && f.write(fb->buf, fb->len) == fb->len;
  if (f) f.close();
  kameraBildFrei(fb);
  return ok;
}

static void lernAufgabe(void*) {
  File csv;
  char ordner[32], pfad[64], zeile[160];
  unsigned long letzteZeile = 0, letztesBild = 0, letzterFlush = 0;
  for (;;) {
    unsigned long jetzt = millis();

    if (fotoWunsch) {
      fotoWunsch = false;
      int nr = karteNaechsteNummer("/fotos", "foto");
      snprintf(pfad, sizeof(pfad), "/fotos/foto_%04d.jpg", nr);
      if (speichereBild(pfad)) { letztesFoto = nr; Serial.printf(">> Foto gespeichert: %s\n", pfad); }
      else Serial.println(">> Foto: Speichern fehlgeschlagen");
    }

    if (startWunsch && !aktiv) {
      startWunsch = false;
      fahrt = karteNaechsteNummer("/lernen", "fahrt");
      snprintf(ordner, sizeof(ordner), "/lernen/fahrt_%04d", fahrt);
      SD.mkdir(ordner);
      snprintf(pfad, sizeof(pfad), "%s/daten.csv", ordner);
      csv = SD.open(pfad, FILE_WRITE);
      if (csv) {
        csv.println("zeit_ms,bild,x,y,links,rechts,drehrate,winkel,nicken,rollen,akku,pegel,modus,licht");
        bilder = 0;
        fahrtStart = letzteZeile = letztesBild = letzterFlush = jetzt;
        aktiv = true;
        Serial.printf(">> LERNEN: schreibe mit nach %s\n", ordner);
      } else {
        Serial.println(">> LERNEN: kann nicht auf die SD-Karte schreiben");
      }
    }

    if (aktiv && stoppWunsch) {
      stoppWunsch = false;
      aktiv = false;
      csv.close();
      Serial.printf(">> LERNEN beendet: Fahrt %d, %.0f s, %d Bilder\n", fahrt, (jetzt - fahrtStart) / 1000.0f, bilder);
    }
    stoppWunsch = false;

    if (aktiv) {
      if (kameraDa() && jetzt - letztesBild >= LERNEN_BILD_MS) {
        letztesBild = jetzt;
        snprintf(pfad, sizeof(pfad), "%s/bild_%05d.jpg", ordner, bilder + 1);
        if (speichereBild(pfad)) bilder = bilder + 1;   // nur diese Aufgabe schreibt, andere lesen nur
      }
      if (jetzt - letzteZeile >= LERNEN_DATEN_MS) {
        letzteZeile = jetzt;
        LernDaten d;
        portENTER_CRITICAL(&sperre);
        d = daten;
        portEXIT_CRITICAL(&sperre);
        snprintf(zeile, sizeof(zeile), "%lu,%d,%d,%d,%d,%d,%.1f,%.1f,%.1f,%.1f,%d,%d,%d,%d",
                 jetzt - fahrtStart, bilder, d.x, d.y, d.links, d.rechts, d.drehrate, d.winkel,
                 d.nicken, d.rollen, d.akku, d.pegel, d.modus, d.licht ? 1 : 0);
        if (!csv.println(zeile)) {                 // Karte voll oder herausgezogen
          aktiv = false;
          csv.close();
          Serial.println(">> LERNEN abgebrochen: SD-Karte voll oder entfernt");
        }
      }
      if (aktiv && jetzt - letzterFlush >= 1000) { letzterFlush = jetzt; csv.flush(); }   // bei Stromausfall max. 1 s weg
    }
    delay(10);
  }
}

void lernenStarten() {
  if (!karteDa()) return;
  xTaskCreatePinnedToCore(lernAufgabe, "lernen", 8192, nullptr, 1, nullptr, 0);
}
