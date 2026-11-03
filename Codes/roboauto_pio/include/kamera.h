#pragma once
#include <Arduino.h>
#include <esp_camera.h>

// Kamera starten (einmal in setup, braucht PSRAM). false = keine Kamera gefunden
bool kameraStarten();

// Live-Video (/stream) und Einzelbild (/bild) liefert der Medien-Server, siehe medien.h

bool kameraDa();
int  kameraZuschauer();      // wie viele Browser gerade das Live-Video ansehen

// Aufloesung umstellen: 0 = 320x240, 1 = 640x480, 2 = 800x600
void kameraAufloesung(int stufe);
int  kameraAufloesungStufe();

// Ein Bild holen (JPEG) - danach unbedingt kameraBildFrei() aufrufen!
camera_fb_t* kameraBild();
void kameraBildFrei(camera_fb_t* bild);
