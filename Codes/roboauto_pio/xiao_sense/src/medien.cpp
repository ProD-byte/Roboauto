#include <Arduino.h>
#include <esp_http_server.h>
#include "config.h"
#include "medien.h"

static httpd_handle_t server = nullptr;

// Eine lange Anfrage in eine eigene Aufgabe auslagern ("async handler" des ESP-Webservers)
struct Auftrag { httpd_req_t* req; esp_err_t (*seite)(httpd_req_t*); };

static void auftragAufgabe(void* p) {
  Auftrag* a = (Auftrag*)p;
  a->seite(a->req);
  httpd_req_async_handler_complete(a->req);
  delete a;
  vTaskDelete(nullptr);
}

static esp_err_t auslagern(httpd_req_t* req) {
  httpd_req_t* kopie = nullptr;
  if (httpd_req_async_handler_begin(req, &kopie) != ESP_OK) return ESP_FAIL;
  Auftrag* a = new Auftrag{ kopie, (esp_err_t (*)(httpd_req_t*))req->user_ctx };
  if (xTaskCreatePinnedToCore(auftragAufgabe, "medien", 8192, a, 2, nullptr, 0) != pdPASS) {
    delete a;
    httpd_req_async_handler_complete(kopie);
    return ESP_FAIL;
  }
  return ESP_OK;
}

static void anmelden(const char* uri, esp_err_t (*seite)(httpd_req_t*), bool lang) {
  httpd_uri_t u = {};
  u.uri = uri;
  u.method = HTTP_GET;
  u.handler = lang ? auslagern : seite;
  u.user_ctx = (void*)seite;
  httpd_register_uri_handler(server, &u);
}

void medienServerStarten() {
  if (server) return;
  httpd_config_t c = HTTPD_DEFAULT_CONFIG();
  c.server_port = MEDIEN_PORT;
  c.ctrl_port   = 32768 + MEDIEN_PORT;       // darf nicht mit anderen Servern kollidieren
  c.max_open_sockets = 5;
  c.lru_purge_enable = true;                 // haengende alte Verbindungen rauswerfen
  c.core_id = 0;                             // loop() laeuft auf Kern 1 -> Medien auf Kern 0
  if (httpd_start(&server, &c) != ESP_OK) { Serial.println(">> FEHLER: Medien-Server startet nicht"); return; }
  anmelden("/stream",  kameraStreamSenden, true);
  anmelden("/bild",    kameraBildSenden,   false);
  anmelden("/ton",     mikroTonSenden,     true);
  anmelden("/dateien", karteListeSenden,   false);
  anmelden("/datei",   karteDateiSenden,   true);
  Serial.printf(">> Medien: Video http://%s.local:%d/stream  Ton :%d/ton  SD :%d/dateien\n",
                WEB_NAME, MEDIEN_PORT, MEDIEN_PORT, MEDIEN_PORT);
}
