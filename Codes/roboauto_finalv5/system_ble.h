#ifndef SYSTEM_BLE_H
#define SYSTEM_BLE_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <DabbleESP32.h>
#include "globals.h"
#include "fluechtiger.h"
#include "polizei.h"
#include "teams.h"
#include "blinker.h"
#include "system_gyro.h" 

extern bool isConnected;
extern bool ledsEnabled;
extern bool startButtonPressed;
extern bool selectButtonPressed;
extern bool triangleButtonPressed; 
extern int currentEffect;
extern const int TOTAL_EFFECTS;
extern int currentLed;
extern bool fahrlichtEnabled;
extern bool fernlichtAktiv;
extern bool hupeAktiv;
extern bool blinkerStatus;
extern int letzterLichtModus;

// Signal-Variablen für den welcome-Effekt im Hauptloop
extern bool triggerWelcome;
extern unsigned long welcomeStartMillis;

// Signal aus dem Hauptsketch, ob der Akku kritisch leer ist (Eco-Modus)
extern bool akkuKritischLeer; 

// Variablen für die Doppelklick-Erkennung
static bool kreisDavorGedrueckt = false;
static unsigned long letzterKreisKlickMillis = 0;
static bool warnblinkerAktiv = false;

static bool dreieckDavorGedrueckt = false;
static unsigned long letzterDreieckKlickMillis = 0;
static unsigned long dreieckKlickTimer = 0;
static bool einfacherKlickAusstehend = false;

inline void checkBluetoothVerbindung() {
  int connectedDevices = BLEDevice::getServer()->getConnectedCount();

  if (connectedDevices > 0) {
    if (!isConnected) {
      isConnected = true;
      clearRing();
      setFrontColor(LICHT_AUS);
      setRearColor(LICHT_AUS);
      warnblinkerAktiv = false;
      fernlichtAktiv = false;
      letzterLichtModus = 1; 
      Serial.println("Handy gekoppelt!");
      
      tone(BUZZER_PIN, 1000, 100); delay(120);
      tone(BUZZER_PIN, 1500, 150);
      noTone(BUZZER_PIN);
      
      triggerWelcome = true;
      welcomeStartMillis = millis();

      // Das Gyroskop exakt beim Koppeln kalibrieren
      kalibriereGyroskop(); 
    }
  } else {
    if (isConnected) {
      isConnected = false;
      clearRing();
      currentLed = 0;
      setFrontColor(LICHT_AUS);
      setRearColor(LICHT_AUS);
      warnblinkerAktiv = false;
      fernlichtAktiv = false;
      triggerWelcome = false;
      
      tone(BUZZER_PIN, 600, 200); delay(220);
      tone(BUZZER_PIN, 400, 300); delay(300);
      noTone(BUZZER_PIN); 
      
      resetFluchtSound();
      delay(100); 
      Dabble.begin("RoboautoV3"); 
    }
  }
}

inline void checkSystemTasten() {
  if (!isConnected) return;
  unsigned long currentMillis = millis();

  if (GamePad.isStartPressed()) {
    startButtonPressed = true;
  } else {
    startButtonPressed = false;
  }

  if (GamePad.isSelectPressed()) {
    if (!selectButtonPressed) {
      currentEffect = (currentEffect + 1) % TOTAL_EFFECTS; 
      selectButtonPressed = true;
      clearRing(); 
    }
  } else { selectButtonPressed = false; }

  if (GamePad.isTrianglePressed()) {
    if (!dreieckDavorGedrueckt) {
      if (currentMillis - letzterDreieckKlickMillis < 350) {
        if (!akkuKritischLeer) {
          fernlichtAktiv = !fernlichtAktiv;
          if (fernlichtAktiv) {
            fahrlichtEnabled = true;
            letzterLichtModus = 2; 
          } else {
            letzterLichtModus = 1; 
          }
          Serial.print("Fernlicht Status: "); Serial.println(fernlichtAktiv);
        } else {
          Serial.println("ECO-MODUS AKTIV: Fernlicht blockiert!");
          tone(BUZZER_PIN, 150, 80); 
        }
        einfacherKlickAusstehend = false; 
        letzterDreieckKlickMillis = 0;
      } else {
        letzterDreieckKlickMillis = currentMillis;
        dreieckKlickTimer = currentMillis;
        einfacherKlickAusstehend = true;
      }
      dreieckDavorGedrueckt = true;
    }
  } else {
    dreieckDavorGedrueckt = false;
  }

  if (einfacherKlickAusstehend && (currentMillis - dreieckKlickTimer >= 350)) {
    if (fahrlichtEnabled || fernlichtAktiv) {
      fahrlichtEnabled = false;
      fernlichtAktiv = false;
      Serial.println("Licht komplett AUS (Modus im Speicher gesichert)");
    } else {
      if (letzterLichtModus == 2 && !akkuKritischLeer) {
        fahrlichtEnabled = true;
        fernlichtAktiv = true; 
        Serial.println("Licht AN -> Memory: Fernlicht reaktiviert!");
      } else {
        fahrlichtEnabled = true;
        fernlichtAktiv = false; 
        Serial.println("Licht AN -> Memory: Standlicht reaktiviert!");
      }
    }
    einfacherKlickAusstehend = false;
  }

  if (GamePad.isCirclePressed()) {
    if (!kreisDavorGedrueckt) {
      if (currentMillis - letzterKreisKlickMillis < 350) {
        warnblinkerAktiv = !warnblinkerAktiv; 
        letzterKreisKlickMillis = 0;
        Serial.print("Warnblinker Status: "); Serial.println(warnblinkerAktiv);
      } else {
        letzterKreisKlickMillis = currentMillis;
      }
      kreisDavorGedrueckt = true;
    }
  } else {
    kreisDavorGedrueckt = false;
  }

  if (GamePad.isCrossPressed()) {
    if (currentEffect == 0) {
      setStaticColor(ring.Color(0, 255, 0));
      setFrontColor(fernlichtAktiv ? FERNLICHT_WEISS : (fahrlichtEnabled ? GELB_STROM_SPAR : LICHT_AUS)); 
      if (!hupeAktiv) { tone(BUZZER_PIN, 300); hupeAktiv = true; }
    }
    else if (currentEffect == 1) {
      spielePolizeiSirene(); 
      updatePolizeiLicht(currentMillis); 
      setFrontColor((currentMillis % 200 < 100) ? (fernlichtAktiv ? FERNLICHT_WEISS : GELB_STROM_SPAR) : LICHT_AUS); 
    } 
    else if (currentEffect == 2) {
      spieleFluchtSound(currentMillis); 
      setFrontColor(fernlichtAktiv ? FERNLICHT_WEISS : GELB_STROM_SPAR); 
    } 
    else if (currentEffect == 3) {
      updateTeamBlau(currentMillis); 
      setFrontColor(fernlichtAktiv ? FERNLICHT_WEISS : (fahrlichtEnabled ? GELB_STROM_SPAR : LICHT_AUS)); 
      if (!hupeAktiv) { tone(BUZZER_PIN, 600); hupeAktiv = true; }
    }
    else if (currentEffect == 4) {
      updateTeamGruen(currentMillis); 
      setFrontColor(fernlichtAktiv ? FERNLICHT_WEISS : (fahrlichtEnabled ? GELB_STROM_SPAR : LICHT_AUS)); 
      if (!hupeAktiv) { tone(BUZZER_PIN, 1200); hupeAktiv = true; }
    }
  } else {
    if (!GamePad.isCrossPressed()) {
      if (!akkuKritischLeer || GamePad.isTrianglePressed() || GamePad.isCirclePressed() || GamePad.isSquarePressed()) {
         noTone(BUZZER_PIN);
      }
      resetFluchtSound();
      hupeAktiv = false;
    }
  }

  if (warnblinkerAktiv) {
    warnblinken(blinkerStatus);
  } 
  else if (GamePad.isSquarePressed()) {
    blinkenLinks(blinkerStatus);
    if (currentEffect == 0) setStaticColor(ring.Color(0, 0, 255)); // FIX: Sauber gelöst!
  } 
  else if (GamePad.isCirclePressed()) {
    blinkenRechts(blinkerStatus);
    if (currentEffect == 0) setStaticColor(ring.Color(255, 0, 0));
  }
}

#endif
