#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "ledring.h"
#include "einstellungen.h"

// ============================================================
//  Lichtdesign "Hightech":
//  - Alle Animationen zeichnen in einen Puffer ("ziel").
//  - Ausgegeben wird weich: jede LED gleitet in ca. 120 ms zur Zielfarbe.
//    Dadurch blenden Zustandswechsel sanft ineinander ueber statt zu springen.
//  - Nur Warnungen (Alarm, Polizei) sind bewusst hart und schnell.
//  - Bewegungen laufen stufenlos (auch "zwischen" zwei LEDs).
// ============================================================

static Adafruit_NeoPixel ring(LED_RING_ANZAHL, LED_RING_PIN, NEO_GRB + NEO_KHZ800);

static float ziel[LED_RING_ANZAHL][3];      // was die Animation gerade zeigen will (0..255)
static float aktuell[LED_RING_ANZAHL][3];   // was tatsaechlich leuchtet
static bool  hart = false;                  // true = ohne Ueberblenden (Blitzer, Alarm)
static unsigned long letzteAusgabe = 0;

static RingQuelle letzteQuelle = RING_NIEMAND;
static uint32_t signalFarbe = 0;
static unsigned long signalStart = 0;
static bool signalAktiv = false;
static bool einrichtung = false;
static bool alarmAn = false;               // ("alarm" allein ist schon eine System-Funktion)
static int  aufnahmeZustand = 0;
static float kalFortschritt = -1;
static unsigned long tankSeit = 0;
static bool musikAn = false;
static int  musikHz = 0, musikNote = -1, musikNoteVorher = -1;
static unsigned long musikNoteSeit = 0;
static float musikHue = 0;

// ---------- Zeichen-Hilfen ----------
static void leeren() {
  for (int i = 0; i < LED_RING_ANZAHL; i++) ziel[i][0] = ziel[i][1] = ziel[i][2] = 0;
}
static void setze(int i, float r, float g, float b) {
  i = ((i % LED_RING_ANZAHL) + LED_RING_ANZAHL) % LED_RING_ANZAHL;
  ziel[i][0] = r; ziel[i][1] = g; ziel[i][2] = b;
}
static void setzeFarbe(int i, uint32_t c, float h = 1.0f) {
  setze(i, ((c >> 16) & 0xFF) * h, ((c >> 8) & 0xFF) * h, (c & 0xFF) * h);
}
static void addiere(int i, float r, float g, float b) {
  i = ((i % LED_RING_ANZAHL) + LED_RING_ANZAHL) % LED_RING_ANZAHL;
  ziel[i][0] = min(255.0f, ziel[i][0] + r);
  ziel[i][1] = min(255.0f, ziel[i][1] + g);
  ziel[i][2] = min(255.0f, ziel[i][2] + b);
}

// Weicher Lichtpunkt an einer stufenlosen Position (z.B. 2,4 = zwischen LED 2 und 3)
static void lichtpunkt(float pos, float r, float g, float b, float breite = 1.0f) {
  for (int i = 0; i < LED_RING_ANZAHL; i++) {
    float d = fabs(pos - i);
    d = fmin(d, LED_RING_ANZAHL - d);                  // im Kreis messen
    float h = constrain(1.0f - d / breite, 0.0f, 1.0f);
    if (h > 0) addiere(i, r * h, g * h, b * h);
  }
}

// Komet: heller Kopf mit weich auslaufendem Schweif
static void komet(float kopf, float r, float g, float b, float schweif) {
  for (int i = 0; i < LED_RING_ANZAHL; i++) {
    float hinten = kopf - i;
    if (hinten < 0) hinten += LED_RING_ANZAHL;
    float h = 0;
    if (hinten <= schweif)               h = 1.0f - hinten / schweif;
    if (hinten > LED_RING_ANZAHL - 1)    h = 1.0f - (LED_RING_ANZAHL - hinten);
    h = constrain(h, 0.0f, 1.0f);
    h *= h;
    addiere(i, r * h, g * h, b * h);
  }
}

// Puffer ausgeben - weich (Ueberblenden) oder hart
static void ausgeben(unsigned long jetzt) {
  if (jetzt - letzteAusgabe < 15) return;            // ca. 66 Bilder pro Sekunde
  letzteAusgabe = jetzt;
  const float k = hart ? 1.0f : 0.22f;               // 0.22 je 15 ms -> ca. 120 ms Ueberblendzeit
  for (int i = 0; i < LED_RING_ANZAHL; i++) {
    for (int c = 0; c < 3; c++) aktuell[i][c] += (ziel[i][c] - aktuell[i][c]) * k;
    ring.setPixelColor(i, ring.Color((uint8_t)aktuell[i][0], (uint8_t)aktuell[i][1], (uint8_t)aktuell[i][2]));
  }
  ring.show();
}

static float atmen(unsigned long jetzt, unsigned long periode) {   // 0..1..0, weich
  return (1 - cosf(2 * PI * (jetzt % periode) / periode)) / 2;
}

void ringStarten() {
  ring.begin();
  ring.setBrightness(einst().ringHell);
  ring.clear();
  ring.show();
}

// ---------- Startanimation (beim Einschalten, laeuft zusammen mit dem Startklang) ----------
// Ein weisser Lichtpunkt rast einmal herum und zieht einen tuerkisen Schweif hinter sich her,
// am Ende leuchtet der ganze Ring kurz auf.
void ringStartanimation(float p) {
  leeren();
  if (p < 0.75f) {
    float pos = p / 0.75f * LED_RING_ANZAHL;
    for (int i = 0; i < LED_RING_ANZAHL; i++) if (i < pos) setze(i, 0, 60, 90);
    lichtpunkt(pos, 255, 255, 255, 1.2f);
  } else {
    float h = 1.0f - (p - 0.75f) / 0.25f;            // Aufleuchten und Ausklingen
    for (int i = 0; i < LED_RING_ANZAHL; i++) setze(i, 120 * h, 220 * h, 255 * h);
  }
  hart = true;
  letzteAusgabe = 0;
  ausgeben(millis());
}

// ---------- Zustands-Animationen ----------

// Niemand verbunden: "Bereitschaft" - Ring atmet dunkelblau, ein heller Punkt kreist langsam (wie ein Radar)
static void zeigeWarten(unsigned long jetzt) {
  float a = 0.25f + 0.75f * atmen(jetzt, 3000);
  for (int i = 0; i < LED_RING_ANZAHL; i++) setze(i, 0, 4 * a, 22 * a);
  komet((jetzt % 2400) * (float)LED_RING_ANZAHL / 2400, 30, 90, 255, 2.5f);
}

// Dabble: Regenbogen - langsamer und weicher als frueher
static void zeigeDabble(unsigned long jetzt) {
  uint16_t farbton = (jetzt * 9) & 0xFFFF;          // eine Runde ca. 7 s
  for (int i = 0; i < LED_RING_ANZAHL; i++) {
    uint32_t c = ring.gamma32(ring.ColorHSV(farbton + i * 65536L / LED_RING_ANZAHL, 230, 255));
    setzeFarbe(i, c);
  }
}

// Webseite: tuerkiser Komet, eine Runde in 4 s
static void zeigeWeb(unsigned long jetzt) {
  for (int i = 0; i < LED_RING_ANZAHL; i++) setze(i, 0, 6, 9);   // ganz schwacher Grundschein
  komet((jetzt % 4000) * (float)LED_RING_ANZAHL / 4000, 0, 180, 255, 3.0f);
}

// Einrichtungsmodus: orange atmen
static void zeigeEinrichtung(unsigned long jetzt) {
  float h = 0.08f + 0.92f * atmen(jetzt, 2500);
  for (int i = 0; i < LED_RING_ANZAHL; i++) setze(i, 255 * h, 100 * h, 0);
}

// Tankanzeige: Balken fuellt sich animiert, Farbe je LED von rot ueber gelb nach gruen.
// Die oberste LED des Balkens "pulsiert" leicht, fast leer blinkt alles rot.
static void zeigeTank(unsigned long jetzt, int akku) {
  if (akku > AKKU_LAEDT) {                           // nur USB: blaue Welle
    for (int i = 0; i < LED_RING_ANZAHL; i++) setze(i, 0, 40, 120);
    lichtpunkt((jetzt % 1500) * (float)LED_RING_ANZAHL / 1500, 0, 150, 255, 1.5f);
    return;
  }
  float stand = map(constrain(akku, AKKU_ANZEIGE_MIN, AKKU_ANZEIGE_MAX),
                    AKKU_ANZEIGE_MIN, AKKU_ANZEIGE_MAX, 0, 1000) / 1000.0f * LED_RING_ANZAHL;
  float aufbau = min(1.0f, (jetzt - tankSeit) / 450.0f);   // Balken laeuft beim Anzeigen hoch
  stand = max(stand * aufbau, 0.6f);
  bool fastLeer = stand < 2.0f;
  for (int i = 0; i < LED_RING_ANZAHL; i++) {
    float h = constrain(stand - i, 0.0f, 1.0f);
    float t = i / (float)(LED_RING_ANZAHL - 1);              // 0 = unten (rot) .. 1 = oben (gruen)
    float r = t < 0.5f ? 255 : 255 * (1 - (t - 0.5f) * 2);
    float g = t < 0.5f ? 255 * t * 2 : 255;
    if (fastLeer) { r = 255; g = 0; h *= 0.3f + 0.7f * atmen(jetzt, 700); }
    setze(i, r * h + 3, g * h + 3, 3);                       // leere LEDs glimmen ganz schwach
  }
}

// Polizei: blau/rot Blitzer - bewusst hart
static void zeigePolizei(unsigned long jetzt) {
  int stufe = (jetzt / 50) % 12;
  leeren();
  if (stufe == 0 || stufe == 2 || stufe == 4) {
    for (int i : {5, 6, 7}) setze(i, 0, 0, 255);
  } else if (stufe == 6 || stufe == 8 || stufe == 10) {
    for (int i : {1, 2, 3}) setze(i, 255, 0, 0);
  }
  hart = true;
}

// Team: Farbwelle in Teamfarbe
static void zeigeTeam(unsigned long jetzt, int modus) {
  float winkel = (jetzt % 3000) * 360.0f / 3000;
  for (int i = 0; i < LED_RING_ANZAHL; i++) {
    float welle = sinf((winkel + i * 360.0f / LED_RING_ANZAHL) * PI / 180.0f);
    uint16_t hue = TEAM_FARBTON[modus] + (int)(welle * TEAM_SCHWANK[modus]);
    float hell = 0.55f + 0.45f * (welle + 1) / 2;            // Welle auch in der Helligkeit
    setzeFarbe(i, ring.gamma32(ring.ColorHSV(hue, 255, 230)), hell);
  }
}

// Signal: Farbe "faehrt" einmal schnell um den Ring und blendet dann weich aus
static bool zeigeSignal(unsigned long jetzt) {
  unsigned long t = jetzt - signalStart;
  if (t > 1300) { signalAktiv = false; return false; }
  leeren();
  float pos = min(t / 450.0f, 1.0f) * LED_RING_ANZAHL;
  float aus = t > 800 ? 1.0f - (t - 800) / 500.0f : 1.0f;
  for (int i = 0; i < LED_RING_ANZAHL; i++)
    if (i < pos) setzeFarbe(i, signalFarbe, 0.55f * aus);
  if (t < 450) {
    uint32_t c = signalFarbe;
    lichtpunkt(pos, ((c >> 16) & 0xFF) * 0.5f + 127, ((c >> 8) & 0xFF) * 0.5f + 127, (c & 0xFF) * 0.5f + 127, 1.2f);
  }
  return true;
}

// Alarm: zwei rote Haelften wechseln sich hart ab, dazwischen weisse Blitze
static void zeigeAlarm(unsigned long jetzt) {
  leeren();
  int phase = (jetzt / 90) % 6;
  for (int i = 0; i < LED_RING_ANZAHL; i++) {
    bool ersteHaelfte = i < LED_RING_ANZAHL / 2;
    if (phase == 2 || phase == 5)                 setze(i, 255, 255, 255);
    else if ((phase < 2) == ersteHaelfte)         setze(i, 255, 0, 0);
  }
  hart = true;
}

// Aufnahme: roter Punkt "atmet" oben (wie die Aufnahme-LED einer Kamera)
// Abspielen: gruener Komet laeuft im Kreis
static void zeigeAufnahme(unsigned long jetzt) {
  leeren();
  if (aufnahmeZustand == 1) {
    float h = 0.15f + 0.85f * atmen(jetzt, 1200);
    setze(0, 255 * h, 0, 0);
    setze(1, 30 * h, 0, 0);
    setze(LED_RING_ANZAHL - 1, 30 * h, 0, 0);
  } else {
    komet((jetzt % 1200) * (float)LED_RING_ANZAHL / 1200, 0, 255, 90, 3.0f);
  }
}

// ---------- Hauptfunktion ----------
// Musik-Lichtshow: jede Note blitzt in einer Farbe je nach Tonhoehe auf,
// der Lichtpunkt springt im Kreis weiter, dazwischen glimmt der Ring weich nach.
static void zeigeMusik(unsigned long jetzt) {
  if (musikNote != musikNoteVorher) {
    musikNoteVorher = musikNote;
    musikNoteSeit = jetzt;
    if (musikHz > 0) {
      float halbton = 12.0f * log2f(musikHz / 440.0f);
      musikHue = fmodf(halbton / 12.0f + 10.0f, 1.0f);      // Tonhoehe -> Farbkreis
    }
  }
  float impuls = musikHz > 0 ? max(0.0f, 1.0f - (jetzt - musikNoteSeit) / 260.0f) : 0.0f;
  int punkt = ((musikNote % LED_RING_ANZAHL) + LED_RING_ANZAHL) % LED_RING_ANZAHL;
  for (int i = 0; i < LED_RING_ANZAHL; i++) {
    float h = fmodf(musikHue + i * 0.04f, 1.0f);
    uint32_t c = ring.ColorHSV((uint16_t)(h * 65535), 255, 255);
    float r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255;
    float stark = 0.10f + 0.30f * impuls + (i == punkt ? 0.6f * (0.3f + 0.7f * impuls) : 0.0f);
    setze(i, r * stark, g * stark, b * stark);
  }
}

void ringMusik(bool an, int hz, int note) { musikAn = an; musikHz = hz; musikNote = note; }

void ringStatus(RingQuelle quelle, bool tankanzeige, int akkuWert, int modus) {
  unsigned long jetzt = millis();
  hart = false;

  static bool tankVorher = false;
  if (tankanzeige && !tankVorher) tankSeit = jetzt;
  tankVorher = tankanzeige;
  letzteQuelle = quelle;

  if (alarmAn)                                zeigeAlarm(jetzt);
  else if (kalFortschritt >= 0) {               // Kalibrierung: weisser Kreis fuellt sich, Lichtpunkt laeuft voraus
    leeren();
    float pos = kalFortschritt * LED_RING_ANZAHL;
    for (int i = 0; i < LED_RING_ANZAHL; i++) setze(i, 40 * constrain(pos - i, 0.0f, 1.0f), 60 * constrain(pos - i, 0.0f, 1.0f), 70 * constrain(pos - i, 0.0f, 1.0f));
    lichtpunkt(pos, 255, 255, 255, 1.0f);
  }
  else if (signalAktiv && zeigeSignal(jetzt)) {}
  else if (musikAn)                           zeigeMusik(jetzt);
  else if (aufnahmeZustand != 0)              zeigeAufnahme(jetzt);
  else if (quelle == RING_NIEMAND)            { if (einrichtung) zeigeEinrichtung(jetzt); else zeigeWarten(jetzt); }
  else if (tankanzeige)                       zeigeTank(jetzt, akkuWert);
  else if (modus == MODUS_POLIZEI)            zeigePolizei(jetzt);
  else if (modus >= MODUS_BLAU)               zeigeTeam(jetzt, modus);
  else if (quelle == RING_DABBLE)             zeigeDabble(jetzt);
  else                                        zeigeWeb(jetzt);

  ausgeben(jetzt);
}

void ringSignal(uint32_t farbe) {
  signalFarbe = farbe;
  signalStart = millis();
  signalAktiv = true;
}

void ringEinrichtung(bool an) { einrichtung = an; }
void ringAlarm(bool an)       { alarmAn = an; }
void ringAufnahme(int z)      { aufnahmeZustand = z; }
void ringKalibrierung(float f){ kalFortschritt = f; }

// Fortschrittsbalken fuer das WLAN-Update (wird direkt ausgegeben, der loop laeuft dann nicht)
void ringFortschritt(int prozent, uint32_t farbe) {
  float stand = constrain(prozent, 0, 100) * LED_RING_ANZAHL / 100.0f;
  leeren();
  for (int i = 0; i < LED_RING_ANZAHL; i++) {
    float h = constrain(stand - i, 0.0f, 1.0f);
    setzeFarbe(i, farbe, 0.15f + 0.85f * h);
  }
  hart = true;
  letzteAusgabe = 0;
  ausgeben(millis());
}

void ringHelligkeit(int h) { ring.setBrightness(h); }

// Aktuelle Farbe einer Ring-LED fuer die Webseite
uint32_t ringAnzeigeFarbe(int i) {
  i = constrain(i, 0, LED_RING_ANZAHL - 1);
  return ((uint32_t)aktuell[i][0] << 16) | ((uint32_t)aktuell[i][1] << 8) | (uint32_t)aktuell[i][2];
}
