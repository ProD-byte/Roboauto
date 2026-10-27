#ifndef GLOBALS_H
#define GLOBALS_H

#include <Adafruit_NeoPixel.h>

// Pins für den ESP32-S3 Super Mini
#define LED_PIN          5   
#define BUZZER_PIN       6   
#define FRONT_LIGHT_PIN  7   
#define REAR_LIGHT_PIN   10  

#define GYRO_SDA_PIN     8   
#define GYRO_SCL_PIN     9   

// NEU KALIBRIERTER MOTOR-BLOCK: Pins 1 und 4 für den linken Motor!
#define MOTOR_L_V        1   // INT3 hängt jetzt an Pin 1 (Sicherer PWM-Pin)
#define MOTOR_L_R        4   // INT4 hängt jetzt an Pin 4 (Sicherer PWM-Pin)
#define MOTOR_R_V        13  // INT1 bleibt an Pin 13
#define MOTOR_R_R        2   // INT2 bleibt an Pin 2

#define BATTERIE_MESS_PIN  3   
#define ONBOARD_LED_PIN   48   

#define NUM_LEDS         8   
#define NUM_FRONT_LEDS   2   
#define NUM_REAR_LEDS    2   

// Scheinwerfer-Farbprofile
const uint32_t GELB_STROM_SPAR = 0x966400;   
const uint32_t FERNLICHT_WEISS = 0xFFFFFF;   

const uint32_t ROT_GEDIMMT     = 0x1E0000;   
const uint32_t ROT_HELL        = 0xFF0000;   
const uint32_t WEISS_RUECK     = 0xC8C8C8;   
const uint32_t LICHT_AUS       = 0x000000;   

// Externe Variablen-Deklarationen für den Compiler
extern Adafruit_NeoPixel ring;
extern Adafruit_NeoPixel frontLights;
extern Adafruit_NeoPixel rearLights;
extern unsigned long previousMillis;
extern int currentEffect;
extern const int TOTAL_EFFECTS;
extern long rainbowCycle;
extern int currentLed;

// Licht- und Systemzustände
extern bool fahrlichtEnabled;
extern bool fernlichtAktiv;
extern bool akkuKritischLeer;
extern bool blinkerStatus;
extern bool triggerWelcome;
extern unsigned long welcomeStartMillis;
extern int letzterLichtModus; 

// Die Hilfsfunktionen direkt hier als "inline"
inline void setStaticColor(uint32_t color) {
  for (int i = 0; i < NUM_LEDS; i++) ring.setPixelColor(i, color);
  ring.show();
}

inline void clearRing() {
  for (int i = 0; i < NUM_LEDS; i++) ring.setPixelColor(i, ring.Color(0, 0, 0));
  ring.show();
}

inline void clearRingNoShow() {
  for (int i = 0; i < NUM_LEDS; i++) ring.setPixelColor(i, ring.Color(0, 0, 0));
}

inline void setFrontColor(uint32_t color) {
  for (int i = 0; i < NUM_FRONT_LEDS; i++) frontLights.setPixelColor(i, color);
  frontLights.show();
}

inline void setRearColor(uint32_t color) {
  for (int i = 0; i < NUM_REAR_LEDS; i++) rearLights.setPixelColor(i, color);
  rearLights.show();
}

#endif
