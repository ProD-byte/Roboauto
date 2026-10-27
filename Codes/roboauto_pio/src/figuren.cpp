#include <Arduino.h>
#include "config.h"
#include "figuren.h"
#include "gyro.h"
#include "antrieb.h"
#include "einstellungen.h"

// Ein Programm besteht aus Abschnitten:
//  F_GERADE: geradeaus fahren (Zeit in ms), Richtung per Gyro halten
//  F_DREHEN: auf der Stelle drehen (Grad, + = links)
//  F_BOGEN : Kurve fahren, bis der Winkel erreicht ist (Grad, + = links)
//  F_PAUSE : kurz stehen (ms)
enum Art { F_GERADE, F_DREHEN, F_BOGEN, F_PAUSE };   // F_ = Figur-Abschnitt
struct Abschnitt { Art art; float wert; };

static const Abschnitt QUADRAT[] = {
  {F_GERADE, FIGUR_GERADE_MS}, {F_PAUSE, 200}, {F_DREHEN, 90},
  {F_GERADE, FIGUR_GERADE_MS}, {F_PAUSE, 200}, {F_DREHEN, 90},
  {F_GERADE, FIGUR_GERADE_MS}, {F_PAUSE, 200}, {F_DREHEN, 90},
  {F_GERADE, FIGUR_GERADE_MS}, {F_PAUSE, 200}, {F_DREHEN, 90},
};
static const Abschnitt ACHT[] = {
  {F_BOGEN, 360}, {F_BOGEN, -360},
};
static const Abschnitt SLALOM[] = {
  {F_BOGEN, 45}, {F_BOGEN, -90}, {F_BOGEN, 90}, {F_BOGEN, -90}, {F_BOGEN, 90}, {F_BOGEN, -45},
};

static const Abschnitt* programm = nullptr;
static int anzahl = 0;
static int schritt = -1;
static int nummer = -1;
static float planRichtung = 0;      // geplante Richtung (Summe aller Winkel) -> kein Aufsummieren von Fehlern
static float abschnittZiel = 0;
static unsigned long abschnittStart = 0;

bool figurAktiv()  { return programm != nullptr; }
int  figurNummer() { return nummer; }

static void starteAbschnitt(int i) {
  schritt = i;
  abschnittStart = millis();
  const Abschnitt& a = programm[i];
  if (a.art == F_DREHEN || a.art == F_BOGEN) {
    abschnittZiel = planRichtung + a.wert;
  }
}

static void naechsterAbschnitt() {
  const Abschnitt& a = programm[schritt];
  if (a.art == F_DREHEN || a.art == F_BOGEN) planRichtung = abschnittZiel;
  if (schritt + 1 < anzahl) {
    starteAbschnitt(schritt + 1);
  } else {
    Serial.printf(">> Figur %s FERTIG (Abweichung %.1f Grad)\n", FIGUR_NAME[nummer], gyroWinkel() - planRichtung);
    figurAbbrechen();
  }
}

bool figurStarten(int figur) {
  if (!gyroBereit()) return false;
  switch (figur) {
    case FIGUR_QUADRAT: programm = QUADRAT; anzahl = sizeof(QUADRAT) / sizeof(Abschnitt); break;
    case FIGUR_ACHT:    programm = ACHT;    anzahl = sizeof(ACHT)    / sizeof(Abschnitt); break;
    case FIGUR_SLALOM:  programm = SLALOM;  anzahl = sizeof(SLALOM)  / sizeof(Abschnitt); break;
    default: return false;
  }
  nummer = figur;
  planRichtung = gyroWinkel();
  starteAbschnitt(0);
  Serial.printf(">> Figur %s START\n", FIGUR_NAME[figur]);
  return true;
}

void figurAbbrechen() {
  if (programm == nullptr) return;
  programm = nullptr;
  nummer = -1;
  schritt = -1;
  antriebStopp();
}

void figurUpdate() {
  if (programm == nullptr) return;
  const Abschnitt& a = programm[schritt];
  unsigned long dauer = millis() - abschnittStart;

  if (dauer > FIGUR_SEGMENT_MAX_MS) {           // Sicherheit: haengt fest?
    Serial.println(">> Figur abgebrochen (Abschnitt dauert zu lange)");
    figurAbbrechen();
    return;
  }

  switch (a.art) {
    case F_PAUSE:
      antriebDirekt(0, 0);
      if (dauer >= a.wert) naechsterAbschnitt();
      break;

    case F_GERADE: {
      // Richtung halten wie beim Geradeauslauf (positive Korrektur = nach rechts lenken)
      float k = GERADEAUS_KP * (gyroWinkel() - planRichtung) + GERADEAUS_KD * gyroDrehrate();
      int korr = constrain((int)k, -GERADEAUS_MAX_KORR, GERADEAUS_MAX_KORR);
      antriebDirekt(FIGUR_TEMPO + korr, FIGUR_TEMPO - korr);
      if (dauer >= a.wert * einst().figurGroesse / 100.0f) naechsterAbschnitt();   // Groesse aus den Einstellungen
      break;
    }

    case F_DREHEN: {
      // Wie die Wende: zuegig drehen, kurz vor dem Ziel langsamer, mit Nachlauf-Vorhalt
      float rest = abschnittZiel - gyroWinkel();
      float richtung = rest >= 0 ? 1 : -1;                // + = links
      float restBetrag = fabs(rest) - fabs(gyroDrehrate()) * FIGUR_VORHALT;
      if (restBetrag <= WENDE_TOLERANZ) { antriebDirekt(0, 0); naechsterAbschnitt(); break; }
      int tempo = WENDE_TEMPO_MAX;
      if (restBetrag < WENDE_BREMS_BEREICH)
        tempo = map((int)restBetrag, 0, (int)WENDE_BREMS_BEREICH, WENDE_TEMPO_MIN, WENDE_TEMPO_MAX);
      antriebDirekt(-richtung * tempo, richtung * tempo);  // links drehen: linkes Rad zurueck
      break;
    }

    case F_BOGEN: {
      float rest = abschnittZiel - gyroWinkel();
      float richtung = a.wert >= 0 ? 1 : -1;
      float restBetrag = rest * richtung - fabs(gyroDrehrate()) * FIGUR_VORHALT;
      if (restBetrag <= WENDE_TOLERANZ) { naechsterAbschnitt(); break; }   // ohne Stopp in den naechsten Bogen
      // Linksbogen: rechtes Rad schneller
      antriebDirekt(FIGUR_BOGEN_TEMPO - richtung * FIGUR_BOGEN_LENK, FIGUR_BOGEN_TEMPO + richtung * FIGUR_BOGEN_LENK);
      break;
    }
  }
}
