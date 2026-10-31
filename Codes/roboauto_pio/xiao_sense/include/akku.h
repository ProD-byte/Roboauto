#pragma once
#include <Arduino.h>

// Akkuwaechter MAX17048 vorbereiten (einmal in setup aufrufen)
void akkuStarten();

// In jedem loop-Durchlauf aufrufen (fragt alle 500 ms den Akkuwaechter)
void akkuUpdate();

// Akkuspannung in Millivolt
int akkuWert();

// true = Akku fast leer -> Eco-Modus (Fernlicht gesperrt)
bool akkuEco();

// Ladestand 0..100 % (rechnet der MAX17048 selbst aus)
int akkuProzent();

// true = Akku wird geladen (USB steckt) oder kein Akkuwaechter gefunden
bool akkuUsb();

// true = MAX17048 antwortet am I2C-Bus
bool akkuGefunden();

// Akku-Verlauf als JSON fuer die Webseite: {"intervall":10,"werte":[..%..]} (255 = USB)
String akkuVerlaufJson();
