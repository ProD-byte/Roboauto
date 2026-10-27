// ============================================================
//  ROBOAUTO - Neuaufbau mit PlatformIO
//  Schritt 1: Bluetooth-Verbindung + Fahren
//  Schritt 2: LED-Ring (blau = warten, Regenbogen = verbunden) + Buzzer
//  Schritt 3: Front-, Rueck-, Brems- und Rueckfahrlicht
//  Schritt 4: Blinker, Warnblinker, Hupe
//  Schritt 6: Akkuanzeige (Start halten) + Eco-Modus
//  Schritt 7: Gyroskop + Geradeauslauf + 180-Grad-Wende (X doppelt)
//  Schritt 10: halbe Geschwindigkeit (MAX_TEMPO in config.h)
//  Schritt 11: WLAN + Web-Steuerung (http://roboauto.local) + Updates ueber WLAN
//  Schritt 12: Polizei-Modus (Select / Knopf), Sirene auf X
//  Schritt 13: Team Blau / Gruen / Gelb / Rot (Ring-Farbspiel + eigener Hupenton)
//  Schritt 14: WLAN-Einrichtung per Handy (eigenes WLAN "Roboauto-Setup")
//  Schritt 15: Crash- und Umkipp-Erkennung (Alarm, Warnblinker, Motoren aus)
//  Schritt 16: Figuren fahren (Quadrat, Acht, Slalom) - Start 2x / Webseite
//  Schritt 17: Fahrweg aufnehmen/abspielen (Start+Viereck / Start+Kreis / Webseite)
//  Balancieren (Experiment): Start+Dreieck / Knopf auf der Webseite
// ============================================================
#include <Arduino.h>

#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE
#include <DabbleESP32.h>
#include <BLEDevice.h>

#include "config.h"
#include "antrieb.h"
#include "ledring.h"
#include "buzzer.h"
#include "lichter.h"
#include "akku.h"
#include "gyro.h"
#include "klick.h"
#include "autoupdate.h"
#include "musik.h"
#include "web.h"
#include "sicherheit.h"
#include "figuren.h"
#include "aufnahme.h"
#include "balance.h"
#include "einstellungen.h"

static bool verbunden = false;     // Handy ODER Webseite (fuer Ring, Lichter, Fahren)
static bool bleVorher = false;
static bool webVorher = false;

// Licht-Modus (siehe enum Modus in config.h): Select in Dabble / Knoepfe auf der Webseite
static int  modus = MODUS_NORMAL;

// Gyro neu kalibrieren: alles anhalten, dann im Hintergrund messen
static void kalibrierungStarten() {
  if (!gyroBereit()) { buzzerWarnung(); return; }
  figurAbbrechen();
  balanceAus();
  if (abspielenLaeuft()) abspielenUmschalten();
  antriebStopp();
  buzzerEin();
  gyroKalibrierungStarten();
}

static void setzeModus(int neu) {
  modus = (neu + MODUS_ANZAHL) % MODUS_ANZAHL;
  buzzerTick();
  Serial.printf(">> Modus: %s\n", MODUS_NAME[modus]);
}
static bool selectVorher = false;
static KlickTaste startKlick;      // Start: halten = Tankanzeige, 2x klicken = naechste Figur fahren
static int  naechsteFigur = 0;
static KlickTaste kreuzKlick;      // X: halten = Hupe, 2x klicken = 180-Grad-Wende
static bool hupeGesperrt = false;  // nach dem Doppelklick bis zum Loslassen nicht hupen

// Fragt den BLE-Server, ob ein Handy verbunden ist
static bool handyVerbunden() {
  BLEServer* server = BLEDevice::getServer();
  return server != nullptr && server->getConnectedCount() > 0;
}

void setup() {
  Serial.begin(115200);
  // WICHTIG: Ist das USB-Kabel gesteckt, aber kein Monitor offen, liest niemand
  // die Debug-Ausgaben. Ohne diese Zeile wartet der ESP dann bei jeder Ausgabe
  // und alles laeuft wie in Zeitlupe. 0 = nie warten, notfalls Text verwerfen.
  Serial.setTxTimeoutMs(0);
  delay(1000);

  autoUpdateStart();                      // nach einem Auto-Update: startet die neue Version sauber? sonst zurueck
  einstellungenLaden();                   // gespeicherte Einstellungen (vor allem anderen)
  antriebStarten();
  ringStarten();
  buzzerStarten();
  buzzerStartklang(ringStartanimation);    // "Hochfahren": Startklang + Ring-Animation
  lichtStarten();
  akkuStarten();
  if (gyroStarten()) gyroKalibrieren();   // Auto beim Einschalten 1 s still stehen lassen
  Dabble.begin(BLE_NAME);
  webStarten();                           // WLAN + Webseite (oder Einrichtungs-WLAN)
  aufnahmeStarten();                      // gespeicherten Fahrweg laden

  Serial.println();
  Serial.println("=== Roboauto - Schritt 31: Traumwelt ===");
  Serial.printf("Grund fuer Neustart: %d (1=Einschalten, 4=Absturz, 9=Spannungseinbruch)\n",
                (int)esp_reset_reason());
}

void loop() {
  Dabble.processInput();

  akkuUpdate();
  gyroUpdate();
  webUpdate();

  // --- WLAN-Ergebnis nach dem Start anzeigen ---
  switch (webEreignis()) {
    case 1: ringSignal(0x00B4FF); buzzerWlanOk();     break;   // tuerkis + drei steigende Blips
    case 2: ringSignal(0xFF2000); buzzerWlanFehler(); break;   // rot + tiefer Ton
    case 3: ringSignal(0xFF8000); buzzerWarnung();    break;   // orange: Einrichtung offen
  }
  static bool rueckfallGemeldet = false;
  if (autoUpdateZurueckgesetzt() && !rueckfallGemeldet && millis() > 3000) {   // Auto-Update ging schief -> alte Version laeuft
    rueckfallGemeldet = true;
    ringSignal(0xFF8000); buzzerWarnung();
  }

  // --- Verbindung ueberwachen ---
  // Handy (Bluetooth) und Webseite getrennt ueberwachen.
  // Nur Dabble/Handy spielt die Verbinden-/Trennen-Melodie - die Webseite meldet sich
  // nur im Monitor, damit es nicht staendig piept, wenn der Browser kurz haengt.
  bool ble = handyVerbunden();
  bool web = webVerbunden();
  if (ble != bleVorher) {
    bleVorher = ble;
    Serial.println(ble ? ">> Handy (Dabble) verbunden" : ">> Handy (Dabble) getrennt");
    if (ble) buzzerVerbunden(); else buzzerGetrennt();
  }
  if (web != webVorher) {
    webVorher = web;
    Serial.println(web ? ">> Webseite verbunden" : ">> Webseite getrennt (kein Lebenszeichen)");
  }
  verbunden = ble || web;

  // --- Fahren (nur mit Verbindung, sonst Stopp) ---
  int x = 0, y = 0;
  int wt = webTasten();                                  // gehaltene Tasten auf der Webseite
  bool startTaste = (ble && GamePad.isStartPressed()) || (wt & WEB_TANK);
  if (startKlick.pruefe(ble && GamePad.isStartPressed(), millis()) == 2) {
    // Dabble: Start 2x = Figuren der Reihe nach (Quadrat, Acht, Slalom)
    if (figurStarten(naechsteFigur)) { naechsteFigur = (naechsteFigur + 1) % FIGUR_ANZAHL; buzzerEin(); }
    else buzzerWarnung();
  }

  if (webFaehrt()) {
    // Wird gerade ueber die Webseite gefahren, hat das Vorrang
    x = webX();
    y = webY();
  } else if (ble) {
    x = GamePad.getx_axis();
    y = GamePad.gety_axis();

    // Pfeiltasten (Digital-Modus in Dabble) wirken wie der Joystick.
    // Im Joystick-Modus liefern sie false, im Digital-Modus ist der Joystick 0.
    if (GamePad.isUpPressed())    y =  DPAD_STAERKE;
    if (GamePad.isDownPressed())  y = -DPAD_STAERKE;
    if (GamePad.isRightPressed()) x =  DPAD_STAERKE;
    if (GamePad.isLeftPressed())  x = -DPAD_STAERKE;
  }

  // --- Dabble: Start halten + Viereck = Aufnahme an/aus, Start halten + Kreis = Abspielen ---
  static bool kombiVorher = false;
  bool startGehalten = ble && GamePad.isStartPressed();
  bool vier = startGehalten && GamePad.isSquarePressed();
  bool kreisK = startGehalten && GamePad.isCirclePressed();
  bool dreiK = startGehalten && GamePad.isTrianglePressed();      // Start + Dreieck = Balancieren
  bool kreuzK = startGehalten && GamePad.isCrossPressed();         // Start + X = Gyro neu kalibrieren
  if ((vier || kreisK || dreiK || kreuzK) && !kombiVorher) {
    if (vier)        aufnahmeUmschalten();
    else if (kreisK) abspielenUmschalten();
    else if (dreiK)  { if (balanceUmschalten()) buzzerEin(); else buzzerAus(); }
    else             kalibrierungStarten();
  }
  kombiVorher = vier || kreisK || dreiK || kreuzK;

  // --- Gyro-Kalibrierung im Hintergrund ---
  if (gyroKalibriertGerade()) { x = 0; y = 0; }      // waehrenddessen nicht fahren
  ringKalibrierung(gyroKalibriertGerade() ? gyroKalibrierFortschritt() : -1);
  switch (gyroKalibrierErgebnis()) {
    case 1: buzzerWlanOk(); ringSignal(0x40FF80); break;   // fertig: gruenes Signal
    case 2: buzzerWarnung(); ringSignal(0xFF4000); break;  // bewegt: orange, alte Werte bleiben
  }
  if (!verbunden) balanceAus();

  // --- Fahrweg aufnehmen / abspielen (ersetzt beim Abspielen x/y) ---
  if (!verbunden && abspielenLaeuft()) abspielenUmschalten();   // Verbindung weg -> stoppen
  aufnahmeUpdate(x, y);
  ringAufnahme(aufnahmeLaeuft() ? 1 : abspielenLaeuft() ? 2 : 0);

  // --- Crash- und Umkipp-Erkennung ---
  bool stickInMitte = abs(x) <= TOTZONE && abs(y) <= TOTZONE;
  // Beim Balancieren steht das Auto schraeg -> Umkipp-Erkennung waere falsch; Balance hat eigene Umfall-Abschaltung
  switch (sicherheitUpdate(verbunden && !balanceAktiv(), stickInMitte, antriebLinks(), antriebRechts())) {
    case 1:                                   // Aufprall
    case 2:                                   // umgekippt
      antriebStopp();
      buzzerAlarm();
      lichtBefehl(WARNBLINKER_AUTO_AN);
      break;
    case 3:                                   // wieder aufgestellt / fahrbereit
      lichtBefehl(WARNBLINKER_AUTO_AUS);
      break;
  }
  ringAlarm(!sicherheitFahrenErlaubt());

  // Figur laeuft? Joystick bewegen, Verbindung weg oder Alarm -> abbrechen
  if (figurAktiv() && (!stickInMitte || !verbunden || !sicherheitFahrenErlaubt())) {
    figurAbbrechen();
    Serial.println(">> Figur abgebrochen");
  }

  if (balanceAktiv() && figurAktiv()) figurAbbrechen();
  if (antriebTestSeite() >= 0 && !stickInMitte) antriebStopp();   // Joystick bricht den Motortest ab
  if (!verbunden || !sicherheitFahrenErlaubt()) antriebStopp();
  else if (antriebTestSeite() >= 0)              antriebTestUpdate();
  else if (balanceAktiv())                       balanceUpdate(x, y);
  else if (figurAktiv())                         figurUpdate();
  else                                           antriebFahren(x, y);

  // --- LED-Ring: Verbindungsstatus ---
  // Regenbogen = Dabble, tuerkiser Komet = Webseite, blau = niemand. Start halten = Tankanzeige
  RingQuelle quelle = ble ? RING_DABBLE : (web ? RING_WEB : RING_NIEMAND);
  // --- Modus umschalten: Select (Dabble) ---
  bool select = ble && GamePad.isSelectPressed();
  if (select && !selectVorher) {
    setzeModus(modus + 1);
  }
  selectVorher = select;
  webSetzeModus(modus);

  ringEinrichtung(webEinrichtung());                 // orange atmend = WLAN einrichten
  ringMusik(musikNummer() >= 0, musikTon(), musikNotenNummer());
  ringStatus(quelle, startTaste, akkuWert(), modus);

  // --- Start + Select 3 s halten (Dabble) = WLAN neu einrichten ---
  static unsigned long kombiSeit = 0;
  if (ble && GamePad.isStartPressed() && GamePad.isSelectPressed()) {
    if (kombiSeit == 0) kombiSeit = millis();
    else if (millis() - kombiSeit > WLAN_VERGESSEN_HALTEN_MS) {
      kombiSeit = 0;
      buzzerWlanFehler();
      webWlanVergessen();
    }
  } else {
    kombiSeit = 0;
  }

  // --- X: Doppelklick = 180-Grad-Wende, halten = Hupe ---
  bool kreuz = ble && GamePad.isCrossPressed() && !GamePad.isStartPressed();   // Start+X = Kalibrieren, nicht hupen
  if (kreuzKlick.pruefe(kreuz, millis()) == 2) {
    hupeGesperrt = true;                     // der zweite Klick soll nicht hupen
    if (!antriebWendeStarten()) buzzerWarnung();
  }
  if (!kreuz) hupeGesperrt = false;

  // --- Einmal-Aktionen von der Webseite ---
  int aktion = webAktion();
  switch (aktion) {
    case WEB_AKTION_LICHT:        lichtBefehl(LICHT_UMSCHALTEN);       break;
    case WEB_AKTION_FERNLICHT:    lichtBefehl(FERNLICHT_UMSCHALTEN);   break;
    case WEB_AKTION_WARN:         lichtBefehl(WARNBLINKER_UMSCHALTEN); break;
    case WEB_AKTION_WENDE:        if (!antriebWendeStarten()) buzzerWarnung(); break;
    case WEB_AKTION_MODUS_WEITER: setzeModus(modus + 1); break;
    default:
      if (aktion == WEB_AKTION_KALIBRIEREN) kalibrierungStarten();
      if (aktion == WEB_AKTION_MUSIK_STOPP) musikStopp();
      if (aktion >= WEB_AKTION_MUSIK && aktion < WEB_AKTION_MUSIK + musikAnzahl()) musikStarten(aktion - WEB_AKTION_MUSIK);
      if (aktion == WEB_AKTION_MOTORTEST || aktion == WEB_AKTION_MOTORTEST + 1) {
        if (figurAktiv()) figurAbbrechen();
        if (abspielenLaeuft()) abspielenUmschalten();
        balanceAus();
        antriebTestStarten(aktion - WEB_AKTION_MOTORTEST);
      }
      if (aktion == WEB_AKTION_BALANCE) { if (balanceUmschalten()) buzzerEin(); else buzzerAus(); }
      if (aktion == WEB_AKTION_AUFNAHME)  aufnahmeUmschalten();
      if (aktion == WEB_AKTION_ABSPIELEN) abspielenUmschalten();
      if (aktion >= WEB_AKTION_FIGUR && aktion < WEB_AKTION_FIGUR + FIGUR_ANZAHL) {
        if (figurAktiv()) figurAbbrechen();                 // nochmal klicken = stoppen
        else if (figurStarten(aktion - WEB_AKTION_FIGUR)) buzzerEin();
        else buzzerWarnung();
      }
      if (aktion >= WEB_AKTION_MODUS_DIREKT && aktion < WEB_AKTION_MODUS_DIREKT + MODUS_ANZAHL)
        setzeModus(aktion - WEB_AKTION_MODUS_DIREKT);
      break;
  }
  webSetzeModus(modus);

  // X / Hupe-Knopf: Polizei = Sirene + Frontblitzer, Teams = eigener Hupenton
  bool hupeGedrueckt = (kreuz && !hupeGesperrt) || (wt & WEB_HUPE);
  if (modus == MODUS_POLIZEI) { buzzerHupe(false); buzzerSirene(hupeGedrueckt); }
  else                        { buzzerSirene(false); buzzerHupe(hupeGedrueckt, TEAM_HUPE_HZ[modus]); }
  lichtBlitzer(modus == MODUS_POLIZEI && hupeGedrueckt);
  musikUpdate();
  buzzerUpdate();

  // --- Lichter, Blinker, Warnblinker ---
  LichtTasten tasten = {};
  if (ble) {
    tasten.dreieck = GamePad.isTrianglePressed();
    tasten.viereck = GamePad.isSquarePressed();
    tasten.kreis   = GamePad.isCirclePressed();
  }
  if (startGehalten) { tasten.viereck = false; tasten.kreis = false; tasten.dreieck = false; }   // Start-Kombis nicht als Blinker/Licht
  if (wt & WEB_BLINK_L) tasten.viereck = true;   // Blinker von der Webseite (gehalten)
  if (wt & WEB_BLINK_R) tasten.kreis   = true;
  // Beim Balancieren zappeln die Motoren staendig -> fuer Brems-/Rueckfahrlicht als 'steht' werten
  lichtUpdate(verbunden, tasten, balanceAktiv() ? 0 : antriebLinks(), balanceAktiv() ? 0 : antriebRechts(), akkuEco());

  // --- Auto-Update: nur wenn das Auto eine Weile unbenutzt steht und der Akku reicht ---
  static unsigned long letzteNutzung = 0;
  if (ble || webFaehrt() || antriebLinks() != 0 || antriebRechts() != 0 || figurAktiv() || balanceAktiv() ||
      musikNummer() >= 0 || aufnahmeLaeuft() || abspielenLaeuft() || !sicherheitFahrenErlaubt())
    letzteNutzung = millis();
  bool ruhig = millis() - letzteNutzung > UPDATE_RUHE_MS && (akkuUsb() || akkuProzent() >= UPDATE_MIN_AKKU);
  autoUpdateLoop(webWlanDa(), ruhig);

  // --- Debug-Ausgabe alle 250 ms ---
  static unsigned long letzteAusgabe = 0;
  if (millis() - letzteAusgabe >= 250) {
    letzteAusgabe = millis();
    Serial.printf("verbunden=%d  x=%d y=%d  links=%d rechts=%d  akku=%d%s  gyro=%6.1f Grad/s  winkel=%6.1f Grad  korr=%d  stoss=%.1fg\n",
                  verbunden, x, y, antriebLinks(), antriebRechts(),
                  akkuWert(), akkuEco() ? " ECO" : "",
                  gyroDrehrate(), gyroWinkel(), antriebKorrektur(), sicherheitMaxStoss());
  }
}
