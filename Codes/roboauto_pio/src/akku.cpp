#include <Arduino.h>
#include "config.h"
#include "akku.h"

static float geglaettet = -1;
static bool eco = false;
static unsigned long letzteMessung = 0;

void akkuStarten() {
  analogSetAttenuation(ADC_11db);     // Messbereich bis ca. 3,1 V (wie V5)
  pinMode(AKKU_PIN, INPUT);
  geglaettet = analogRead(AKKU_PIN);  // erster Wert direkt
}

void akkuVerlaufUpdate();

void akkuUpdate() {
  akkuVerlaufUpdate();
  unsigned long jetzt = millis();
  if (jetzt - letzteMessung < 20) return;
  letzteMessung = jetzt;

  // Gleitender Mittelwert: Ausreisser und Flackern werden geglaettet
  geglaettet += (analogRead(AKKU_PIN) - geglaettet) * 0.1f;

  // Eco-Modus mit Hysterese, damit er an der Grenze nicht hin- und herspringt
  int wert = akkuWert();
  if (!eco && wert < AKKU_ECO_AN) {
    eco = true;
    Serial.printf(">> ECO-MODUS AN (Akku=%d)\n", wert);
  } else if (eco && wert > AKKU_ECO_AUS) {
    eco = false;
    Serial.printf(">> ECO-MODUS AUS (Akku=%d)\n", wert);
  }
}

int  akkuWert() { return (int)geglaettet; }
bool akkuEco()  { return eco; }

int akkuProzent() {
  int wert = constrain(akkuWert(), AKKU_ANZEIGE_MIN, AKKU_ANZEIGE_MAX);
  return map(wert, AKKU_ANZEIGE_MIN, AKKU_ANZEIGE_MAX, 0, 100);
}
bool akkuUsb() { return akkuWert() > AKKU_LAEDT; }

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
