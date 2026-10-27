#ifndef SYSTEM_GYRO_H
#define SYSTEM_GYRO_H

#include <Arduino.h>
#include <Wire.h>
#include "globals.h"

#define MPU6050_ADDR   0x68 // Deine gefundene I2C-Adresse

// Variablen für die Abweichungen (Offsets)
static float gyroZOffset = 0.0;
static bool gyroBereit = false;

// Initialisiert das Gyroskop im Direkt-Register-Modus
inline void initGyroskop() {
  Wire.begin(GYRO_SDA_PIN, GYRO_SCL_PIN);
  
  // MPU6050 aufwecken (Register 0x6B auf 0 setzen)
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x6B); 
  Wire.write(0);    
  byte fehler = Wire.endTransmission();

  if (fehler != 0) {
    Serial.println("❌ FEHLER: MPU6050 im Direkt-Modus nicht gefunden!");
    gyroBereit = false;
    return;
  }
  
  // Gyroskop-Bereich einstellen auf +/- 250 Grad/Sekunde (Register 0x1B)
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x1B); 
  Wire.write(0x00); 
  Wire.endTransmission();
  
  Serial.println("🎉 MPU6050 erfolgreich im Direkt-Modus gestartet.");
  gyroBereit = true;
}

// Kalibrierung mit der coolen Ladebalken-Animation auf dem Ring
inline void kalibriereGyroskop() {
  if (!gyroBereit) return;
  
  Serial.println("Starte Gyro-Kalibrierung... BITTE NICHT BEWEGEN!");
  clearRing(); 

  float summeZ = 0;
  int messungen = 200; 

  for (int i = 0; i < messungen; i++) {
    // Direktes Auslesen des Z-Registers
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(0x47); 
    Wire.endTransmission(false);
    Wire.requestFrom(MPU6050_ADDR, 2);
    
    if (Wire.available() >= 2) {
      int16_t roherWertZ = (Wire.read() << 8) | Wire.read();
      summeZ += ((float)roherWertZ / 131.0);
    }
    delay(5); 
    
    // Animation: Alle 25 Messungen schaltet sich die nächste LED in Orange dazu
    int aktuelleLedAnzahl = (i + 1) / 25; 
    clearRingNoShow(); 
    for (int led = 0; led < aktuelleLedAnzahl; led++) {
      ring.setPixelColor(led, ring.Color(120, 40, 0)); 
    }
    ring.show(); 
  }

  // Durchschnitt berechnen = Fehlerwert im Stillstand
  gyroZOffset = summeZ / (float)messungen;
  Serial.print("Kalibrierung beendet. Gyro-Z-Offset: "); Serial.println(gyroZOffset);
  
  // Erfolgs-Anzeige: Ring wechselt kurz auf GRÜN
  for (int i = 0; i < NUM_LEDS; i++) {
    ring.setPixelColor(i, ring.Color(0, 255, 0)); 
  }
  ring.show();
  delay(400);
  clearRing(); 
}

// Hilfsfunktion: Liefert die echte, fehlerbereinigte Drehgeschwindigkeit des Autos (Grad/s)
inline float getEchteDrehungZ() {
  if (!gyroBereit) return 0.0;
  
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x47); 
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, 2);
  
  if (Wire.available() >= 2) {
    int16_t roherWertZ = (Wire.read() << 8) | Wire.read();
    return ((float)roherWertZ / 131.0) - gyroZOffset;
  }
  return 0.0;
}

#endif
