#pragma once
#include <Arduino.h>

// Mikrofon (PDM) starten - liest ab dann in einer eigenen Aufgabe mit
bool mikroStarten();

// Live-Ton (/ton, WAV-Stream fuer den Browser) liefert der Medien-Server, siehe medien.h

bool mikroDa();
int  mikroPegel();        // aktuelle Lautstaerke 0..100 (fuer Anzeige und Lern-Daten)
int  mikroZuhoerer();     // wie viele Browser gerade zuhoeren

// Liefert einmalig true, wenn zweimal kurz hintereinander geklatscht wurde
bool mikroDoppelKlatsch();
