#pragma once
#include <Arduino.h>
#include <esp_camera.h>

// Kamera starten (einmal in setup, braucht PSRAM). false = keine Kamera gefunden
bool kameraStarten();

// Live-Video (/stream) und Einzelbild (/bild) liefert der Medien-Server, siehe medien.h

bool kameraDa();
int  kameraZuschauer();      // wie viele Browser gerade das Live-Video ansehen

// Aufloesung umstellen: 0 = 320x240, 1 = 640x480, 2 = 800x600, 3 = 1280x720 (HD)
void kameraAufloesung(int stufe);
int  kameraAufloesungStufe();

// JPEG-Qualitaet 4..40 (kleiner = schoener, aber groessere Bilder = weniger Bilder pro Sekunde)
void kameraQualitaet(int q);
int  kameraQualitaetWert();

// Autofokus (nur OV5640): -1 = keiner, 0 = stellt dauernd nach, 1 = einmal scharfgestellt und fest
int  kameraFokusArt();
void kameraFokus(bool fest);   // fest = jetzt scharfstellen und halten, sonst wieder dauernd automatisch
bool kameraScharf();           // meldet der Sensor "scharf"?

// Ein Bild holen (JPEG) - danach unbedingt kameraBildFrei() aufrufen!
camera_fb_t* kameraBild();
void kameraBildFrei(camera_fb_t* bild);
