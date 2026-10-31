#include <Arduino.h>
#include "config.h"
#include "antrieb.h"
#include "gyro.h"
#include "einstellungen.h"

static int sollLinks = 0;
static int sollRechts = 0;
static int korrektur = 0;
static bool haeltKurs = false;   // true = Geradeauslauf aktiv, Zielrichtung gemerkt
static float zielWinkel = 0;

// --- 180-Grad-Wende ---
static bool wendeAktiv = false;
static float wendeZiel = 0;
static unsigned long wendeStart = 0;
static unsigned long wendeEnde = 0;      // fuer die Nachlauf-Messung

// --- Motortest (Assistent auf der Webseite) ---
static int testSeite = -1;               // -1 = kein Test, 0 = links, 1 = rechts
static unsigned long testEnde = 0;

// Einen Motorausgang ansteuern: positiv = vor, negativ = zurueck
static void setzeMotor(int pinVor, int pinRueck, int wert) {
  wert = constrain(wert, -PWM_MAX, PWM_MAX);
  analogWrite(pinVor,   wert > 0 ?  wert : 0);
  analogWrite(pinRueck, wert < 0 ? -wert : 0);
}

// sollLinks/sollRechts an die Motoren geben.
// Umkehren und Tauschen kommen aus den Einstellungen (Webseite) - so kann man
// falsch angeschlossene Motoren ohne Umloeten und ohne neuen Code korrigieren.
static void ausgeben() {
  int l = einst().motorLInv ? -sollLinks  : sollLinks;
  int r = einst().motorRInv ? -sollRechts : sollRechts;
  if (einst().motorTausch) { int t = l; l = r; r = t; }
  setzeMotor(MOTOR_L_VOR, MOTOR_L_RUECK, l);
  setzeMotor(MOTOR_R_VOR, MOTOR_R_RUECK, r);
}

void antriebStarten() {
  pinMode(MOTOR_L_VOR, OUTPUT);
  pinMode(MOTOR_L_RUECK, OUTPUT);
  pinMode(MOTOR_R_VOR, OUTPUT);
  pinMode(MOTOR_R_RUECK, OUTPUT);
  antriebStopp();
}

void antriebStopp() {
  wendeAktiv = false;
  sollLinks = sollRechts = 0;
  korrektur = 0;
  haeltKurs = false;
  testSeite = -1;
  ausgeben();
}

bool antriebWendeStarten() {
  if (!gyroBereit()) {
    Serial.println(">> Wende nicht moeglich: kein Gyro");
    return false;
  }
  wendeAktiv = true;
  wendeZiel  = gyroWinkel() + WENDE_WINKEL;     // links herum = Winkel wird groesser
  wendeStart = millis();
  Serial.println(">> 180-Grad-Wende START");
  return true;
}

bool antriebWendeAktiv() { return wendeAktiv; }

// Ein Schritt der Wende: auf der Stelle links drehen, kurz vor dem Ziel langsamer
static void wendeSchritt() {
  float rest = wendeZiel - gyroWinkel();

  // Vorhalt: so weit dreht das Auto nach dem Abschalten noch nach (Schwung)
  float nachlauf = gyroDrehrate() * WENDE_VORHALT;

  if (rest - nachlauf <= WENDE_TOLERANZ || millis() - wendeStart > WENDE_MAX_ZEIT) {
    Serial.printf(">> 180-Grad-Wende STOPP (Rest %.1f, erwarteter Nachlauf %.1f Grad)\n", rest, nachlauf);
    antriebStopp();
    wendeEnde = millis();
    return;
  }

  int tempo = WENDE_TEMPO_MAX;
  if (rest < WENDE_BREMS_BEREICH) {
    tempo = map((int)rest, 0, (int)WENDE_BREMS_BEREICH, WENDE_TEMPO_MIN, WENDE_TEMPO_MAX);
  }

  // Links drehen: linkes Rad rueckwaerts, rechtes vorwaerts
  sollLinks  = -tempo;
  sollRechts =  tempo;
  korrektur  = 0;
  haeltKurs  = false;
  ausgeben();
}

void antriebFahren(int x, int y) {
  // Nach der Wende: wenn das Auto ausgerollt ist, das echte Ergebnis melden
  if (wendeEnde != 0 && millis() - wendeEnde > 700) {
    wendeEnde = 0;
    float fehler = gyroWinkel() - wendeZiel;
    Serial.printf(">> 180-Grad-Wende ERGEBNIS: %+.1f Grad %s\n", fehler,
                  fehler > 0 ? "(zu weit)" : "(zu kurz)");
  }

  bool stickInMitte = abs(x) <= TOTZONE && abs(y) <= TOTZONE;

  if (wendeAktiv) {
    if (stickInMitte) { wendeSchritt(); return; }
    wendeAktiv = false;                          // Joystick bewegt -> Wende abbrechen
    Serial.println(">> 180-Grad-Wende ABGEBROCHEN (Joystick)");
  }

  if (stickInMitte) {
    antriebStopp();
    return;
  }

  // y = Stick vor(+)/zurueck(-), x = Stick rechts(+)/links(-)
  const int maxTempo = einst().tempo;                 // aus den Einstellungen (Webseite)
  int tempo = map(y, -JOYSTICK_MAX, JOYSTICK_MAX, -maxTempo, maxTempo);

  // Lenkstaerke je nach Tempo: im Stand kraeftig (auf der Stelle drehen),
  // bei voller Fahrt sanft (gut steuerbar). Dazwischen fliessend.
  int lenkMax = map(abs(tempo), 0, maxTempo, einst().lenkungStand, einst().lenkung);
  int lenkung = map(x, -JOYSTICK_MAX, JOYSTICK_MAX, -lenkMax, lenkMax);

  // --- Geradeauslauf: kein Lenkbefehl, aber Fahrt -> Richtung mit dem Gyro halten ---
  // Gyro: Linksdrehung = positiver Winkel. Positive Lenkung dreht nach rechts.
  bool geradeaus = einst().geradeaus && gyroBereit() && abs(x) <= TOTZONE && abs(y) > TOTZONE;
  if (geradeaus) {
    if (!haeltKurs) {               // gerade erst losgefahren/Lenkung losgelassen
      haeltKurs = true;
      zielWinkel = gyroWinkel();
    }
    float abweichung = gyroWinkel() - zielWinkel;
    float k = GERADEAUS_KP * abweichung + GERADEAUS_KD * gyroDrehrate();
    korrektur = constrain((int)k, -GERADEAUS_MAX_KORR, GERADEAUS_MAX_KORR);
    lenkung = korrektur;
  } else {
    haeltKurs = false;              // beim Lenken folgt das Auto einfach dem Joystick
    korrektur = 0;
  }

  // Rechts lenken = linkes Rad schneller, rechtes langsamer
  // Obergrenze: normal das Hoechsttempo, beim Drehen im Stand darf die Lenkung mehr
  int grenze = max(maxTempo, lenkMax);
  sollLinks  = constrain(tempo + lenkung, -grenze, grenze);
  sollRechts = constrain(tempo - lenkung, -grenze, grenze);

  ausgeben();
}

int antriebLinks()  { return sollLinks; }
int antriebRechts() { return sollRechts; }
int antriebKorrektur() { return korrektur; }


// Motoren direkt setzen (-255..255) - fuer selbststaendige Programme (Figuren, Aufnahme)
void antriebDirekt(int links, int rechts) {
  wendeAktiv = false;
  haeltKurs  = false;
  korrektur  = 0;
  sollLinks  = constrain(links,  -PWM_MAX, PWM_MAX);
  sollRechts = constrain(rechts, -PWM_MAX, PWM_MAX);
  ausgeben();
}


// Motortest: eine Seite kurz vorwaerts drehen (seite 0 = links, 1 = rechts)
void antriebTestStarten(int seite) {
  antriebStopp();
  testSeite = seite ? 1 : 0;
  testEnde  = millis() + MOTORTEST_MS;
  Serial.printf(">> Motortest: %s vorwaerts\n", testSeite ? "rechts" : "links");
}

int antriebTestSeite() { return testSeite; }

void antriebTestUpdate() {
  if (testSeite < 0) return;
  if ((long)(millis() - testEnde) >= 0) { antriebStopp(); return; }
  sollLinks  = testSeite == 0 ? MOTORTEST_TEMPO : 0;
  sollRechts = testSeite == 1 ? MOTORTEST_TEMPO : 0;
  ausgeben();
}
