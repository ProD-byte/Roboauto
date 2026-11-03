#pragma once
#include <Arduino.h>

// Video-Aufnahme auf die SD-Karte: /videos/2026-11-03/video_15-43-10.avi
// AVI mit MJPEG-Bildern (so schnell wie die Kamera, hoechstens ca. 20/s) und - wenn gewuenscht -
// Ton vom Mikrofon (16 kHz, 16 Bit). Laeuft VLC, Windows, die meisten Handys.

void videoStarten();                  // einmal in setup (nach karteStarten)

bool  videoAufnahmeStarten(bool mitTon);   // false = geht nicht (keine Karte/Kamera, laeuft schon)
void  videoAufnahmeStoppen();
bool  videoLaeuft();
bool  videoMitTon();
float videoSekunden();                // Laenge der laufenden Aufnahme
int   videoZaehler();                 // zaehlt jede fertige Aufnahme hoch (Webseite laedt dann die Liste neu)
