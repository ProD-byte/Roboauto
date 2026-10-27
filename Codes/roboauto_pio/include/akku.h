#pragma once
#include <Arduino.h>

// Akkumessung vorbereiten (einmal in setup aufrufen)
void akkuStarten();

// In jedem loop-Durchlauf aufrufen (misst alle 20 ms und glaettet)
void akkuUpdate();

// Geglaetteter ADC-Rohwert (0..4095)
int akkuWert();

// true = Akku fast leer -> Eco-Modus (Fernlicht gesperrt)
bool akkuEco();

// Fuellstand 0..100 % (nach den V5-Grenzen AKKU_ANZEIGE_MIN/MAX)
int akkuProzent();

// true = Wert liegt so hoch, dass offenbar nur USB/Netzteil anliegt
bool akkuUsb();

// Akku-Verlauf als JSON fuer die Webseite: {"intervall":10,"werte":[..%..]} (255 = USB)
String akkuVerlaufJson();
