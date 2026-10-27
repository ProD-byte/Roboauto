#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "lichter.h"
#include "buzzer.h"
#include "klick.h"
#include "einstellungen.h"

static Adafruit_NeoPixel front(FRONT_LED_ANZAHL, FRONT_LED_PIN, NEO_GRB + NEO_KHZ800);
static Adafruit_NeoPixel rear(REAR_LED_ANZAHL,  REAR_LED_PIN,  NEO_GRB + NEO_KHZ800);

constexpr int FRONT_LED_RECHTS = 1 - FRONT_LED_LINKS;
constexpr int REAR_LED_RECHTS  = 1 - REAR_LED_LINKS;

// --- Lichtzustand ---
static bool lichtAn        = true;    // Fahrlicht beim Start an (wie V5)
static bool fernlicht      = false;
static bool merkeFernlicht = false;   // "Memory": war beim Ausschalten Fernlicht an?
static bool warnblinker    = false;

static int offenerBefehl = 0;         // von der Webseite
static bool blitzer = false;          // Polizei-Frontblitzer
static bool warnAutomatisch = false;  // Warnblinker wurde automatisch (Crash) eingeschaltet

static KlickTaste dreieckKlick;
static KlickTaste kreisKlick;

// --- Blinker-Takt ---
static int  blinkRichtung = 0;        // 0 = aus, 1 = links, 2 = rechts, 3 = warnblinker
static unsigned long blinkStart = 0;

// --- Bremslicht ---
static bool warAmFahren = false;
static unsigned long bremsStart = 0;

// --- Weiche Lichter (wie bei modernen LED-Scheinwerfern) ---
// Jede der 4 LEDs gleitet in ca. 100 ms zur Zielfarbe: Licht "dimmt" an und aus,
// Blinker faden weich. Nur das Bremslicht geht sofort an (Sicherheit!).
enum { V_L = 0, V_R, H_L, H_R };
static float lichtIst[4][3];
static uint32_t lichtZiel[4] = {0, 0, 0, 0};
static unsigned long letzteLichtAusgabe = 0;

static void setzeLichter(uint32_t vorneL, uint32_t vorneR, uint32_t hintenL, uint32_t hintenR) {
  lichtZiel[V_L] = vorneL; lichtZiel[V_R] = vorneR;
  lichtZiel[H_L] = hintenL; lichtZiel[H_R] = hintenR;

  unsigned long jetzt = millis();
  if (jetzt - letzteLichtAusgabe < 12) return;
  letzteLichtAusgabe = jetzt;

  bool vorneGeaendert = false, hintenGeaendert = false;
  for (int n = 0; n < 4; n++) {
    float soll[3] = { (float)((lichtZiel[n] >> 16) & 0xFF), (float)((lichtZiel[n] >> 8) & 0xFF), (float)(lichtZiel[n] & 0xFF) };
    bool sofort = (lichtZiel[n] == FARBE_BREMSLICHT) || blitzer;   // Bremslicht + Polizei-Blitzer ohne Verzoegerung
    for (int c = 0; c < 3; c++) {
      float alt = lichtIst[n][c];
      lichtIst[n][c] = sofort ? soll[c] : alt + (soll[c] - alt) * 0.18f;
      if (fabs(soll[c] - lichtIst[n][c]) < 0.6f) lichtIst[n][c] = soll[c];
      if ((int)alt != (int)lichtIst[n][c]) { if (n < 2) vorneGeaendert = true; else hintenGeaendert = true; }
    }
  }
  auto farbe = [](int n) {
    return Adafruit_NeoPixel::Color((uint8_t)lichtIst[n][0], (uint8_t)lichtIst[n][1], (uint8_t)lichtIst[n][2]);
  };
  if (vorneGeaendert) {
    front.setPixelColor(FRONT_LED_LINKS,  farbe(V_L));
    front.setPixelColor(FRONT_LED_RECHTS, farbe(V_R));
    front.show();
  }
  if (hintenGeaendert) {
    rear.setPixelColor(REAR_LED_LINKS,  farbe(H_L));
    rear.setPixelColor(REAR_LED_RECHTS, farbe(H_R));
    rear.show();
  }
}

void lichtStarten() {
  front.begin(); front.setBrightness(einst().lichtHell); front.clear(); front.show();
  rear.begin();  rear.setBrightness(einst().lichtHell);  rear.clear();  rear.show();
}

// Dreieck 1x: Licht an/aus (merkt sich, ob Fernlicht an war)
static void lichtUmschalten() {
  if (lichtAn) {
    merkeFernlicht = fernlicht;
    lichtAn = false;
    fernlicht = false;
    Serial.println(">> Licht AUS");
    buzzerAus();
  } else {
    lichtAn = true;
    fernlicht = merkeFernlicht;
    Serial.println(fernlicht ? ">> Licht AN (Fernlicht)" : ">> Licht AN");
    buzzerEin();
  }
}

// Dreieck 2x: Fernlicht an/aus (schaltet das Licht dabei ggf. mit ein)
static void fernlichtUmschalten() {
  fernlicht = !fernlicht;
  if (fernlicht) lichtAn = true;
  Serial.println(fernlicht ? ">> Fernlicht AN" : ">> Fernlicht AUS");
  if (fernlicht) buzzerEin(); else buzzerAus();
}

void lichtBefehl(LichtBefehl befehl) { offenerBefehl = befehl; }
void lichtBlitzer(bool an) { blitzer = an; }
bool lichtIstAn()          { return lichtAn; }
bool lichtIstFernlicht()   { return fernlicht; }
bool lichtIstWarnblinker() { return warnblinker; }

void lichtUpdate(bool verbunden, const LichtTasten& tasten, int links, int rechts, bool eco) {
  unsigned long jetzt = millis();

  if (!verbunden) {
    setzeLichter(FARBE_AUS, FARBE_AUS, FARBE_AUS, FARBE_AUS);
    fernlicht = false;            // wie V5: nach Verbindung wieder normales Fahrlicht
    warnblinker = false;
    blinkRichtung = 0;
    dreieckKlick.reset();
    kreisKlick.reset();
    warAmFahren = false;
    return;
  }

  // --- Befehl von der Webseite -> wie ein Klick auf Dreieck bzw. Doppelklick auf Kreis ---
  int befehl = offenerBefehl;
  offenerBefehl = 0;
  if (befehl == WARNBLINKER_AUTO_AN && !warnblinker) {
    warnblinker = true;
    warnAutomatisch = true;
    Serial.println(">> Warnblinker AN (automatisch)");
  }
  if (befehl == WARNBLINKER_AUTO_AUS && warnAutomatisch) {
    warnblinker = false;
    warnAutomatisch = false;
    Serial.println(">> Warnblinker AUS (automatisch)");
  }
  if (befehl == WARNBLINKER_UMSCHALTEN) {
    warnAutomatisch = false;           // von Hand geschaltet -> gehoert wieder dem Fahrer
    warnblinker = !warnblinker;
    if (warnblinker) buzzerEin(); else buzzerAus();
    Serial.println(warnblinker ? ">> Warnblinker AN" : ">> Warnblinker AUS");
  }

  // --- Tasten auswerten ---
  int d = dreieckKlick.pruefe(tasten.dreieck, jetzt);
  if (befehl == LICHT_UMSCHALTEN)     d = 1;
  if (befehl == FERNLICHT_UMSCHALTEN) d = 2;
  if (d == 1) lichtUmschalten();
  if (d == 2) {
    if (eco && !fernlicht) {        // Eco-Modus: Fernlicht einschalten verboten
      Serial.println(">> ECO-MODUS: Fernlicht gesperrt!");
      buzzerWarnung();
    } else {
      fernlichtUmschalten();
    }
  }

  // Eco-Modus: ein evtl. eingeschaltetes Fernlicht automatisch abschalten
  if (eco && fernlicht) {
    fernlicht = false;
    Serial.println(">> ECO-MODUS: Fernlicht abgeschaltet");
  }
  if (eco) merkeFernlicht = false;

  if (kreisKlick.pruefe(tasten.kreis, jetzt) == 2) {
    warnblinker = !warnblinker;
    warnAutomatisch = false;
    if (warnblinker) buzzerEin(); else buzzerAus();
    Serial.println(warnblinker ? ">> Warnblinker AN" : ">> Warnblinker AUS");
  }

  // --- Blinker: Warnblinker hat Vorrang, sonst Taste halten ---
  int richtung = 0;
  if (warnblinker)        richtung = 3;
  else if (tasten.viereck) richtung = 1;
  else if (tasten.kreis)   richtung = 2;

  if (richtung != blinkRichtung) {   // neuer Blinker -> sofort mit "an" beginnen
    blinkRichtung = richtung;
    blinkStart = jetzt;
  }
  bool blinkPhaseAn = ((jetzt - blinkStart) / BLINK_TAKT) % 2 == 0;
  bool blinkL = blinkPhaseAn && (blinkRichtung == 1 || blinkRichtung == 3);
  bool blinkR = blinkPhaseAn && (blinkRichtung == 2 || blinkRichtung == 3);

  // --- Fahrzustand aus den Motorwerten ---
  bool faehrt      = (links != 0 || rechts != 0);
  bool rueckwaerts = (links + rechts) < 0;      // Drehen auf der Stelle zaehlt nicht

  if (faehrt) {
    warAmFahren = true;
  } else if (warAmFahren) {
    warAmFahren = false;
    bremsStart = jetzt;
  }
  bool bremst = !faehrt && bremsStart != 0 && (jetzt - bremsStart < BREMSLICHT_DAUER);

  // --- Grundfarben ---
  uint32_t vorne  = !lichtAn ? FARBE_AUS : (fernlicht ? FARBE_FERNLICHT : FARBE_FAHRLICHT);
  uint32_t hinten = lichtAn ? FARBE_RUECKLICHT : FARBE_AUS;

  uint32_t vL = vorne, vR = vorne, hL = hinten, hR = hinten;

  if (bremst) {                       // Bremslicht leuchtet auch bei ausgeschaltetem Licht
    hL = hR = FARBE_BREMSLICHT;
  } else if (rueckwaerts) {           // Rueckfahrlicht ebenfalls immer
    if (REAR_LED_RUECKFAHR == REAR_LED_LINKS) hL = FARBE_RUECKFAHR;
    else                                      hR = FARBE_RUECKFAHR;
  }

  // --- Polizei-Blitzer: Scheinwerfer blitzen abwechselnd weiss/blau ---
  if (blitzer) {
    bool phase = (jetzt / 100) % 2;
    vL = phase ? FARBE_FERNLICHT : 0x0000FF;
    vR = phase ? 0x0000FF : FARBE_FERNLICHT;
  }

  // --- Blinker ueberlagert die jeweilige Seite (hat Vorrang) ---
  if (blinkL) { vL = FARBE_BLINKER; hL = FARBE_BLINKER; }
  if (blinkR) { vR = FARBE_BLINKER; hR = FARBE_BLINKER; }

  setzeLichter(vL, vR, hL, hR);
}

void lichtHelligkeit(int h) {
  front.setBrightness(h); rear.setBrightness(h);
  letzteLichtAusgabe = 0;
  for (int n = 0; n < 4; n++) lichtIst[n][0] = lichtIst[n][1] = lichtIst[n][2] = -1;   // neu ausgeben erzwingen
}

// Aktuelle (gefadete) Farbe einer Lampe fuer die Webseite: 0 = vorne links, 1 = vorne rechts,
// 2 = hinten links, 3 = hinten rechts
uint32_t lichtAnzeigeFarbe(int n) {
  n = constrain(n, 0, 3);
  return ((uint32_t)max(0.0f, lichtIst[n][0]) << 16) | ((uint32_t)max(0.0f, lichtIst[n][1]) << 8) | (uint32_t)max(0.0f, lichtIst[n][2]);
}
