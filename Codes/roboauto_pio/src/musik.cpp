#include <Arduino.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "musik.h"
#include "buzzer.h"

// ------------------------------------------------------------
//  Notenschrift (kleine eigene Sprache, gut lesbar):
//    e6:4    Note e in Oktave 6, Viertelnote      (#/b fuer Halbtoene: c#6, bb5)
//    a5:8.   punktierte Achtel
//    p:8     Pause
//    [a5c6e6]:4   Akkord - wird als schnelles Arpeggio gespielt (typischer Chiptune-Klang)
//    T150    Tempo in Schlaegen pro Minute (Viertel)
// ------------------------------------------------------------
static const char LIED_TETRIS[] =
  "T160 "
  "e6:4 b5:8 c6:8 d6:4 c6:8 b5:8 a5:4 a5:8 c6:8 e6:4 d6:8 c6:8 b5:4. c6:8 d6:4 e6:4 c6:4 a5:4 a5:4 p:4 "
  "d6:4. f6:8 a6:4 g6:8 f6:8 e6:4. c6:8 e6:4 d6:8 c6:8 b5:4 b5:8 c6:8 d6:4 e6:4 c6:4 a5:4 a5:4 p:4 "
  "e6:2 c6:2 d6:2 b5:2 c6:2 a5:2 g#5:2 b5:4 p:4 e6:2 c6:2 d6:2 b5:2 c6:4 e6:4 a6:2 g#6:2 p:4 "
  "T200 "
  "e6:4 b5:8 c6:8 d6:4 c6:8 b5:8 a5:4 a5:8 c6:8 e6:4 d6:8 c6:8 b5:4. c6:8 d6:4 e6:4 c6:4 a5:4 a5:4 p:4 "
  "d6:4. f6:8 a6:4 g6:8 f6:8 e6:4. c6:8 e6:4 d6:8 c6:8 b5:4 b5:8 c6:8 d6:4 e6:4 c6:4 a5:4 [a5c6e6]:2";

#define BERG "a5:8 b5:8 c6:8 d6:8 e6:8 c6:8 e6:4 d#6:8 b5:8 d#6:4 d6:8 bb5:8 d6:4 " \
             "a5:8 b5:8 c6:8 d6:8 e6:8 c6:8 e6:8 a6:8 g6:8 e6:8 c6:8 e6:8 g6:2 "
static const char LIED_BERGKOENIG[] =
  "T100 " BERG "T130 " BERG "T165 " BERG "T210 " BERG "T260 " BERG
  "[a5c6e6]:4 p:4 [a5c6e6]:4 p:4 [a5c6e6a6]:2";

static const char LIED_ELISE[] =
  "T100 "
  "e6:16 d#6:16 e6:16 d#6:16 e6:16 b5:16 d6:16 c6:16 a5:8 p:16 c5:16 e5:16 a5:16 b5:8 p:16 e5:16 g#5:16 b5:16 c6:8 p:16 e5:16 "
  "e6:16 d#6:16 e6:16 d#6:16 e6:16 b5:16 d6:16 c6:16 a5:8 p:16 c5:16 e5:16 a5:16 b5:8 p:16 e5:16 c6:16 b5:16 a5:4 p:8 "
  "e6:16 d#6:16 e6:16 d#6:16 e6:16 b5:16 d6:16 c6:16 a5:8 p:16 c5:16 e5:16 a5:16 b5:8 p:16 e5:16 g#5:16 b5:16 c6:8 p:16 e5:16 "
  "e6:16 d#6:16 e6:16 d#6:16 e6:16 b5:16 d6:16 c6:16 a5:8 p:16 c5:16 e5:16 a5:16 b5:8 p:16 e5:16 c6:16 b5:16 a5:4";

static const char LIED_FREUDE[] =
  "T150 "
  "e6:4 e6:4 f6:4 g6:4 g6:4 f6:4 e6:4 d6:4 c6:4 c6:4 d6:4 e6:4 e6:4. d6:8 d6:2 "
  "e6:4 e6:4 f6:4 g6:4 g6:4 f6:4 e6:4 d6:4 c6:4 c6:4 d6:4 e6:4 d6:4. c6:8 c6:2 "
  "d6:4 d6:4 e6:4 c6:4 d6:4 e6:8 f6:8 e6:4 c6:4 d6:4 e6:8 f6:8 e6:4 d6:4 c6:4 d6:4 g5:2 "
  "e6:4 e6:4 f6:4 g6:4 g6:4 f6:4 e6:4 d6:4 c6:4 c6:4 d6:4 e6:4 d6:4. c6:8 [c6e6g6]:2";

#define ENT_A "d5:16 d#5:16 e5:16 c6:8 e5:16 c6:8 e5:16 c6:4. " \
              "c6:16 d6:16 d#6:16 e6:16 c6:16 d6:16 e6:8 b5:16 d6:8 c6:4 p:8 "
static const char LIED_ENTERTAINER[] =
  "T95 " ENT_A
  "d5:16 d#5:16 e5:16 c6:8 e5:16 c6:8 e5:16 c6:4. "
  "a5:16 g5:16 f#5:16 a5:16 c6:16 e6:8 d6:16 c6:16 a5:16 d6:4 p:8 "
  ENT_A
  "c6:16 d6:16 e6:16 c6:16 d6:16 e6:8 c6:16 d6:16 c6:16 e6:16 c6:16 d6:16 e6:8 c6:16 d6:16 c6:16 e6:16 c6:16 d6:16 e6:8 b5:16 d6:8 [c6e6g6]:4";

#define THEMA "[a5c6e6]:8 p:16 [a5c6e6]:16 a6:8 g6:8 e6:8 c6:8 d6:8 e6:8 " \
              "[f5a5c6]:8 p:16 [f5a5c6]:16 a6:8 g6:8 f6:8 e6:8 d6:8 c6:8 " \
              "[c6e6g6]:8 p:16 [c6e6g6]:16 g6:8 e6:8 c7:8 b6:8 a6:8 g6:8 " \
              "[g5b5d6]:4 b6:8 c7:8 d7:4 [g5b5d6]:4 "
static const char LIED_THEMA[] =
  "T150 a4:8 a5:8 a4:8 a5:8 a4:8 a5:8 [e6g#6b6]:4 " THEMA THEMA
  "[a5c6e6a6]:2 p:4 a4:8 a5:8 a6:4";

#define BOSS "e5:16 e5:16 e6:8 e5:16 e5:16 d6:8 e5:16 e5:16 c6:8 e5:16 e5:16 bb5:8 e5:16 e5:16 b5:8 c6:8 "
static const char LIED_BOSS[] =
  "T180 " BOSS BOSS
  "[e5g#5b5]:8 [f5a5c6]:8 [f#5a#5c#6]:8 [g5b5d6]:8 [g#5c6d#6]:4 [a5c6e6]:4 " BOSS BOSS
  "f6:16 e6:16 d#6:16 e6:16 g6:16 f#6:16 f6:16 f#6:16 a6:16 g#6:16 g6:16 g#6:16 b6:8 c7:8 [e6a6c7e7]:2";

static const char LIED_GESCHAFFT[] =
  "T140 g5:8 c6:8 e6:8 g6:4 e6:8 g6:2 p:8 a5:8 d6:8 f6:8 a6:4 f6:8 a6:2 p:8 "
  "b5:8 d6:8 g6:8 b6:8 a6:8 b6:8 [c6e6g6c7]:2.";


// ---------- Neue Stuecke (Update) ----------
// Neon-Jagd: C64-Stil - Bass und Akkord wechseln sich im 16tel-Takt ab, darueber die Melodie
#define NEON_A "e5:16 e6:16 [e6g6b6]:8 e5:16 e6:16 [e6g6b6]:8 e5:16 e6:16 [e6g6b6]:8 d6:16 e6:16 g6:8 " \
               "c5:16 c6:16 [c6e6g6]:8 c5:16 c6:16 [c6e6g6]:8 c5:16 c6:16 [c6e6g6]:8 b5:16 c6:16 e6:8 " \
               "d5:16 d6:16 [d6f#6a6]:8 d5:16 d6:16 [d6f#6a6]:8 d5:16 d6:16 [d6f#6a6]:8 a5:16 d6:16 f#6:8 " \
               "b4:16 b5:16 [b5d#6f#6]:8 b4:16 b5:16 [b5d#6f#6]:8 b4:16 b5:16 [b5d#6f#6]:8 f#6:16 a6:16 b6:8 "
#define NEON_B "b6:8 b6:16 a6:16 g6:8 f#6:8 g6:8 a6:8 b6:4 c7:8 b6:16 a6:16 g6:8 e6:8 g6:4 e6:4 " \
               "d6:8 f#6:16 a6:16 d7:8 c7:8 b6:8 a6:8 f#6:4 d#6:8 f#6:8 b6:8 a6:8 g6:8 f#6:8 d#6:4 "
static const char LIED_NEON[] =
  "T170 " NEON_A NEON_B "T175 " NEON_A
  "e7:8 e7:16 d7:16 b6:8 g6:8 a6:8 b6:8 e7:4 c7:8 c7:16 b6:16 g6:8 e6:8 g6:8 b6:8 c7:4 "
  "d7:8 d7:16 c7:16 a6:8 f#6:8 a6:8 d7:8 f#7:4 [b6d#7f#7]:4 b7:8 a7:8 f#7:8 d#7:8 b6:4 "
  "T190 " NEON_A "[e6g6b6e7]:4 p:8 e5:8 [e6g6b6e7]:2";

// Pixel-Galaxie: episch und weit - schimmernde Akkorde, singende Melodie mit Vibrato
#define GALAXIE "[a5c6e6]:8 e6:4 d6:8 c6:8 b5:8 c6:4 [f5a5c6]:8 a6:4 g6:8 f6:8 e6:8 c6:4 " \
                "[c6e6g6]:8 g6:4 f6:8 e6:8 d6:8 e6:4 [g5b5d6]:8 d6:4 c6:8 b5:8 a5:8 b5:4 "
static const char LIED_GALAXIE[] =
  "T120 [a5c6e6]:2 [f5a5c6]:2 [c6e6g6]:2 [g5b5d6]:2 " GALAXIE GALAXIE
  "e6:8 a6:8 c7:4 b6:8 g6:8 e6:4 f6:8 a6:8 c7:4 d7:4 c7:4 "
  "b6:8 g6:8 e6:8 g6:8 c7:4 b6:4 [a5c6e6a6]:2 [e6a6c7e7]:2 "
  "T110 " GALAXIE "[a5c6e6a6]:1";

// Turbo-Rennen: Startampel, dann Vollgas - pulsierender Bass im Wechsel mit der Melodie
#define TURBO_A "a4:16 a5:16 a4:16 g5:16 a4:16 e5:16 a4:16 d5:16 a4:16 c5:16 a4:16 d5:16 a4:16 e5:16 g5:16 a5:16 "
#define TURBO_D "d5:16 d6:16 d5:16 c6:16 d5:16 a5:16 d5:16 g5:16 d5:16 f5:16 d5:16 g5:16 d5:16 a5:16 c6:16 d6:16 "
#define TURBO_E "e5:16 e6:16 e5:16 d6:16 e5:16 b5:16 e5:16 a5:16 f5:16 f6:16 f5:16 e6:16 g5:16 g6:16 g#5:16 g#6:16 "
static const char LIED_TURBO[] =
  "T120 a5:8 p:4. a5:8 p:4. a5:8 p:4. a6:2 "                              // Startampel: piep - piep - piep - LOS
  "T200 " TURBO_A TURBO_A TURBO_D TURBO_A TURBO_E TURBO_A TURBO_A TURBO_D TURBO_A TURBO_E
  "T210 a5:16 c6:16 e6:16 a6:16 e6:16 c6:16 a5:16 c6:16 e6:16 a6:16 c7:16 a6:16 e6:16 a6:16 c7:16 e7:16 "
  "[a5c#6e6]:8 [a5c#6e6]:8 p:8 [a5c#6e6a6]:2";

// Wilhelm Tell (Rossini, 1829) - der beruehmte Galopp
#define TELL_1 "e5:16 e5:16 e5:8 e5:16 e5:16 e5:8 e5:16 e5:16 a5:8 b5:8 c#6:8 "
#define TELL_2 "e5:16 e5:16 e5:8 e5:16 e5:16 a5:8 c#6:16 c#6:16 b5:8 g#5:8 e5:8 "
#define TELL_3 "a5:8 c#6:8 e6:4 d6:8 c#6:8 b5:8 a5:8 "
static const char LIED_TELL[] =
  "T150 " TELL_1 TELL_2 TELL_1 TELL_3 TELL_1 TELL_2 TELL_1
  "a5:8 c#6:8 e6:8 a6:8 g#6:8 e6:8 b5:8 g#5:8 a5:4 [a5c#6e6]:4 [a5c#6e6a6]:2";

// Hummelflug (Rimski-Korsakow, 1900) - rasende chromatische Laeufe
#define HUMMEL_RUNTER "e6:16 d#6:16 d6:16 c#6:16 d6:16 c#6:16 c6:16 b5:16 c6:16 b5:16 a#5:16 a5:16 g#5:16 g5:16 f#5:16 f5:16 "
#define HUMMEL_E "e5:16 d#5:16 d5:16 c#5:16 c5:16 f5:16 e5:16 d#5:16 e5:16 d#5:16 d5:16 c#5:16 c5:16 c#5:16 d5:16 d#5:16 "
#define HUMMEL_A "a5:16 g#5:16 g5:16 f#5:16 f5:16 a#5:16 a5:16 g#5:16 a5:16 g#5:16 g5:16 f#5:16 f5:16 f#5:16 g5:16 g#5:16 "
static const char LIED_HUMMEL[] =
  "T145 " HUMMEL_RUNTER HUMMEL_RUNTER HUMMEL_E HUMMEL_E HUMMEL_A HUMMEL_A HUMMEL_E HUMMEL_E
  "e5:16 f5:16 f#5:16 g5:16 g#5:16 a5:16 a#5:16 b5:16 c6:16 c#6:16 d6:16 d#6:16 e6:16 f6:16 f#6:16 g6:16 "
  "g#6:16 a6:16 a#6:16 b6:16 c7:16 c#7:16 d7:16 d#7:16 e7:8 p:8 [a5c6e6a6]:4";


// Traumwelt: eigenes Titelmelodie-Intro im Stil der spaeten C64-Aera (d-Moll / F-Dur)
// Aufbau: schimmerndes Intro -> treibender Groove -> Melodie -> Breakdown -> Aufstieg -> Finale
#define TRAUM_INTRO "[d6f6a6]:2 [a#5d6f6]:2 [c6f6a6]:2 [c6e6g6]:2 "
#define TRAUM_GROOVE "d5:16 d6:16 [d6f6a6]:8 d5:16 d6:16 [d6f6a6]:8 d5:16 a5:16 d6:16 f6:16 a6:16 f6:16 d6:16 a5:16 " \
                     "a#4:16 a#5:16 [a#5d6f6]:8 a#4:16 a#5:16 [a#5d6f6]:8 a#4:16 f5:16 a#5:16 d6:16 f6:16 d6:16 a#5:16 f5:16 " \
                     "f5:16 f6:16 [f6a6c7]:8 f5:16 f6:16 [f6a6c7]:8 f5:16 c6:16 f6:16 a6:16 c7:16 a6:16 f6:16 c6:16 " \
                     "c5:16 c6:16 [c6e6g6]:8 c5:16 c6:16 [c6e6g6]:8 c5:16 g5:16 c6:16 e6:16 g6:16 e6:16 c6:16 g5:16 "
#define TRAUM_A "a6:8 a6:16 g6:16 f6:8 e6:8 f6:4 d6:4 d6:8 f6:8 a#6:4 a6:8 g6:8 f6:4 " \
                "c7:8 a6:8 f6:8 a6:8 g6:4. f6:8 e6:8 f6:8 g6:8 e6:8 c6:4 p:4 "
#define TRAUM_B "d7:8 c7:8 a6:8 f6:8 g6:8 a6:8 f6:4 a#6:8 a6:8 g6:8 f6:8 d6:4 f6:4 " \
                "a6:8 c7:8 f7:4 e7:8 d7:8 c7:4 [c6e6g6c7]:4 c7:8 e7:8 [d6f6a6d7]:2 "
#define TRAUM_BREAK "d5:8 p:8 d6:16 p:16 [d6f6a6]:8 p:4 a5:8 c6:8 a#4:8 p:8 a#5:16 p:16 [a#5d6f6]:8 p:4 f5:8 a5:8 " \
                    "f5:8 p:8 f6:16 p:16 [f6a6c7]:8 p:4 c6:8 e6:8 c5:8 p:8 c6:16 p:16 [c6e6g6]:8 p:4 g5:8 c6:8 "
#define TRAUM_AUFSTIEG "d6:16 e6:16 f6:16 g6:16 a6:16 a#6:16 c7:16 d7:16 e7:16 f7:16 e7:16 d7:16 c7:16 a#6:16 a6:16 g6:16 " \
                       "[c6e6g6]:2 [c6e6g6c7]:2 "
static const char LIED_TRAUM[] =
  "T100 " TRAUM_INTRO TRAUM_INTRO
  "T150 " TRAUM_GROOVE TRAUM_A TRAUM_B TRAUM_GROOVE TRAUM_BREAK TRAUM_AUFSTIEG
  "T165 " TRAUM_A TRAUM_B TRAUM_GROOVE
  "[d6f6a6d7]:4 p:8 d5:8 [d6f6a6d7]:2";

static const char* const LIEDER[] = { LIED_TETRIS, LIED_BERGKOENIG, LIED_ELISE, LIED_FREUDE,
                                      LIED_ENTERTAINER, LIED_THEMA, LIED_BOSS, LIED_GESCHAFFT,
                                      LIED_NEON, LIED_GALAXIE, LIED_TURBO, LIED_TELL, LIED_HUMMEL, LIED_TRAUM };
static const int ANZAHL = sizeof(LIEDER) / sizeof(LIEDER[0]);

// ------------------------------------------------------------
static int nummer = -1;
static const char* pos = nullptr;          // Lesestelle im Text
static int tempo = 120;
static int akkord[6]; static int akkordN = 0;
static unsigned long noteStart = 0, noteDauer = 0, klangDauer = 0;
static int aktTon = 0;
static int notenZaehler = 0;
static int gesamtLaenge = 1;

static float frequenz(char name, int vorz, int oktave) {
  static const int HALBTON[7] = {9, 11, 0, 2, 4, 5, 7};   // a b c d e f g
  int midi = 12 * (oktave + 1) + HALBTON[name - 'a'] + vorz;
  return 440.0f * powf(2.0f, (midi - 69) / 12.0f);
}

// eine Note "c#6" ab p lesen, liefert Frequenz und schiebt p weiter
static int leseNote(const char*& p) {
  char n = *p++;
  int vorz = 0;
  if (*p == '#') { vorz = 1; p++; }
  else if (*p == 'b' && isdigit((unsigned char)p[1])) { vorz = -1; p++; }
  int okt = 0;
  while (isdigit((unsigned char)*p)) okt = okt * 10 + (*p++ - '0');
  return (int)(frequenz(n, vorz, okt) + 0.5f);
}

// naechsten Baustein lesen. false = Lied zu Ende
static bool naechsteNote() {
  while (true) {
    while (*pos == ' ') pos++;
    if (*pos == 0) return false;
    if (*pos == 'T') { pos++; tempo = atoi(pos); while (isdigit((unsigned char)*pos)) pos++; continue; }
    akkordN = 0;
    if (*pos == 'p') { pos++; }
    else if (*pos == '[') {
      pos++;
      while (*pos && *pos != ']' && akkordN < 6) akkord[akkordN++] = leseNote(pos);
      if (*pos == ']') pos++;
    } else if (*pos >= 'a' && *pos <= 'g') {
      akkord[akkordN++] = leseNote(pos);
    } else { pos++; continue; }                     // unbekanntes Zeichen ueberspringen
    int teiler = 4;
    if (*pos == ':') { pos++; teiler = atoi(pos); while (isdigit((unsigned char)*pos)) pos++; }
    float ms = 60000.0f / tempo * 4.0f / max(1, teiler);
    if (*pos == '.') { ms *= 1.5f; pos++; }
    noteStart = millis();
    noteDauer = (unsigned long)ms;
    // Chiptune-Artikulation: kurze Noten leicht abgesetzt, lange fast gebunden
    klangDauer = akkordN ? (unsigned long)(ms * (ms < 200 ? 0.80f : 0.92f)) : 0;
    if (akkordN) notenZaehler++;
    return true;
  }
}

int musikAnzahl() { return ANZAHL; }
int musikNummer() { return nummer; }
int musikTon() { return nummer >= 0 ? aktTon : 0; }
int musikNotenNummer() { return notenZaehler; }
int musikFortschritt() {
  if (nummer < 0) return 0;
  return constrain((int)((pos - LIEDER[nummer]) * 100L / gesamtLaenge), 0, 100);
}

void musikStopp() {
  if (nummer < 0) return;
  nummer = -1;
  aktTon = 0;
  buzzerMusik(-1);
  Serial.println(">> Musik: Stopp");
}

void musikStarten(int nr) {
  if (nr < 0 || nr >= ANZAHL) return;
  if (nr == nummer) { musikStopp(); return; }        // nochmal klicken = aus
  nummer = nr;
  pos = LIEDER[nr];
  gesamtLaenge = max(1, (int)strlen(LIEDER[nr]));
  tempo = 120;
  Serial.printf(">> Musik: Lied %d\n", nr);
  if (!naechsteNote()) musikStopp();
}

void musikUpdate() {
  if (nummer < 0) return;
  unsigned long jetzt = millis();
  unsigned long t = jetzt - noteStart;
  if (t >= noteDauer) {
    if (!naechsteNote()) { musikStopp(); return; }
    t = 0;
  }
  int f = 0;
  if (akkordN && t < klangDauer) {
    if (akkordN > 1) {
      f = akkord[(t / 38) % akkordN];                      // Arpeggio: alle 38 ms der naechste Akkordton
    } else {
      f = akkord[0];
      if (noteDauer >= 400 && t > 150)                      // lange Noten: leichtes Vibrato
        f = (int)(f * (1.0f + 0.012f * sinf(((t - 150) / 10 * 10) * 0.0377f)));   // nur alle 10 ms neu
    }
  }
  aktTon = f;
  buzzerMusik(f);
}
