#include <Arduino.h>
#include <esp_http_server.h>
#include <atomic>
#include <freertos/idf_additions.h>
#include <esp_heap_caps.h>
#include <lwip/sockets.h>
#include "config.h"
#include "medien.h"

static httpd_handle_t server = nullptr;

// Lange Anfragen (Video, Ton, Downloads) laufen nicht im Webserver selbst, sondern bei einem von
// LANG_MAX festen "Arbeitern" ("async handler" des ESP-Webservers) - so blockieren sie sich nicht gegenseitig.
// Die Arbeiter werden EINMAL beim Start angelegt, mit ihrem Stapelspeicher im PSRAM: der interne
// Arbeitsspeicher ist mit WLAN, Bluetooth und Kamera fast voll (frueher: eine neue 8-KB-Aufgabe je
// Anfrage -> nach ein paar Video-Versuchen kein Speicher mehr, der Server legte stumm auf).
struct Auftrag { httpd_req_t* req; esp_err_t (*seite)(httpd_req_t*); };

// Die Roboauto-App zeigt die Oberflaeche von einer eigenen Adresse aus -> Antworten fuer alle freigeben (CORS)
// Ausserdem: kleine Pakete sofort senden (TCP_NODELAY). Sonst wartet jedes kleine Stueck (z.B. der Vorspann
// eines Datenblocks) auf die Bestaetigung des vorigen - mit verzoegerten Bestaetigungen bis zu 200 ms.
static void freigeben(httpd_req_t* req) {
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  int ein = 1, fd = httpd_req_to_sockfd(req);
  if (fd >= 0) setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &ein, sizeof(ein));
}

static esp_err_t kurz(httpd_req_t* req) {
  freigeben(req);
  return ((esp_err_t (*)(httpd_req_t*))req->user_ctx)(req);
}

// Jede lange Anfrage haelt eine der MEDIEN_VERBINDUNGEN fest -> hoechstens LANG_MAX gleichzeitig,
// darueber gibt es eine klare Antwort "503" (statt stumm aufzulegen).
constexpr int MEDIEN_VERBINDUNGEN = 6, LANG_MAX = 4;   // der ESP hat nur 16 Sockets fuer alles zusammen
static std::atomic<int> langLaufend{0};
static QueueHandle_t auftraege = nullptr;

static void arbeiter(void*) {
  Auftrag a;
  for (;;) {
    if (xQueueReceive(auftraege, &a, portMAX_DELAY) != pdTRUE) continue;
    freigeben(a.req);
    a.seite(a.req);
    httpd_req_async_handler_complete(a.req);
    langLaufend--;
  }
}

static esp_err_t besetzt(httpd_req_t* req, const char* grund) {
  Serial.printf(">> Medien: %s abgewiesen (%s, %d laufen, intern %u KB frei)\n",
                req->uri, grund, (int)langLaufend, (unsigned)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024));
  freigeben(req);
  httpd_resp_set_status(req, "503 Service Unavailable");
  httpd_resp_set_hdr(req, "Retry-After", "2");
  return httpd_resp_sendstr(req, grund);
}

static esp_err_t auslagern(httpd_req_t* req) {
  if (!auftraege || langLaufend >= LANG_MAX) return besetzt(req, "zu viele gleichzeitige Anfragen");
  httpd_req_t* kopie = nullptr;
  if (httpd_req_async_handler_begin(req, &kopie) != ESP_OK) return besetzt(req, "keine Verbindung frei");
  Auftrag a{ kopie, (esp_err_t (*)(httpd_req_t*))req->user_ctx };
  langLaufend++;
  if (xQueueSend(auftraege, &a, 0) != pdTRUE) {          // Platz fuer LANG_MAX - sollte nie voll sein
    langLaufend--;
    httpd_req_async_handler_complete(kopie);
    return ESP_FAIL;
  }
  return ESP_OK;
}

static void anmelden(const char* uri, esp_err_t (*seite)(httpd_req_t*), bool lang) {
  httpd_uri_t u = {};
  u.uri = uri;
  u.method = HTTP_GET;
  u.handler = lang ? auslagern : kurz;
  u.user_ctx = (void*)seite;
  httpd_register_uri_handler(server, &u);
}

void medienServerStarten() {
  if (server) return;
  httpd_config_t c = HTTPD_DEFAULT_CONFIG();
  c.server_port = MEDIEN_PORT;
  c.ctrl_port   = 32768 + MEDIEN_PORT;       // darf nicht mit anderen Servern kollidieren
  c.max_open_sockets = MEDIEN_VERBINDUNGEN;   // LANG_MAX lange + Platz fuer kurze (Einzelbild, Liste)
  c.lru_purge_enable = true;                 // haengende alte Verbindungen rauswerfen
  c.core_id = 0;                             // loop() laeuft auf Kern 1 -> Medien auf Kern 0
  if (httpd_start(&server, &c) != ESP_OK) { Serial.println(">> FEHLER: Medien-Server startet nicht"); return; }
  auftraege = xQueueCreate(LANG_MAX, sizeof(Auftrag));
  for (int i = 0; i < LANG_MAX; i++)
    if (xTaskCreatePinnedToCoreWithCaps(arbeiter, "medien", 8192, nullptr, 2, nullptr, 0, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) != pdPASS)
      Serial.println(">> FEHLER: Medien-Arbeiter startet nicht");
  anmelden("/stream",  kameraStreamSenden, true);
  anmelden("/bild",    kameraBildSenden,   false);
  anmelden("/ton",     mikroTonSenden,     true);
  anmelden("/dateien", karteListeSenden,   false);
  anmelden("/datei",   karteDateiSenden,   true);
  Serial.printf(">> Medien: Video http://%s.local:%d/stream  Ton :%d/ton  SD :%d/dateien\n",
                WEB_NAME, MEDIEN_PORT, MEDIEN_PORT, MEDIEN_PORT);
}
