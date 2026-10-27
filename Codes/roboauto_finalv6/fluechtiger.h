#ifndef FLUECHTIGER_H
#define FLUECHTIGER_H

#include <Arduino.h>
#include "globals.h"

// Variablen für den Flucht-Modus
static int escapeStep = 0;
static int motorFrequenz = 300; 

// Modernes TANK-SYSTEM für den Nitro-Boost (in Millisekunden)
const long TANK_MAX = 3000;              // Maximal 3 Sekunden Boost-Energie im Tank
static long boostTank = TANK_MAX;        // Start mit vollem Tank
static unsigned long letzteBerechnung = 0; // Merkt sich die Zeit für den Verbrauch/Ladevorgang

static bool istImBoost = false;
static bool boostGesperrt = false; 
static unsigned long sperrStartMillis = 0;
const unsigned long COOLDOWN_DAUER = 6000;   // 6 Sekunden Zwangspause nach kompletter Überhitzung

// Licht-Animation im Stillstand (Standard, wenn keine Taste gedrückt ist - Weiß -> Pink/Violett)
inline void updateFluchtLicht(unsigned long currentMillis) {
  static unsigned long letzterSchritt = 0;   // V6: eigener Takt statt globalem previousMillis
  if (currentMillis - letzterSchritt >= 150) { 
    letzterSchritt = currentMillis;
    clearRingNoShow();
    escapeStep = (escapeStep + 1) % 4; 
    
    if (escapeStep == 0) {
      ring.setPixelColor(0, ring.Color(255, 255, 255)); 
    } else if (escapeStep == 1) {
      ring.setPixelColor(1, ring.Color(255, 120, 255)); 
      ring.setPixelColor(7, ring.Color(255, 120, 255)); 
    } else if (escapeStep == 2) {
      ring.setPixelColor(2, ring.Color(255, 40, 220)); 
      ring.setPixelColor(6, ring.Color(255, 40, 220));   
    } else if (escapeStep == 3) {
      ring.setPixelColor(3, ring.Color(255, 0, 180)); 
      ring.setPixelColor(4, ring.Color(255, 0, 180)); 
      ring.setPixelColor(5, ring.Color(255, 0, 180)); 
    }
    ring.show();
  }
}

// Komplexer Sound- & Licht-Effekt für den Nitro-Boost (Jetzt mit echtem Tank-Verbrauch!)
inline void spieleFluchtSound(unsigned long currentMillis) {
  // Zeit-Differenz seit dem letzten Durchlauf berechnen (wichtig für flüssigen Verbrauch)
  if (letzteBerechnung == 0) letzteBerechnung = currentMillis;
  long zeitVergangen = currentMillis - letzteBerechnung;
  letzteBerechnung = currentMillis;

  // 1. ZUSTAND: Motor ist komplett überhitzt und in der 6-Sekunden-Zwangspause
  if (boostGesperrt) {
    unsigned long vergangeneKuehlZeit = currentMillis - sperrStartMillis;
    if (vergangeneKuehlZeit >= COOLDOWN_DAUER) {
      boostGesperrt = false; 
      boostTank = TANK_MAX; // Tank nach Zwangspause wieder voll geladen
    } else {
      // Überhitzungs-Sound und LED-Status-Anzeige (Ladebalken)
      if (currentMillis % 100 < 50) tone(BUZZER_PIN, 250);
      else                          noTone(BUZZER_PIN);
      
      if (currentMillis % 200 < 100) setRearColor(ring.Color(255, 0, 0));
      else                           setRearColor(ring.Color(0, 0, 0));
      
      // HIER REAGIERT DER RING: Zeigt den roten Ladebalken NUR an, solange X (Kreuz) gedrückt gehalten wird!
      clearRingNoShow();
      int leuchtendeLeds = map(vergangeneKuehlZeit, 0, COOLDOWN_DAUER, 0, NUM_LEDS);
      for (int i = 0; i < leuchtendeLeds; i++) ring.setPixelColor(i, ring.Color(100, 0, 0)); 
      ring.show();
      return;
    }
  }

  // 2. ZUSTAND: Boost feuert (Solange noch Energie im Tank ist)
  if (boostTank > 0 && !boostGesperrt) {
    istImBoost = true;
    boostTank -= zeitVergangen; // Tank leert sich echtheitsgetreu um die vergangen Millisekunden
    if (boostTank < 0) boostTank = 0;

    motorFrequenz += 25; 
    if (motorFrequenz > 2000) motorFrequenz = 1800; 
    tone(BUZZER_PIN, motorFrequenz);
    delay(2); 
    
    // Rücklichter blitzen wie Nitro-Geschosse
    if (currentMillis % 60 < 30) {
      for (int i = 0; i < NUM_REAR_LEDS; i++) rearLights.setPixelColor(i, ring.Color(150, 220, 255));
    } else {
      for (int i = 0; i < NUM_REAR_LEDS; i++) rearLights.setPixelColor(i, ring.Color(255, 40, 0));
    }
    rearLights.show();
    
    // LED-Ring brennt!
    for (int i = 0; i < NUM_LEDS; i++) {
      ring.setPixelColor(i, ring.Color(random(180, 255), random(20, 90), 0));
    }
    ring.show();
  } 
  // 3. ZUSTAND: Tank ist während des Drückens leer geworden -> BOOM, Überhitzung!
  else {
    istImBoost = false;
    boostGesperrt = true;
    sperrStartMillis = currentMillis;
  }
}

// Wird automatisch vom Hauptsketch aufgerufen, wenn X (Kreuz) LOSGELASSEN wird
inline void resetFluchtSound() {
  istImBoost = false;
  unsigned long currentMillis = millis();

  // Zeit-Differenz berechnen, um den Tank auch im Hintergrund aufzuladen
  if (letzteBerechnung == 0) letzteBerechnung = currentMillis;
  long zeitVergangen = currentMillis - letzteBerechnung;
  letzteBerechnung = currentMillis;

  if (!boostGesperrt) {
    motorFrequenz = 300; // Sound fällt auf Standgas zurück

    // Tank lädt sich beim Loslassen lautlos im Hintergrund wieder auf (Faktor 0.5)
    boostTank += (zeitVergangen * 0.5); 
    if (boostTank > TANK_MAX) boostTank = TANK_MAX;
    
    // WICHTIGER COMPLIANCE-FIX: Keine LEDs mehr überschreiben! 
    // Der Ring bleibt frei für die originale Flucht-Animation aus dem Hauptsketch.
  }
}

#endif
