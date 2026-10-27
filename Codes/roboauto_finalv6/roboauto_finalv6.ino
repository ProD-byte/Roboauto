/*
 * ====================================================================
 * 🚗 INTERAKTIVES ROBOAUTO - SKETCH NR. 1: LED-RING, BLE & FAHRBETRIEB🚀
 * ====================================================================
 * S3-RELEASE V1.60 (V6: Bugfixes & Aufräumen)
 *
 * Änderungen gegenüber V5:
 *  - Rückfahrlicht reagiert auf Rückwärtsfahrt (x-Achse) statt auf Lenken
 *  - Warnblinker wird nicht mehr vom normalen Licht überschrieben
 *  - Hupe stoppt beim Loslassen von X auch im Eco-Modus
 *  - Motoren stoppen bei Verbindungsabbruch sofort
 *  - BLE wird nach Abbruch nicht mehr doppelt initialisiert
 *  - Akku-Messung geglättet (nur noch eine Messung alle 20 ms)
 *  - Jeder Licht-Effekt hat einen eigenen Takt
 *  - Effekt-Auswahl und Lichtlogik zusammengefasst
 */

#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE  

#include <DabbleESP32.h>
#include <Adafruit_NeoPixel.h>
#include <BLEDevice.h> 

#include "globals.h"
#include "polizei.h"
#include "fluechtiger.h"
#include "blinker.h"
#include "testmodus.h"
#include "teams.h"
#include "system_ble.h"
#include "system_gyro.h" 

Adafruit_NeoPixel ring(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel frontLights(NUM_FRONT_LEDS, FRONT_LIGHT_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel rearLights(NUM_REAR_LEDS, REAR_LIGHT_PIN, NEO_GRB + NEO_KHZ800);

int currentLed = 0;
bool isConnected = false;        

bool startButtonPressed = false; 
bool selectButtonPressed = false;
bool fahrlichtEnabled = true; 
bool fernlichtAktiv = false; 
bool hupeAktiv = false; 
bool blinkerStatus = false;
int letzterLichtModus = 1; 

bool akkuKritischLeer = false; 
int akkuWert = 0;                       // geglätteter ADC-Rohwert (0..4095)

unsigned long bremsStartMillis = 0;
bool warAmFahren = false;
const unsigned long BREMS_DAUER = 400; 

unsigned long blinkerMillis = 0;
int currentEffect = 0;          
const int TOTAL_EFFECTS = 5; 

bool triggerWelcome = false;
unsigned long welcomeStartMillis = 0;

// DEBUG: 1 = Fahrwerte alle 200 ms im Seriellen Monitor ausgeben (115200 Baud)
#define DEBUG_FAHRT 1
int dbgLinks = 0, dbgRechts = 0;

// Joystick-Totzone: Werte von -1..+1 gelten als "Stick in der Mitte"
bool istAusgelenkt(int wert) { return wert > 1 || wert < -1; }

// FINALER FIX: Richtungen komplett gespiegelt und invertiert!
void verarbeiteFahrantrieb(int xWert, int yWert) {
  if (!istAusgelenkt(xWert) && !istAusgelenkt(yWert)) {
    stoppeMotoren();
    dbgLinks = dbgRechts = 0;
    return;
  }

  // x-Achse = Vorwärts/Rückwärts
  int basisGeschwindigkeit = map(xWert, -7, 7, -255, 255);
  
  // y-Achse = Lenkung (invertiert)
  int lenkEinfluss = map(-yWert, -7, 7, -150, 150);

  // Differentialsteuerung berechnen und auf das PWM-Maximum begrenzen
  int linksSoll  = constrain(basisGeschwindigkeit + lenkEinfluss, -255, 255);
  int rechtsSoll = constrain(basisGeschwindigkeit - lenkEinfluss, -255, 255);

  dbgLinks = linksSoll; dbgRechts = rechtsSoll;

  // LINKER MOTOR
  analogWrite(MOTOR_L_V, linksSoll  > 0 ?  linksSoll  : 0);
  analogWrite(MOTOR_L_R, linksSoll  < 0 ? -linksSoll  : 0);

  // RECHTER MOTOR
  analogWrite(MOTOR_R_V, rechtsSoll > 0 ?  rechtsSoll : 0);
  analogWrite(MOTOR_R_R, rechtsSoll < 0 ? -rechtsSoll : 0);
}

// V6: Akku nur alle 20 ms messen und glätten (gleitender Mittelwert),
// damit Tankanzeige und Eco-Modus nicht flackern.
void messeAkku(unsigned long currentMillis) {
  static unsigned long letzteMessung = 0;
  static float geglaettet = -1;
  if (currentMillis - letzteMessung < 20) return;
  letzteMessung = currentMillis;

  int roh = analogRead(BATTERIE_MESS_PIN);
  if (geglaettet < 0) geglaettet = roh;          // erste Messung direkt übernehmen
  else                geglaettet += (roh - geglaettet) * 0.1f;
  akkuWert = (int)geglaettet;
}

// Eco-Modus mit Hysterese: unter 1500 an, erst über 1600 wieder aus
void verarbeiteSmartBordnetz() {
  if (akkuWert < 1500) {
    if (!akkuKritischLeer) { akkuKritischLeer = true; fernlichtAktiv = false; }
  } else if (akkuWert > 1600) {
    akkuKritischLeer = false;
  }
}

void zeigeRingTankanzeige() {
  clearRingNoShow();
  if (akkuWert > 2000) {                       // Lädt / Netzteil
    setStaticColor(ring.Color(0, 0, 255));
    return;
  }
  int anzeigeWert = constrain(akkuWert, 1400, 1950);
  int leuchtLeds = map(anzeigeWert, 1400, 1950, 0, NUM_LEDS);
  uint32_t farbe;
  if (leuchtLeds >= 6)      farbe = ring.Color(0, 255, 0);   
  else if (leuchtLeds >= 3) farbe = ring.Color(180, 70, 0);  
  else {
    float pulsWelle = sin(millis() * 0.005); 
    int roteHelligkeit = map(pulsWelle * 100, -100, 100, 20, 255);
    farbe = ring.Color(roteHelligkeit, 0, 0);
    if (leuchtLeds == 0) leuchtLeds = 1; 
  }
  for (int i = 0; i < leuchtLeds; i++) ring.setPixelColor(i, farbe);
  ring.show();
}

void verarbeiteWelcomeEffekt(unsigned long currentMillis) {
  if (!triggerWelcome) return;
  unsigned long zeitVergangen = currentMillis - welcomeStartMillis;
  if (zeitVergangen < 280) {
    // Lichthupe: zweimal kurz aufblitzen
    if (zeitVergangen < 70 || (zeitVergangen >= 140 && zeitVergangen < 210)) {
      setFrontColor(FERNLICHT_WEISS); setRearColor(ROT_HELL);
    } else { setFrontColor(LICHT_AUS); setRearColor(LICHT_AUS); }
  }
  else if (zeitVergangen < 1000) {
    setFrontColor(getAktuelleHintergrundLichtFarbe());
    setRearColor(getAktuellesRuecklichtFarbe());
    float welle = sin((zeitVergangen - 280) * 0.0044); 
    int helligkeit = map(welle * 100, 0, 100, 0, 60);
    if (helligkeit < 0) helligkeit = 0;
    setStaticColor(ring.Color(0, 0, helligkeit));
  } else { triggerWelcome = false; clearRing(); }
}

// Modus 1-4 auf dem Ring
void aktualisiereRingEffekt(unsigned long currentMillis) {
  switch (currentEffect) {
    case 1: updatePolizeiLicht(currentMillis); break;
    case 2: updateFluchtLicht(currentMillis);  break;
    case 3: updateTeamBlau(currentMillis);     break;
    case 4: updateTeamGruen(currentMillis);    break;
  }
}

// Modus 0 während der Fahrt: Ring zeigt Richtung und Stärke des Joysticks
void zeigeRichtungsRing(int xWert, int yWert) {
  clearRingNoShow();
  float winkel = atan2(xWert, yWert); 
  if (winkel < 0) winkel += 2 * M_PI;   
  float virtuelleLedPos = (winkel / (2 * M_PI)) * NUM_LEDS;
  int r = 0, g = 0, b = 0;
  if (yWert > 0)  { int a = map(yWert, 0, 7, 0, 255); r += a; g += a; } 
  if (yWert < -1) { g += map(-yWert, 0, 7, 0, 255); }               
  if (xWert > 0)  { r += map(xWert, 0, 7, 0, 255); }                    
  if (xWert < -1) { b += map(-xWert, 0, 7, 0, 255); }               
  r = min(r, 255); g = min(g, 255); b = min(b, 255);

  for (int i = 0; i < NUM_LEDS; i++) {
    float abstand = fabs(virtuelleLedPos - i);
    if (abstand > NUM_LEDS / 2) abstand = NUM_LEDS - abstand; 
    if (abstand < 1.0) {
      float f = 1.0 - abstand;
      ring.setPixelColor(i, ring.Color(r * f, g * f, b * f));
    }
  }
  ring.show(); 
}

// Lauflicht, solange kein Handy verbunden ist
void zeigeWarteAnimation(unsigned long currentMillis) {
  static unsigned long letzterSchritt = 0;
  if (currentMillis - letzterSchritt < 100) return;
  letzterSchritt = currentMillis;
  for (int i = 0; i < NUM_LEDS; i++) ring.setPixelColor(i, ring.Color(0, 0, 25)); 
  ring.setPixelColor(currentLed, ring.Color(0, 0, 255)); 
  ring.show();
  setFrontColor(LICHT_AUS); setRearColor(LICHT_AUS);
  currentLed = (currentLed + 1) % NUM_LEDS; 
}

void setup() {
  Serial.begin(115200); 
  delay(1500); 
  
  ring.begin();        ring.setBrightness(40);        ring.show();
  frontLights.begin(); frontLights.setBrightness(40); frontLights.show();
  rearLights.begin();  rearLights.setBrightness(40);  rearLights.show();
  
  // Motoren-Pins als Ausgänge deklarieren
  pinMode(MOTOR_L_V, OUTPUT); pinMode(MOTOR_L_R, OUTPUT);
  pinMode(MOTOR_R_V, OUTPUT); pinMode(MOTOR_R_R, OUTPUT);
  stoppeMotoren(); 

  analogSetAttenuation(ADC_11db); 
  pinMode(BATTERIE_MESS_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT); 
  messeAkku(millis());

  initGyroskop();
  Dabble.begin("RoboautoV3"); 
  Serial.println("Roboauto S3-Plattform bereit!");
#if DEBUG_FAHRT
  // 1=Einschalten, 3=Software, 4=Absturz, 9=Brownout (Spannungseinbruch)
  Serial.printf("Grund fuer letzten Neustart: %d\n", (int)esp_reset_reason());
#endif
}

void loop() {
  Dabble.processInput(); 
  unsigned long currentMillis = millis();

  checkBluetoothVerbindung();
  messeAkku(currentMillis);
  verarbeiteSmartBordnetz(); 

  if (!isConnected) {
    stoppeMotoren(); 
    zeigeWarteAnimation(currentMillis);
    return;
  }

  // Blinker-Takt (250 ms an / 250 ms aus) - VOR den Tasten, damit alle Blinker denselben Takt nutzen
  if (currentMillis - blinkerMillis >= 250) {
    blinkerMillis = currentMillis;
    blinkerStatus = !blinkerStatus;
  }

  checkSystemTasten();

  int xWert = GamePad.getx_axis(); 
  int yWert = GamePad.gety_axis(); 
  verarbeiteFahrantrieb(xWert, yWert);

#if DEBUG_FAHRT
  static unsigned long letzteAusgabe = 0;
  if (currentMillis - letzteAusgabe >= 200) {
    letzteAusgabe = currentMillis;
    Serial.printf("x=%d y=%d  links=%d rechts=%d  akku=%d  verbunden=%d\n",
                  xWert, yWert, dbgLinks, dbgRechts, akkuWert, isConnected);
  }
#endif

  // Während X gedrückt ist, steuert checkSystemTasten() Ring, Licht und Ton
  if (GamePad.isCrossPressed()) return;

  bool blinkerTaste = GamePad.isSquarePressed() || GamePad.isCirclePressed();
  bool faehrt       = istAusgelenkt(xWert) || istAusgelenkt(yWert);
  bool fahrModus    = faehrt && !blinkerTaste;
  bool rueckwaerts  = xWert < -1;     // V6: x-Achse ist Vorwärts/Rückwärts (siehe verarbeiteFahrantrieb)

  // --- Bremslicht-Erkennung ---
  if (fahrModus) {
    warAmFahren = true;
  } else if (warAmFahren) {
    warAmFahren = false;
    if (!blinkerTaste) bremsStartMillis = currentMillis;   // Bremslicht nur beim echten Anhalten
  }

  // --- Front- und Rücklichter ---
  // V6: Blinker UND Warnblinker haben Vorrang - sonst wurde der Warnblinker sofort überschrieben
  if (!blinkerTaste && !warnblinkerAktiv) {
    if (triggerWelcome) {
      verarbeiteWelcomeEffekt(currentMillis);
    } else {
      uint32_t ruecklicht = getAktuellesRuecklichtFarbe();
      setFrontColor(getAktuelleHintergrundLichtFarbe());
      if (fahrModus) {
        rearLights.setPixelColor(0, ruecklicht);
        rearLights.setPixelColor(1, rueckwaerts ? WEISS_RUECK : ruecklicht);   // Rückfahrscheinwerfer
        rearLights.show();
      } else if (currentMillis - bremsStartMillis < BREMS_DAUER) {
        setRearColor(ROT_HELL);                                                // Bremslicht
      } else {
        setRearColor(ruecklicht);
      }
    }
  }

  // --- LED-Ring ---
  if (startButtonPressed)            zeigeRingTankanzeige();
  else if (triggerWelcome)           { /* Ring gehört gerade dem Welcome-Effekt */ }
  else if (currentEffect != 0)       aktualisiereRingEffekt(currentMillis);
  else if (fahrModus)                zeigeRichtungsRing(xWert, yWert);
  else if (GamePad.isTrianglePressed()) setStaticColor(ring.Color(255, 255, 0));
  else if (!blinkerTaste)            updateTestmodusLicht(currentMillis);
}
