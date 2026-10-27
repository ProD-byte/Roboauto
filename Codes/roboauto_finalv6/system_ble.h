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
extern bool startButtonPressed;
extern bool selectButtonPressed;
extern bool hupeAktiv;

// Variablen für die Doppelklick-Erkennung
static bool kreisDavorGedrueckt = false;
static unsigned long letzterKreisKlickMillis = 0;
static bool warnblinkerAktiv = false;      // wird auch im Hauptloop abgefragt

static bool dreieckDavorGedrueckt = false;
static unsigned long letzterDreieckKlickMillis = 0;
static unsigned long dreieckKlickTimer = 0;
static bool einfacherKlickAusstehend = false;

// V6: Merkt sich, ob X (Kreuz) in der letzten Runde gedrückt war.
// Der Buzzer wird nur noch beim LOSLASSEN von X gestoppt - nicht mehr in jeder Runde.
// Dadurch hört die Hupe auch im Eco-Modus auf und kurze Warn-Piepser werden nicht abgewürgt.
static bool kreuzWarGedrueckt = false;

inline void checkBluetoothVerbindung() {
  BLEServer* server = BLEDevice::getServer();
  int connectedDevices = server ? server->getConnectedCount() : 0;

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
      
      // V6: das sofortige noTone() hat den zweiten Piepton bisher abgeschnitten
      tone(BUZZER_PIN, 1000, 100); delay(120);
      tone(BUZZER_PIN, 1500, 150);
      
      triggerWelcome = true;
      welcomeStartMillis = millis();

      // Das Gyroskop exakt beim Koppeln kalibrieren
      kalibriereGyroskop(); 
      welcomeStartMillis = millis();   // V6: Welcome-Effekt startet erst NACH der Kalibrierung
    }
  } else {
    if (isConnected) {
      isConnected = false;
      stoppeMotoren();                 // V6: Sicherheit - Motoren sofort aus, nicht erst nach den Tönen
      clearRing();
      currentLed = 0;
      setFrontColor(LICHT_AUS);
      setRearColor(LICHT_AUS);
      warnblinkerAktiv = false;
      fernlichtAktiv = false;
      triggerWelcome = false;
      hupeAktiv = false;
      kreuzWarGedrueckt = false;
      Serial.println("Verbindung getrennt!");
      
      tone(BUZZER_PIN, 600, 200); delay(220);
      tone(BUZZER_PIN, 400, 300); delay(300);
      noTone(BUZZER_PIN); 
      
      resetFluchtSound();
      // V6: Statt Dabble.begin() erneut aufzurufen (BLE-Stack doppelt initialisiert),
      // wird nur die Sichtbarkeit (Advertising) neu gestartet.
      BLEDevice::startAdvertising();
    }
  }
}

inline void checkSystemTasten() {
  if (!isConnected) return;
  unsigned long currentMillis = millis();

  startButtonPressed = GamePad.isStartPressed();

  // SELECT: nächster Licht-Modus
  if (GamePad.isSelectPressed()) {
    if (!selectButtonPressed) {
      currentEffect = (currentEffect + 1) % TOTAL_EFFECTS; 
      selectButtonPressed = true;
      clearRing(); 
      if (hupeAktiv) { noTone(BUZZER_PIN); hupeAktiv = false; }  // V6: neuer Modus = neuer Hupenton
    }
  } else { selectButtonPressed = false; }

  // DREIECK: Einfachklick = Licht an/aus, Doppelklick = Fernlicht
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

  // KREIS: Doppelklick = Warnblinker an/aus
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

  // KREUZ (X): Hupe / Sirene / Nitro je nach Modus
  if (GamePad.isCrossPressed()) {
    kreuzWarGedrueckt = true;
    uint32_t frontFarbe = getAktuelleHintergrundLichtFarbe();

    if (currentEffect == 0) {
      setStaticColor(ring.Color(0, 255, 0));
      setFrontColor(frontFarbe); 
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
      setFrontColor(frontFarbe); 
      if (!hupeAktiv) { tone(BUZZER_PIN, 600); hupeAktiv = true; }
    }
    else if (currentEffect == 4) {
      updateTeamGruen(currentMillis); 
      setFrontColor(frontFarbe); 
      if (!hupeAktiv) { tone(BUZZER_PIN, 1200); hupeAktiv = true; }
    }
  } else {
    if (kreuzWarGedrueckt) {          // V6: nur beim Loslassen stoppen
      noTone(BUZZER_PIN);
      kreuzWarGedrueckt = false;
      hupeAktiv = false;
    }
    resetFluchtSound();
  }

  // Blinker (Warnblinker hat Vorrang)
  if (warnblinkerAktiv) {
    warnblinken(blinkerStatus);
  } 
  else if (GamePad.isSquarePressed()) {
    blinkenLinks(blinkerStatus);
    if (currentEffect == 0) setStaticColor(ring.Color(0, 0, 255));
  } 
  else if (GamePad.isCirclePressed()) {
    blinkenRechts(blinkerStatus);
    if (currentEffect == 0) setStaticColor(ring.Color(255, 0, 0));
  }
}

#endif
