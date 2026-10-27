#include <Arduino.h>
#include "config.h"
#include "balance.h"
#include "gyro.h"
#include "antrieb.h"

static bool  aktiv = false;
static bool  umgefallen = false;
static float sollWinkel = 0;
static float integral = 0;
static int   letzteLinks = 0, letzteRechts = 0;   // fuer die sanfte Rampe

bool balanceAktiv() { return aktiv; }

void balanceAus() {
  if (!aktiv) return;
  aktiv = false;
  antriebStopp();
  Serial.println(">> BALANCIEREN AUS");
}

bool balanceUmschalten() {
  if (aktiv) { balanceAus(); return false; }
  if (!gyroBereit()) {
    Serial.println(">> Balancieren nicht moeglich: kein Gyro");
    return false;
  }
  aktiv = true;
  umgefallen = false;
  integral = 0;
  letzteLinks = letzteRechts = 0;
  sollWinkel = gyroNeigungY();          // aktuelle Lage = Balancierpunkt
  Serial.printf(">> BALANCIEREN AN - Balancierpunkt Y=%.1f Grad\n", sollWinkel);
  return true;
}

void balanceUpdate(int x, int y) {
  if (!aktiv) return;
  static uint32_t letzteMessung = 0;
  if (gyroMessNummer() == letzteMessung) return;   // Regler laeuft im Takt der Messung (200 Hz)
  letzteMessung = gyroMessNummer();

  // Kippachse ist Y (im Test gemessen): Schnauze hoch = Y negativ, Schnauze runter = Y positiv.
  float neigung = gyroNeigungY();

  // Joystick vor/zurueck: Sollwinkel etwas nach vorne/hinten verschieben -> Auto faehrt los.
  float fahrVersatz = BALANCE_FAHR_WINKEL * y / (float)JOYSTICK_MAX;
  float ziel = sollWinkel + fahrVersatz;

  // "Nase kippt nach vorne" = positiv
  float kippVorne = (neigung - ziel) * BALANCE_RICHTUNG;
  float kippRate  = gyroRateY() * BALANCE_RICHTUNG;

  // Umgefallen? -> Motoren aus, bis das Auto wieder aufgestellt ist
  if (fabs(neigung - sollWinkel) > BALANCE_UMFALL_WINKEL) {
    if (!umgefallen) {
      umgefallen = true;
      Serial.println(">> Umgefallen - Motoren aus (wieder aufrichten, dann geht es weiter)");
    }
    antriebDirekt(0, 0);
    integral = 0;
    letzteLinks = letzteRechts = 0;
    return;
  }
  if (umgefallen && fabs(neigung - sollWinkel) < 5.0f) {
    umgefallen = false;
    Serial.println(">> Wieder aufgerichtet - balanciere weiter");
  }
  if (umgefallen) { antriebDirekt(0, 0); return; }

  // PID-Regler: kippt die Nase nach vorne, faehrt das Auto nach vorne darunter
  integral = constrain(integral + kippVorne * 0.005f, -50.0f, 50.0f);
  float tempo = BALANCE_KP * kippVorne + BALANCE_KD * kippRate + BALANCE_KI * integral;

  // Totzone der Motoren ueberwinden
  if (tempo > 1)       tempo += BALANCE_MIN_PWM;
  else if (tempo < -1) tempo -= BALANCE_MIN_PWM;
  else                 tempo = 0;

  int lenkung = x * BALANCE_LENKUNG / JOYSTICK_MAX;
  int zielL = constrain((int)tempo + lenkung, -BALANCE_MAX_PWM, BALANCE_MAX_PWM);
  int zielR = constrain((int)tempo - lenkung, -BALANCE_MAX_PWM, BALANCE_MAX_PWM);

  // Rampe: pro Takt nur begrenzt aendern. Ein Sprung von +180 auf -180 dauert so
  // ca. 90 ms statt 0 ms -> deutlich kleinere Stromspitzen beim Richtungswechsel.
  letzteLinks  += constrain(zielL - letzteLinks,  -BALANCE_MAX_SPRUNG, BALANCE_MAX_SPRUNG);
  letzteRechts += constrain(zielR - letzteRechts, -BALANCE_MAX_SPRUNG, BALANCE_MAX_SPRUNG);
  antriebDirekt(letzteLinks, letzteRechts);
}
