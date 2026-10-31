#include <Arduino.h>
#include "config.h"
#include "buzzer.h"
#include "einstellungen.h"

// ============================================================
//  Klangdesign "Hightech": statt harter Pieptoene gleitende Toene (Chirps),
//  kurze Klicks als Bestaetigung und eine Zwei-Ton-Hupe.
//  Jeder Ton gleitet in "dauer" ms von "von" nach "bis" Hz (0 = Pause).
// ============================================================
struct Ton {
  uint16_t von;
  uint16_t bis;
  uint16_t dauer;
};

#define LAENGE(m) (sizeof(m) / sizeof(Ton))

// Verbinden: zwei aufsteigende Chirps - "da-ding"
static const Ton KLANG_VERBUNDEN[]  = { {900, 1800, 90}, {0, 0, 25}, {1800, 3200, 110} };
// Trennen: absteigender Gleitton - "uuuh"
static const Ton KLANG_GETRENNT[]   = { {2400, 700, 260} };
// WLAN ok: drei schnelle, steigende Blips
static const Ton KLANG_WLAN_OK[]    = { {2600, 2600, 35}, {0, 0, 35}, {3100, 3100, 35}, {0, 0, 35}, {3700, 3700, 60} };
// WLAN Fehler: tiefer, fallender Ton
static const Ton KLANG_WLAN_FEHLER[]= { {700, 250, 380} };
// Warnung / gesperrt: zwei tiefe, kurze Blips
static const Ton KLANG_WARNUNG[]    = { {420, 380, 50}, {0, 0, 45}, {420, 380, 50} };
// Bestaetigung (Licht, Modus, ...): ganz kurzer hoher Klick
static const Ton KLANG_TICK[]       = { {3200, 2600, 18} };
// Ein / Aus von Funktionen: kurzer Chirp hoch bzw. runter
static const Ton KLANG_EIN[]        = { {1600, 2800, 70} };
static const Ton KLANG_AUS[]        = { {2800, 1400, 70} };
// Alarm: schnelle Auf-Ab-Sweeps
static const Ton KLANG_ALARM[]      = { {900, 3000, 110}, {3000, 900, 110}, {900, 3000, 110}, {3000, 900, 110},
                                        {900, 3000, 110}, {3000, 900, 110}, {900, 3000, 110}, {3000, 600, 250} };
// Einschalten: "Hochfahren" wie ein Geraet, das bootet
static const Ton KLANG_START[]      = { {300, 2200, 380}, {0, 0, 40}, {2200, 2200, 30}, {0, 0, 40}, {3300, 3300, 70} };

static const Ton* melodie = nullptr;
static int anzahlToene = 0;
static int aktuellerTon = -1;
static unsigned long tonStart = 0;
static unsigned long letzteNachstellung = 0;
static bool tonLaeuft = false;

static bool hupeAn = false;
static int  hupeFrequenz = HUPE_FREQUENZ;
static bool sireneAn = false;
static unsigned long effektStart = 0;
static unsigned long effektLetzte = 0;

static int  musikHz = -1;          // -1 = keine Musik
static int  gesetztHz = 0;         // was der Buzzer gerade spielt

// Lautstaerke: der Buzzer bekommt ein Rechtecksignal. Bei 50 % Tastverhaeltnis ist er am lautesten,
// je schmaler die Pulse, desto leiser. Quadratische Kurve, damit der Regler sich gleichmaessig anfuehlt.
static bool pwmAngemeldet = false;
static uint32_t tastverhaeltnis() {
  float a = einst().lautstaerke / 100.0f;
  a = constrain(a * a, 0.0f, 1.0f);
  return max(1, (int)(asinf(a) / PI * 1024.0f + 0.5f));        // 10 Bit: 512 = 50 % = volle Lautstaerke
}

static void stille() {
  if (pwmAngemeldet) ledcWrite(BUZZER_PIN, 0);
  tonLaeuft = false;
  gesetztHz = 0;
}

static void setzeFrequenz(int f) {
  if (f <= 0) { stille(); return; }
  if (!pwmAngemeldet) pwmAngemeldet = ledcAttach(BUZZER_PIN, f, 10);
  ledcWriteTone(BUZZER_PIN, f);                       // Frequenz einstellen ...
  ledcWrite(BUZZER_PIN, tastverhaeltnis());           // ... und Lautstaerke
  tonLaeuft = true;
  gesetztHz = f;
}

void buzzerMusik(int hz) {
  if (hz < 0 && musikHz >= 0 && melodie == nullptr && !hupeAn && !sireneAn) stille();
  musikHz = hz;
}

// wichtig = auch bei "nur wichtige Toene" hoerbar (Alarm, Verbinden/Trennen, Warnung)
static void starteMelodie(const Ton* m, int anzahl, bool wichtig = false) {
  if (einst().toene == TOENE_STUMM) return;
  if (einst().toene == TOENE_WICHTIG && !wichtig) return;
  hupeAn = false;                    // Melodie beendet laufende Hupe/Sirene
  sireneAn = false;
  melodie = m;
  anzahlToene = anzahl;
  aktuellerTon = 0;
  tonStart = millis();
  letzteNachstellung = 0;
  setzeFrequenz(m[0].von);
}

void buzzerStarten() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}

void buzzerVerbunden()  { starteMelodie(KLANG_VERBUNDEN,   LAENGE(KLANG_VERBUNDEN), true); }
void buzzerGetrennt()   { starteMelodie(KLANG_GETRENNT,    LAENGE(KLANG_GETRENNT), true); }
void buzzerWlanOk()     { starteMelodie(KLANG_WLAN_OK,     LAENGE(KLANG_WLAN_OK)); }
void buzzerWlanFehler() { starteMelodie(KLANG_WLAN_FEHLER, LAENGE(KLANG_WLAN_FEHLER), true); }
void buzzerAlarm()      { starteMelodie(KLANG_ALARM,       LAENGE(KLANG_ALARM), true); }
void buzzerWarnung()    { starteMelodie(KLANG_WARNUNG,     LAENGE(KLANG_WARNUNG), true); }
void buzzerTick()       { if (!hupeAn && !sireneAn) starteMelodie(KLANG_TICK, LAENGE(KLANG_TICK)); }
void buzzerEin()        { if (!hupeAn && !sireneAn) starteMelodie(KLANG_EIN,  LAENGE(KLANG_EIN)); }
void buzzerProbe()      { starteMelodie(KLANG_EIN, LAENGE(KLANG_EIN), true); }
void buzzerPiepsen(int hz, int ms) { setzeFrequenz(hz); delay(ms); stille(); }
void buzzerAus()        { if (!hupeAn && !sireneAn) starteMelodie(KLANG_AUS,  LAENGE(KLANG_AUS)); }

// Startklang: laeuft BLOCKIEREND (fuer setup, wo noch kein loop laeuft).
// Ruft waehrenddessen "beiJedemSchritt(fortschritt 0..1)" auf - fuer die passende Ring-Animation.
void buzzerStartklang(void (*beiJedemSchritt)(float)) {
  unsigned long gesamt = 0;
  bool stumm = einst().toene != TOENE_ALLE;     // Startklang nur bei "alle Toene"
  for (auto& t : KLANG_START) gesamt += t.dauer;
  unsigned long start = millis(), vorher = 0;
  for (auto& t : KLANG_START) {
    unsigned long tStart = millis();
    while (millis() - tStart < t.dauer) {
      float p = (millis() - tStart) / (float)t.dauer;
      setzeFrequenz(stumm || t.von == 0 ? 0 : t.von + (int)((t.bis - t.von) * p));
      if (beiJedemSchritt) beiJedemSchritt((millis() - start) / (float)gesamt);
      delay(8);
    }
    vorher += t.dauer;
  }
  stille();
  if (beiJedemSchritt) beiJedemSchritt(1.0f);
}

void buzzerHupe(bool an, int frequenz) {
  if (an == hupeAn) return;
  hupeAn = an;
  if (an) {
    melodie = nullptr;
    sireneAn = false;
    hupeFrequenz = frequenz;
    effektStart = millis();
    effektLetzte = 0;
  } else {
    stille();
  }
}

void buzzerSirene(bool an) {
  if (an == sireneAn) return;
  sireneAn = an;
  if (an) {
    melodie = nullptr;
    hupeAn = false;
    effektStart = millis();
    effektLetzte = 0;
  } else {
    stille();
  }
}

void buzzerUpdate() {
  unsigned long jetzt = millis();

  // Hupe: zwei Toene im schnellen Wechsel (Grundton + grosse Terz) -> klingt "elektronisch"
  if (hupeAn) {
    if (jetzt - effektLetzte < 35) return;
    effektLetzte = jetzt;
    bool hoch = ((jetzt - effektStart) / 35) % 2;
    setzeFrequenz(hoch ? hupeFrequenz * 5 / 4 : hupeFrequenz);
    return;
  }

  // Sirene: Frequenz laeuft im Dreieck MIN -> MAX -> MIN
  if (sireneAn) {
    if (jetzt - effektLetzte < 10) return;
    effektLetzte = jetzt;
    float phase = ((jetzt - effektStart) % SIRENE_RUNDE_MS) / (float)SIRENE_RUNDE_MS;
    float dreieck = phase < 0.5f ? phase * 2 : (1 - phase) * 2;
    setzeFrequenz(SIRENE_MIN_HZ + (int)((SIRENE_MAX_HZ - SIRENE_MIN_HZ) * dreieck));
    return;
  }

  if (melodie == nullptr) {
    if (musikHz >= 0 && musikHz != gesetztHz) setzeFrequenz(musikHz);   // Jukebox
    return;
  }
  const Ton& t = melodie[aktuellerTon];
  unsigned long dauer = jetzt - tonStart;

  if (dauer >= t.dauer) {                         // naechster Ton
    if (aktuellerTon + 1 < anzahlToene) {
      aktuellerTon++;
      tonStart = jetzt;
      setzeFrequenz(melodie[aktuellerTon].von);
    } else {
      stille();
      melodie = nullptr;
    }
    return;
  }
  // Gleiten: alle 8 ms die Frequenz nachstellen
  if (t.von != t.bis && t.von != 0 && jetzt - letzteNachstellung >= 8) {
    letzteNachstellung = jetzt;
    setzeFrequenz(t.von + (int)((t.bis - t.von) * (dauer / (float)t.dauer)));
  }
}
