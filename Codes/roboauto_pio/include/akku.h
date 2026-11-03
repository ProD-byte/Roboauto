#pragma once
#include <Arduino.h>

// Zwei Umsetzungen mit denselben Funktionen:
//   akku.cpp           Super Mini: Spannungsteiler am ADC
//   akku_max17048.cpp  XIAO Sense: Akkuwaechter MAX17048 am I2C-Bus

// Akkumessung vorbereiten (einmal in setup aufrufen)
void akkuStarten();

// In jedem loop-Durchlauf aufrufen (misst regelmaessig und glaettet)
void akkuUpdate();

// Messwert: Super Mini = geglaetteter ADC-Rohwert (0..4095), XIAO = Akkuspannung in Millivolt
int akkuWert();

// true = Akku fast leer -> Eco-Modus (Fernlicht gesperrt)
bool akkuEco();

// Fuellstand 0..100 %
int akkuProzent();

// true = offenbar haengt nur USB/Netzteil dran bzw. der Akku wird geladen
bool akkuUsb();

// Akku-Verlauf als JSON fuer die Webseite: {"intervall":10,"werte":[..%..]} (255 = USB)
String akkuVerlaufJson();
