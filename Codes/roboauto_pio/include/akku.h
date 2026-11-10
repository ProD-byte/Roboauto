#pragma once
#include <Arduino.h>

// Umsetzung: akku_max17048.cpp - Akkuwaechter MAX17048/MAX17043 am I2C-Bus

// Akkumessung vorbereiten (einmal in setup aufrufen)
void akkuStarten();

// In jedem loop-Durchlauf aufrufen (misst regelmaessig und glaettet)
void akkuUpdate();

// Messwert: Akkuspannung in Millivolt
int akkuWert();

// true = Akku fast leer -> Eco-Modus (Fernlicht gesperrt)
bool akkuEco();

// Fuellstand 0..100 %
int akkuProzent();

// true = offenbar haengt nur USB/Netzteil dran bzw. der Akku wird geladen
bool akkuUsb();

// Akku-Verlauf als JSON fuer die Webseite: {"intervall":10,"werte":[..%..]} (255 = USB)
String akkuVerlaufJson();
