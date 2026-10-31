#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <esp_http_server.h>
#include "config.h"
#include "karte.h"
#include "medien.h"

// ============================================================
//  microSD-Karte auf dem Sense-Board (SPI an D8/D9/D10, CS = GPIO21).
//  Hier landen Lern-Daten, Fotos und lange Fahrweg-Aufnahmen.
//  Das Dateisystem (FAT) ist von sich aus fuer mehrere Aufgaben gleichzeitig gesichert.
// ============================================================

static bool da = false;

bool karteStarten() {
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  da = SD.begin(SD_CS_PIN, SPI, 20000000);
  if (!da || SD.cardType() == CARD_NONE) {
    da = false;
    Serial.println(">> SD-Karte: keine gefunden (FAT32 formatiert?) - Lernen und Fotos aus");
    return false;
  }
  SD.mkdir("/lernen");
  SD.mkdir("/fotos");
  Serial.printf(">> SD-Karte: %u MB, davon %u MB belegt\n", (unsigned)karteGesamtMB(), (unsigned)karteBelegtMB());
  return true;
}

bool karteDa()             { return da; }
uint32_t karteGesamtMB()   { return da ? (uint32_t)(SD.totalBytes() / (1024 * 1024)) : 0; }
uint32_t karteBelegtMB()   { return da ? (uint32_t)(SD.usedBytes()  / (1024 * 1024)) : 0; }

// Hoechste vorhandene Nummer suchen (z.B. fahrt_0007) und eins dazu
int karteNaechsteNummer(const char* ordner, const char* name) {
  if (!da) return 1;
  int hoechste = 0;
  size_t laenge = strlen(name);
  File d = SD.open(ordner);
  if (!d) return 1;
  for (File f = d.openNextFile(); f; f = d.openNextFile()) {
    const char* n = f.name();
    if (strncmp(n, name, laenge) == 0 && n[laenge] == '_') hoechste = max(hoechste, atoi(n + laenge + 1));
    f.close();
  }
  d.close();
  return hoechste + 1;
}

static int zaehle(const char* ordner) {
  int n = 0;
  File d = SD.open(ordner);
  if (!d) return 0;
  for (File f = d.openNextFile(); f; f = d.openNextFile()) { n++; f.close(); }
  d.close();
  return n;
}

String karteJson() {
  if (!da) return "{\"da\":0}";
  return "{\"da\":1,\"gesamt\":" + String(karteGesamtMB()) + ",\"belegt\":" + String(karteBelegtMB()) +
         ",\"fahrten\":" + String(zaehle("/lernen")) + ",\"fotos\":" + String(zaehle("/fotos")) + "}";
}

// ---------- Seiten fuer den Medien-Server ----------

// "pfad" aus der Adresse holen (mit %xx-Entschluesselung), ".." ist verboten
static bool holePfad(httpd_req_t* req, char* pfad, size_t groesse, bool* loeschen = nullptr) {
  char anfrage[200], roh[160];
  if (httpd_req_get_url_query_str(req, anfrage, sizeof(anfrage)) != ESP_OK) { strcpy(pfad, "/"); }
  else {
    if (httpd_query_key_value(anfrage, "pfad", roh, sizeof(roh)) != ESP_OK) strcpy(roh, "/");
    size_t j = 0;
    for (size_t i = 0; roh[i] && j < groesse - 1; i++) {
      if (roh[i] == '%' && isxdigit(roh[i + 1]) && isxdigit(roh[i + 2])) {
        char hex[3] = { roh[i + 1], roh[i + 2], 0 };
        pfad[j++] = (char)strtol(hex, nullptr, 16);
        i += 2;
      } else pfad[j++] = roh[i] == '+' ? ' ' : roh[i];
    }
    pfad[j] = 0;
    if (loeschen) { char w[4]; *loeschen = httpd_query_key_value(anfrage, "weg", w, sizeof(w)) == ESP_OK; }
  }
  return pfad[0] == '/' && strstr(pfad, "..") == nullptr;
}

// /dateien?pfad=/lernen  ->  [{"n":"fahrt_0001","g":0,"o":1}, ...]   (o = Ordner)
esp_err_t karteListeSenden(httpd_req_t* req) {
  char pfad[160];
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  if (!da) { httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "keine SD-Karte"); return ESP_FAIL; }
  if (!holePfad(req, pfad, sizeof(pfad))) { httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "pfad?"); return ESP_FAIL; }
  File d = SD.open(pfad);
  if (!d || !d.isDirectory()) { httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "kein Ordner"); return ESP_FAIL; }
  httpd_resp_set_type(req, "application/json");
  httpd_resp_send_chunk(req, "[", 1);
  bool erster = true;
  char zeile[200];
  for (File f = d.openNextFile(); f; f = d.openNextFile()) {
    int n = snprintf(zeile, sizeof(zeile), "%s{\"n\":\"%s\",\"g\":%u,\"o\":%d}", erster ? "" : ",",
                     f.name(), (unsigned)f.size(), f.isDirectory() ? 1 : 0);
    httpd_resp_send_chunk(req, zeile, n);
    erster = false;
    f.close();
  }
  d.close();
  httpd_resp_send_chunk(req, "]", 1);
  return httpd_resp_send_chunk(req, nullptr, 0);
}

// Datei oder ganzen Ordner loeschen. Eine Fahrt hat tausende Bilder -> in Portionen zu 32
static bool loescheAlles(const char* pfad) {
  File f = SD.open(pfad);
  if (!f) return false;
  bool ordner = f.isDirectory();
  f.close();
  if (!ordner) return SD.remove(pfad);
  for (;;) {
    String kinder[32];
    int anzahl = 0;
    File d = SD.open(pfad);
    for (File k = d.openNextFile(); k && anzahl < 32; k = d.openNextFile()) kinder[anzahl++] = String(pfad) + "/" + k.name();
    d.close();
    if (anzahl == 0) break;
    int geloescht = 0;
    for (int i = 0; i < anzahl; i++) if (loescheAlles(kinder[i].c_str())) geloescht++;
    if (geloescht == 0) return false;      // nichts ging -> nicht endlos weiterprobieren
  }
  return SD.rmdir(pfad);
}

// /datei?pfad=/fotos/foto_0001.jpg  herunterladen,  &weg=1 = loeschen (auch ganze Ordner)
esp_err_t karteDateiSenden(httpd_req_t* req) {
  char pfad[160];
  bool loeschen = false;
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  if (!da) { httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "keine SD-Karte"); return ESP_FAIL; }
  if (!holePfad(req, pfad, sizeof(pfad), &loeschen) || strcmp(pfad, "/") == 0) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "pfad?");
    return ESP_FAIL;
  }
  if (loeschen) {
    bool ok = loescheAlles(pfad);
    Serial.printf(">> SD: %s %s\n", pfad, ok ? "geloescht" : "nicht geloescht");
    return httpd_resp_sendstr(req, ok ? "ok" : "fehler");
  }
  File f = SD.open(pfad);
  if (!f || f.isDirectory()) { httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "nicht gefunden"); return ESP_FAIL; }
  const char* endung = strrchr(pfad, '.');
  const char* typ = "application/octet-stream";
  if (endung && !strcasecmp(endung, ".jpg")) typ = "image/jpeg";
  if (endung && !strcasecmp(endung, ".csv")) typ = "text/csv";
  if (endung && !strcasecmp(endung, ".wav")) typ = "audio/wav";
  httpd_resp_set_type(req, typ);
  char name[200];
  snprintf(name, sizeof(name), "%s; filename=\"%s\"", strcmp(typ, "image/jpeg") ? "attachment" : "inline", strrchr(pfad, '/') + 1);
  httpd_resp_set_hdr(req, "Content-Disposition", name);
  uint8_t puffer[2048];
  esp_err_t ok = ESP_OK;
  size_t n;
  while (ok == ESP_OK && (n = f.read(puffer, sizeof(puffer))) > 0) ok = httpd_resp_send_chunk(req, (const char*)puffer, n);
  f.close();
  if (ok == ESP_OK) ok = httpd_resp_send_chunk(req, nullptr, 0);
  return ok;
}
