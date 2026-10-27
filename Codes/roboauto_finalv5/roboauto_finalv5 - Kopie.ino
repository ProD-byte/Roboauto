/*
 * ====================================================================
 * 🚗 INTERAKTIVES ROBOAUTO - SKETCH NR. 1: LED-RING, BLE & FAHRBETRIEB🚀
 * ====================================================================
 * S3-RELEASE V1.59.2 (Teil 1: Setup & Finaler Achsen-Invertierungs-Fix)
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
#include "system_akku.h"
#include "system_gyro.h" 

Adafruit_NeoPixel ring(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel frontLights(NUM_FRONT_LEDS, FRONT_LIGHT_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel rearLights(NUM_REAR_LEDS, REAR_LIGHT_PIN, NEO_GRB + NEO_KHZ800);

unsigned long previousMillis = 0;
unsigned long serialMillis = 0;
int currentLed = 0;
long rainbowCycle = 0;
bool isConnected = false;        

bool ledsEnabled = true;       
bool startButtonPressed = false; 
bool selectButtonPressed = false;
bool triangleButtonPressed = false; 
bool fahrlichtEnabled = true; 
bool fernlichtAktiv = false; 
bool hupeAktiv = false; 
bool blinkerStatus = false;
int letzterLichtModus = 1; 

bool akkuKritischLeer = false; 
unsigned long ecoPiepMillis = 0;

unsigned long bremsStartMillis = 0;
bool warAmFahren = false;
const unsigned long BREMS_DAUER = 400; 

unsigned long blinkerMillis = 0;
int currentEffect = 0;          
const int TOTAL_EFFECTS = 5; 

bool triggerWelcome = false;
unsigned long welcomeStartMillis = 0;

// Hilfsfunktion: Stoppt beide Motoren komplett (Bremse)
void stoppeMotoren() {
  analogWrite(MOTOR_L_V, 0); analogWrite(MOTOR_L_R, 0);
  analogWrite(MOTOR_R_V, 0); analogWrite(MOTOR_R_R, 0);
}

// FINALER FIX: Richtungen komplett gespiegelt und invertiert!
void verarbeiteFahrantrieb(int xWert, int yWert) {
  if (xWert >= -1 && xWert <= 1 && yWert >= -1 && yWert <= 1) {
    stoppeMotoren();
    return;
  }

  // Von -xWert auf +xWert geändert -> Vorwärts/Rückwärts umgedreht!
  int basisGeschwindigkeit = map(xWert, -7, 7, -255, 255);
  
  // Von yWert auf -yWert geändert -> Links/Rechts umgedreht!
  int lenkEinfluss = map(-yWert, -7, 7, -150, 150);

  // Differentialsteuerung berechnen
  int linksSoll  = basisGeschwindigkeit + lenkEinfluss;
  int rechtsSoll = basisGeschwindigkeit - lenkEinfluss;

  // Werte auf das PWM-Maximum begrenzen
  linksSoll  = constrain(linksSoll, -255, 255);
  rechtsSoll = constrain(rechtsSoll, -255, 255);

  // LINKER MOTOR ANSTEUERN (Pins aus globals.h)
  if (linksSoll >= 0) {
    analogWrite(MOTOR_L_V, linksSoll);
    analogWrite(MOTOR_L_R, 0);
  } else {
    analogWrite(MOTOR_L_V, 0);
    analogWrite(MOTOR_L_R, abs(linksSoll));
  }

  // RECHTER MOTOR ANSTEUERN (Pins aus globals.h)
  if (rechtsSoll >= 0) {
    analogWrite(MOTOR_R_V, rechtsSoll);
    analogWrite(MOTOR_R_R, 0);
  } else {
    analogWrite(MOTOR_R_V, 0);
    analogWrite(MOTOR_R_R, abs(rechtsSoll));
  }
}

void verarbeiteSmartBordnetz() {
  int aktuellerAkku = analogRead(BATTERIE_MESS_PIN);
  if (aktuellerAkku > 2000) { akkuKritischLeer = false; return; }
  if (aktuellerAkku < 1500) {
    if (!akkuKritischLeer) { akkuKritischLeer = true; fernlichtAktiv = false; }
  } else if (aktuellerAkku > 1600) { akkuKritischLeer = false; }
}

void zeigeRingTankanzeige() {
  int akkuWert = analogRead(BATTERIE_MESS_PIN);
  clearRingNoShow();
  if (akkuWert > 2000) {
    for (int i = 0; i < NUM_LEDS; i++) ring.setPixelColor(i, ring.Color(0, 0, 255)); 
    ring.show(); return;
  }
  if (akkuWert < 1400) akkuWert = 1400;
  if (akkuWert > 1950) akkuWert = 1950;
  int leuchtLeds = map(akkuWert, 1400, 1950, 0, 8);
  uint32_t farbe = ring.Color(0, 0, 0);
  if (leuchtLeds >= 6) farbe = ring.Color(0, 255, 0);   
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
    if ((zeitVergangen >= 0 && zeitVergangen < 70) || (zeitVergangen >= 140 && zeitVergangen < 210)) {
      setFrontColor(FERNLICHT_WEISS); setRearColor(ROT_HELL);
    } else { setFrontColor(LICHT_AUS); setRearColor(LICHT_AUS); }
  }
  else if (zeitVergangen >= 280 && zeitVergangen < 1000) {
    uint32_t aktuelleFrontFarbe = fernlichtAktiv ? FERNLICHT_WEISS : (fahrlichtEnabled ? GELB_STROM_SPAR : LICHT_AUS);
    setFrontColor(aktuelleFrontFarbe); setRearColor(fahrlichtEnabled ? ROT_GEDIMMT : LICHT_AUS);
    float welle = sin((zeitVergangen - 280) * 0.0044); 
    int helligkeit = map(welle * 100, 0, 100, 0, 60);
    if (helligkeit < 0) helligkeit = 0;
    for (int i = 0; i < NUM_LEDS; i++) ring.setPixelColor(i, ring.Color(0, 0, helligkeit));
    ring.show();
  } else { triggerWelcome = false; clearRing(); }
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

  initGyroskop();
  Dabble.begin("RoboautoV3"); 
  Serial.println("Roboauto S3-Plattform bereit!");
}
void loop() {
  Dabble.processInput(); 
  unsigned long currentMillis = millis();

  checkBluetoothVerbindung();
  checkSystemTasten();
  verarbeiteSmartBordnetz(); 

  if (!isConnected) {
    stoppeMotoren(); 
    if (currentMillis - previousMillis >= 100) {
      previousMillis = currentMillis;
      for (int i = 0; i < NUM_LEDS; i++) ring.setPixelColor(i, ring.Color(0, 0, 25)); 
      ring.setPixelColor(currentLed, ring.Color(0, 0, 255)); 
      ring.show();
      setFrontColor(LICHT_AUS); setRearColor(LICHT_AUS);
      currentLed++; if (currentLed >= NUM_LEDS) currentLed = 0; 
    }
    return;
  }

  if (currentMillis - blinkerMillis >= 250) {
    blinkerMillis = currentMillis;
    blinkerStatus = !blinkerStatus;
  }

  int xWert = GamePad.getx_axis(); 
  int yWert = GamePad.gety_axis(); 

  verarbeiteFahrantrieb(xWert, yWert);

  bool blinkerAktivDieseRunde = GamePad.isSquarePressed() || GamePad.isCirclePressed();
  uint32_t aktuelleFrontFarbe = LICHT_AUS;
  if (fernlichtAktiv)          aktuelleFrontFarbe = FERNLICHT_WEISS;
  else if (fahrlichtEnabled)   aktuelleFrontFarbe = GELB_STROM_SPAR;

  if ((xWert > 1 || xWert < -1 || yWert > 1 || yWert < -1) && !blinkerAktivDieseRunde && !GamePad.isCrossPressed()) {
    warAmFahren = true; 
    
    if (triggerWelcome) { verarbeiteWelcomeEffekt(currentMillis); } 
    else {
      setFrontColor(aktuelleFrontFarbe);
      if (yWert < -1) {
        rearLights.setPixelColor(0, fahrlichtEnabled ? ROT_GEDIMMT : LICHT_AUS);
        rearLights.setPixelColor(1, WEISS_RUECK); 
      } else {
        rearLights.setPixelColor(0, fahrlichtEnabled ? ROT_GEDIMMT : LICHT_AUS);
        rearLights.setPixelColor(1, fahrlichtEnabled ? ROT_GEDIMMT : LICHT_AUS);
      }
      rearLights.show();
    }
    
    if (startButtonPressed) { zeigeRingTankanzeige(); }
    else if (!triggerWelcome && currentEffect == 0) { 
      clearRingNoShow();
      float winkel = atan2(xWert, yWert); 
      if (winkel < 0) winkel += 2 * M_PI;   
      float virtuelleLedPos = (winkel / (2 * M_PI)) * NUM_LEDS;
      int r = 0, g = 0, b = 0;
      if (yWert > 0)  { int a = map(yWert, 0, 7, 0, 255); r += a; g += a; } 
      if (yWert < -1) { g += map(abs(yWert), 0, 7, 0, 255); }               
      if (xWert > 0)  { r += map(xWert, 0, 7, 0, 255); }                    
      if (xWert < -1) { b += map(abs(xWert), 0, 7, 0, 255); }               
      if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;

      for (int i = 0; i < NUM_LEDS; i++) {
        float abstand = fabs(virtuelleLedPos - i);
        if (abstand > NUM_LEDS / 2) abstand = NUM_LEDS - abstand; 
        if (abstand < 1.0) {
          float f = 1.0 - abstand; ring.setPixelColor(i, ring.Color(r*f, g*f, b*f));
        }
      }
      ring.show(); 
    } 
    else if (!triggerWelcome) {
      if (currentEffect == 1)      updatePolizeiLicht(currentMillis);
      else if (currentEffect == 2) updateFluchtLicht(currentMillis);
      else if (currentEffect == 3) updateTeamBlau(currentMillis);
      else if (currentEffect == 4) updateTeamGruen(currentMillis);
    }
  }
  else if (!GamePad.isCrossPressed()) {
    if (!blinkerAktivDieseRunde) {
      if (triggerWelcome) { verarbeiteWelcomeEffekt(currentMillis); } 
      else {
        setFrontColor(aktuelleFrontFarbe);
        if (warAmFahren) { warAmFahren = false; bremsStartMillis = currentMillis; }
        if (currentMillis - bremsStartMillis < BREMS_DAUER) setRearColor(ROT_HELL);
        else                                                setRearColor(fahrlichtEnabled ? ROT_GEDIMMT : LICHT_AUS); 
      }
    } else { if (warAmFahren) { warAmFahren = false; } }
    
    if (startButtonPressed) { zeigeRingTankanzeige(); }
    else if (!triggerWelcome && currentEffect == 0) {
      if (GamePad.isTrianglePressed()) { setStaticColor(ring.Color(255, 255, 0)); } 
      else if (!blinkerAktivDieseRunde && !GamePad.isSquarePressed() && !GamePad.isCirclePressed()) {
        updateTestmodusLicht(currentMillis); 
      }
    }
    else if (!triggerWelcome) {
      if (currentEffect == 1)  { updatePolizeiLicht(currentMillis); }
      else if (currentEffect == 2)  { updateFluchtLicht(currentMillis); }
      else if (currentEffect == 3)  { updateTeamBlau(currentMillis); }
      else if (currentEffect == 4)  { updateTeamGruen(currentMillis); }
    }
  }
}
