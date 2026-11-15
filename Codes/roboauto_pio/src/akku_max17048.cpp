#include <Arduino.h>
#include <Wire.h>
#include <Preferences.h>
#include "config.h"
#include "akku.h"

// ============================================================
//  Akku ueber den Akkuwaechter MAX17048 oder den aelteren MAX17043 (I2C, gleicher Bus wie das Gyro,
//  beide auf Adresse 0x36). Der Chip rechnet den Ladestand selbst aus - genauer als eine
//  Spannungsmessung, weil er auch Last und Alterung beruecksichtigt.
//  Unterschiede: der MAX17043 zaehlt die Spannung anders und kennt keine Lade-/Entladerate -
//  die rechnet die Firmware dann selbst aus dem Ladestand.
// ============================================================

constexpr uint8_t REG_SPANNUNG = 0x02;   // MAX17048: 78,125 uV pro Bit, MAX17043: obere 12 Bit, 1,25 mV pro Bit
constexpr uint8_t REG_LADUNG   = 0x04;   // oberes Byte = %, unteres Byte = 1/256 %
constexpr uint8_t REG_VERSION  = 0x08;   // MAX17043: 0x0002/0x0003, MAX17048: 0x001x
constexpr uint8_t REG_RATE     = 0x16;   // nur MAX17048: Lade-/Entladerate, 0,208 %/Stunde pro Bit (mit Vorzeichen)

static bool  gefunden = false;
static bool  alt = false;                // MAX17043 (ohne Raten-Register)
static float rateProzent = -1;           // MAX17043: Ladestand beim letzten Raten-Messpunkt
static int   rateMillivolt = 0;          // MAX17043: Spannung beim letzten Raten-Messpunkt
static unsigned long rateZeit = 0;
static int   millivolt = 0;
static float prozent = 0;
static float rate = 0;                   // %/Stunde, + = laedt
static bool  eco = false;
static unsigned long letzteMessung = 0;

// "Voll" lernt das Auto selbst: hat es geladen und bleibt die Spannung danach lange gleich, ist die
// Ladeschaltung fertig - der Rohwert des Chips in diesem Moment gilt kuenftig als 100 %.
// So passt die Anzeige zu jedem Akku (und zu einem Akku, der mit der Zeit weniger aufnimmt).
constexpr unsigned long VOLL_RUHE_MS = 20UL * 60000;   // so lange ohne Anstieg = fertig geladen
constexpr int VOLL_MIN_MV = 3900;                       // darunter ist der Akku sicher nicht voll
static float vollRaw = AKKU_VOLL_PROZENT;               // Rohwert, der als 100 % gilt (gespeichert)
static bool  hatGeladen = false;                        // seit dem letzten Lernen geladen?
static int   ruheMv = 0;
static unsigned long ruheSeit = 0;

static void vollLernen(unsigned long jetzt) {
  if (rate > AKKU_LAEDT_RATE) hatGeladen = true;
  if (!hatGeladen || millivolt < VOLL_MIN_MV) { ruheSeit = 0; return; }
  if (ruheSeit == 0 || millivolt > ruheMv + 8) { ruheMv = millivolt; ruheSeit = jetzt; return; }   // steigt noch
  if (millivolt < ruheMv - 30) { hatGeladen = false; ruheSeit = 0; return; }   // faellt: Kabel ab / faehrt
  if (jetzt - ruheSeit < VOLL_RUHE_MS) return;
  float neu = constrain(prozent, 50.0f, 100.0f);
  hatGeladen = false; ruheSeit = 0;
  if (fabsf(neu - vollRaw) < 0.5f) return;
  vollRaw = neu;
  Preferences p;
  p.begin("akku", false);
  p.putFloat("vollRaw", vollRaw);
  p.end();
  Serial.printf(">> Akku: fertig geladen bei %d mV - %.1f %% (Chip) gelten jetzt als 100 %%\n", millivolt, vollRaw);
}

static bool lies16(uint8_t reg, uint16_t& wert) {
  Wire.beginTransmission(AKKU_ADRESSE);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(AKKU_ADRESSE, (uint8_t)2) != 2) return false;
  wert = (Wire.read() << 8) | Wire.read();
  return true;
}

static void messen() {
  uint16_t v, l, r;
  gefunden = lies16(REG_SPANNUNG, v) && lies16(REG_LADUNG, l);
  if (!gefunden) return;
  millivolt = alt ? (int)((v >> 4) * 1.25f) : (int)(v * 78.125f / 1000.0f);
  prozent = constrain(l / 256.0f, 0.0f, 100.0f);
  if (!alt) { rate = lies16(REG_RATE, r) ? (int16_t)r * 0.208f : 0; return; }
  // MAX17043: Rate selbst aus dem Ladestand (alle 60 s ein Messpunkt, %/Stunde).
  // Ganz unten und ganz oben im Akku bewegt sich der Ladestand kaum - steigt die Spannung
  // spuerbar (ab 5 mV pro Minute), haengt trotzdem USB dran und der Akku laedt.
  unsigned long jetzt = millis();
  if (rateProzent < 0) { rateProzent = prozent; rateMillivolt = millivolt; rateZeit = jetzt; }
  else if (jetzt - rateZeit >= 60000) {
    rate = (prozent - rateProzent) * 3600000.0f / (jetzt - rateZeit);
    float mvProMinute = (millivolt - rateMillivolt) * 60000.0f / (jetzt - rateZeit);
    if (mvProMinute >= 5 && rate <= AKKU_LAEDT_RATE) rate = AKKU_LAEDT_RATE + 1;
    rateProzent = prozent; rateMillivolt = millivolt; rateZeit = jetzt;
  }
}

void akkuStarten() {
  Wire.begin(GYRO_SDA_PIN, GYRO_SCL_PIN);   // schon offen, wenn das Gyro zuerst gestartet hat
  uint16_t version = 0;
  gefunden = lies16(REG_VERSION, version);
  Preferences p;
  p.begin("akku", true);
  vollRaw = p.getFloat("vollRaw", AKKU_VOLL_PROZENT);
  p.end();
  if (gefunden) {
    alt = version < 0x0010;
    messen();
    Serial.printf(">> Akkuwaechter (%s, Version %04X) gefunden: %d mV, %.0f %% (voll = %.0f %%)\n",
                  alt ? "MAX17043" : "MAX17048", version, millivolt, prozent, vollRaw);
  } else {
    Serial.println(">> FEHLER: Akkuwaechter (MAX17048/MAX17043) nicht gefunden - zeige USB an");
  }
}

void akkuVerlaufUpdate();

void akkuUpdate() {
  akkuVerlaufUpdate();
  unsigned long jetzt = millis();
  if (jetzt - letzteMessung < 500) return;   // der Chip rechnet ohnehin nur ca. 1x pro Sekunde neu
  letzteMessung = jetzt;
  messen();
  if (gefunden) vollLernen(jetzt);

  // Eco-Modus mit Hysterese, damit er an der Grenze nicht hin- und herspringt
  int p = akkuProzent();
  if (!eco && !akkuUsb() && p < AKKU_ECO_AN) {
    eco = true;
    Serial.printf(">> ECO-MODUS AN (Akku=%d %%)\n", p);
  } else if (eco && (akkuUsb() || p > AKKU_ECO_AUS)) {
    eco = false;
    Serial.printf(">> ECO-MODUS AUS (Akku=%d %%)\n", p);
  }
}

int  akkuWert()    { return millivolt; }
bool akkuEco()     { return eco; }
// Angezeigter Ladestand: auf "voll geladen" bei diesem Akku umgerechnet (gelernt, siehe vollLernen)
int  akkuProzent() { return (int)min(100.0f, prozent * 100.0f / vollRaw + 0.5f); }
bool akkuUsb()     { return !gefunden || rate > AKKU_LAEDT_RATE; }

// ---------- Verlauf: alle AKKU_VERLAUF_MS ein Prozentwert, die letzten AKKU_VERLAUF_PUNKTE ----------
static uint8_t verlauf[AKKU_VERLAUF_PUNKTE];
static int verlaufAnzahl = 0, verlaufPos = 0;
static unsigned long letzterPunkt = 0;

void akkuVerlaufUpdate() {
  unsigned long jetzt = millis();
  if (verlaufAnzahl > 0 && jetzt - letzterPunkt < AKKU_VERLAUF_MS) return;
  letzterPunkt = jetzt;
  verlauf[verlaufPos] = akkuUsb() ? 255 : akkuProzent();   // 255 = USB
  verlaufPos = (verlaufPos + 1) % AKKU_VERLAUF_PUNKTE;
  if (verlaufAnzahl < AKKU_VERLAUF_PUNKTE) verlaufAnzahl++;
}

String akkuVerlaufJson() {
  String s = "{\"intervall\":" + String(AKKU_VERLAUF_MS / 1000) + ",\"werte\":[";
  int start = (verlaufPos - verlaufAnzahl + AKKU_VERLAUF_PUNKTE) % AKKU_VERLAUF_PUNKTE;
  for (int i = 0; i < verlaufAnzahl; i++) {
    if (i) s += ',';
    s += verlauf[(start + i) % AKKU_VERLAUF_PUNKTE];
  }
  return s + "]}";
}
