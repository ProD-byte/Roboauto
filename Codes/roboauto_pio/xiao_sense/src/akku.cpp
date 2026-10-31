#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "akku.h"

// ============================================================
//  Akku ueber den Akkuwaechter MAX17048 (I2C, gleicher Bus wie das Gyro).
//  Der Chip rechnet den Ladestand selbst aus - genauer als eine Spannungsmessung,
//  weil er auch Last und Alterung beruecksichtigt.
// ============================================================

constexpr uint8_t REG_SPANNUNG = 0x02;   // 78,125 uV pro Bit
constexpr uint8_t REG_LADUNG   = 0x04;   // oberes Byte = %, unteres Byte = 1/256 %
constexpr uint8_t REG_VERSION  = 0x08;
constexpr uint8_t REG_RATE     = 0x16;   // Lade-/Entladerate, 0,208 %/Stunde pro Bit (mit Vorzeichen)

static bool  gefunden = false;
static int   millivolt = 0;
static float prozent = 0;
static float rate = 0;                   // %/Stunde, + = laedt
static bool  eco = false;
static unsigned long letzteMessung = 0;

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
  millivolt = (int)(v * 78.125f / 1000.0f);
  prozent = constrain(l / 256.0f, 0.0f, 100.0f);
  rate = lies16(REG_RATE, r) ? (int16_t)r * 0.208f : 0;
}

void akkuStarten() {
  Wire.begin(GYRO_SDA_PIN, GYRO_SCL_PIN);   // schon offen, wenn das Gyro zuerst gestartet hat
  uint16_t version = 0;
  gefunden = lies16(REG_VERSION, version);
  if (gefunden) {
    messen();
    Serial.printf(">> Akkuwaechter (MAX17048, Version %04X) gefunden: %d mV, %.0f %%\n", version, millivolt, prozent);
  } else {
    Serial.println(">> FEHLER: Akkuwaechter (MAX17048) nicht gefunden - zeige USB an");
  }
}

void akkuVerlaufUpdate();

void akkuUpdate() {
  akkuVerlaufUpdate();
  unsigned long jetzt = millis();
  if (jetzt - letzteMessung < 500) return;   // der Chip rechnet ohnehin nur ca. 1x pro Sekunde neu
  letzteMessung = jetzt;
  messen();

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
int  akkuProzent() { return (int)(prozent + 0.5f); }
bool akkuUsb()     { return !gefunden || rate > AKKU_LAEDT_RATE; }
bool akkuGefunden(){ return gefunden; }

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
