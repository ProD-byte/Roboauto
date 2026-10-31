#pragma once
#include <esp_err.h>
// Nur vorwaerts deklariert: esp_http_server.h vertraegt sich nicht mit WebServer.h (HTTP_ANY doppelt)
typedef struct httpd_req httpd_req_t;

// Medien-Server auf Port MEDIEN_PORT starten (erst, wenn das WLAN laeuft).
// Live-Video, Live-Ton und Downloads dauern lange - jede solche Anfrage bekommt
// ihre eigene kleine Aufgabe, damit sich die Browser nicht gegenseitig blockieren.
void medienServerStarten();

// Die einzelnen Seiten (stehen in kamera.cpp, mikro.cpp, karte.cpp)
esp_err_t kameraStreamSenden(httpd_req_t* req);
esp_err_t kameraBildSenden(httpd_req_t* req);
esp_err_t mikroTonSenden(httpd_req_t* req);
esp_err_t karteListeSenden(httpd_req_t* req);
esp_err_t karteDateiSenden(httpd_req_t* req);
