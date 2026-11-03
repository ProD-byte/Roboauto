#include <Arduino.h>
#include <freertos/idf_additions.h>
#include <ESP_I2S.h>
#include <esp_http_server.h>
#include <freertos/stream_buffer.h>
#include <atomic>
#include "config.h"
#include "mikro.h"
#include "medien.h"

// ============================================================
//  PDM-Mikrofon des XIAO ESP32S3 Sense.
//  Eine eigene Aufgabe liest staendig 32 ms Ton, misst die Lautstaerke,
//  erkennt Klatscher und gibt den Ton an den Medien-Server (Port 81, /ton) weiter.
// ============================================================

static I2SClass i2s;
static bool da = false;
static volatile int pegel = 0;
static volatile bool doppelKlatsch = false;
static std::atomic<int> zuhoerer{0};
static StreamBufferHandle_t puffer = nullptr;   // Ton fuer den Live-Stream
static StreamBufferHandle_t aufnahmePuffer = nullptr;   // Ton fuer die Video-Aufnahme (im PSRAM)
static volatile bool aufnahmeAn = false;

constexpr int BLOCK = 512;                      // 512 Werte = 32 ms bei 16 kHz

static void mikroAufgabe(void*) {
  static int16_t ton[BLOCK];
  bool laut = false;
  unsigned long leiseSeit = 0, ersterKlatsch = 0;
  int32_t mitte = 0;                            // Gleichanteil (das Mikro liefert einen kleinen Versatz)
  for (;;) {
    size_t n = i2s.readBytes((char*)ton, sizeof(ton)) / sizeof(int16_t);
    if (n == 0) { delay(5); continue; }

    // Lautstaerke: Effektivwert -> grob logarithmisch auf 0..100
    int64_t summe = 0, summeQ = 0;
    for (size_t i = 0; i < n; i++) summe += ton[i];
    mitte += ((int32_t)(summe / (int64_t)n) - mitte) / 8;
    for (size_t i = 0; i < n; i++) {
      int32_t v = (ton[i] - mitte) * MIKRO_VERSTAERKUNG;
      v = constrain(v, -32768, 32767);
      ton[i] = v;
      summeQ += (int64_t)v * v;
    }
    float rms = sqrtf((float)summeQ / n);
    int p = rms < 1 ? 0 : constrain((int)(20.0f * log10f(rms) * 1.1f - 10), 0, 100);
    pegel = p;

    // Klatschen: ploetzlich laut, dann wieder leise. Zwei davon kurz hintereinander = Doppelklatsch
    unsigned long jetzt = millis();
    if (p >= KLATSCH_SCHWELLE && !laut && jetzt - leiseSeit > KLATSCH_PAUSE_MS) {
      laut = true;
      if (ersterKlatsch && jetzt - ersterKlatsch < KLATSCH_FENSTER_MS) { doppelKlatsch = true; ersterKlatsch = 0; }
      else ersterKlatsch = jetzt;
    } else if (p < KLATSCH_SCHWELLE - 15 && laut) {
      laut = false;
      leiseSeit = jetzt;
    }

    // Hoert jemand zu? Dann den Ton weiterreichen (ist der Puffer voll, fallen alte Daten weg)
    if (zuhoerer > 0) xStreamBufferSend(puffer, ton, n * sizeof(int16_t), 0);
    if (aufnahmeAn) xStreamBufferSend(aufnahmePuffer, ton, n * sizeof(int16_t), 0);
  }
}

bool mikroStarten() {
  i2s.setPinsPdmRx(MIKRO_TAKT_PIN, MIKRO_DATEN_PIN);
  if (!i2s.begin(I2S_MODE_PDM_RX, MIKRO_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO)) {
    Serial.println(">> FEHLER: Mikrofon startet nicht");
    return false;
  }
  // 2 Sekunden Vorrat (im PSRAM): so uebersteht der Live-Ton auch kurze Funkstoerungen ohne Luecke
  puffer = xStreamBufferCreateWithCaps(MIKRO_RATE * 2 * 2, 1024, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  // Stapelspeicher im PSRAM - der interne Arbeitsspeicher wird fuer WLAN, Bluetooth und Kamera gebraucht
  xTaskCreatePinnedToCoreWithCaps(mikroAufgabe, "mikro", 4096, nullptr, 3, nullptr, 0, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  da = true;
  Serial.println(">> Mikrofon bereit");
  return true;
}

bool mikroDa()       { return da; }
int  mikroPegel()    { return pegel; }
int  mikroZuhoerer() { return zuhoerer; }
// Video-Aufnahme: Ton sammeln (1 Sekunde Puffer im PSRAM - der interne Speicher ist knapp)
bool mikroAufnahme(bool an) {
  if (an && !da) return false;
  if (an && !aufnahmePuffer) aufnahmePuffer = xStreamBufferCreateWithCaps(MIKRO_RATE * 2, 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!aufnahmePuffer) return false;
  if (an) xStreamBufferReset(aufnahmePuffer);
  aufnahmeAn = an;
  return true;
}
size_t mikroAufnahmeLesen(void* ziel, size_t max) {
  return aufnahmePuffer ? xStreamBufferReceive(aufnahmePuffer, ziel, max & ~1u, 0) : 0;
}

bool mikroDoppelKlatsch() { bool k = doppelKlatsch; doppelKlatsch = false; return k; }

// ---------- Live-Ton: WAV-Datei ohne Ende (der Browser spielt sie einfach ab) ----------
// G.711 mu-law (wie beim Telefon): 16 Bit -> 8 Bit, halbe Datenmenge, kaum hoerbarer Unterschied
static uint8_t ulaw(int16_t wert) {
  int s = wert, vorzeichen = 0;
  if (s < 0) { s = -s; vorzeichen = 0x80; }
  if (s > 32635) s = 32635;
  s += 0x84;
  int stufe = 7;
  for (int maske = 0x4000; (s & maske) == 0 && stufe > 0; maske >>= 1) stufe--;
  return ~(vorzeichen | (stufe << 4) | ((s >> (stufe + 3)) & 0x0F));
}

static void wavKopf(uint8_t* k, bool mulaw) {
  const uint32_t rate = MIKRO_RATE, bytesProSek = MIKRO_RATE * (mulaw ? 1 : 2), riesig = 0x7FFFFFFF;
  memcpy(k, "RIFF", 4);       memcpy(k + 4, &riesig, 4);  memcpy(k + 8, "WAVEfmt ", 8);
  uint32_t fmtLaenge = 16;    memcpy(k + 16, &fmtLaenge, 4);
  uint16_t pcm = mulaw ? 7 : 1, kanaele = 1, block = mulaw ? 1 : 2, bits = mulaw ? 8 : 16;   // 7 = mu-law
  memcpy(k + 20, &pcm, 2);    memcpy(k + 22, &kanaele, 2);
  memcpy(k + 24, &rate, 4);   memcpy(k + 28, &bytesProSek, 4);
  memcpy(k + 32, &block, 2);  memcpy(k + 34, &bits, 2);
  memcpy(k + 36, "data", 4);  memcpy(k + 40, &riesig, 4);
}

esp_err_t mikroTonSenden(httpd_req_t* req) {
  if (!da) { httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "kein Mikrofon"); return ESP_FAIL; }
  // Der Tonpuffer hat nur einen Ausgang -> immer nur ein Zuhoerer gleichzeitig
  if (zuhoerer > 0) { httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "es hoert schon jemand zu"); return ESP_FAIL; }
  // ?fmt=ulaw: halbe Datenmenge (die Webseite nimmt das - das 2,4-GHz-WLAN ist oft knapp)
  char anfrage[64], fmt[8] = "";
  if (httpd_req_get_url_query_str(req, anfrage, sizeof(anfrage)) == ESP_OK) httpd_query_key_value(anfrage, "fmt", fmt, sizeof(fmt));
  const bool mulaw = !strcmp(fmt, "ulaw");
  httpd_resp_set_type(req, "audio/wav");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  uint8_t kopf[44];
  wavKopf(kopf, mulaw);
  esp_err_t ok = httpd_resp_send_chunk(req, (const char*)kopf, sizeof(kopf));
  xStreamBufferReset(puffer);
  zuhoerer++;
  alignas(4) uint8_t daten[2048];   // ausgerichtet: wird auch als 16-Bit-Werte gelesen
  while (ok == ESP_OK) {
    size_t n = xStreamBufferReceive(puffer, daten, sizeof(daten), pdMS_TO_TICKS(500));
    if (n > 1 && mulaw) {
      const int16_t* w = (const int16_t*)daten;
      for (size_t i = 0; i < n / 2; i++) daten[i] = ulaw(w[i]);   // vorne ueberschreiben geht: Ziel <= Quelle
      ok = httpd_resp_send_chunk(req, (const char*)daten, n / 2);
    } else if (n > 0) ok = httpd_resp_send_chunk(req, (const char*)daten, n & ~1u);
  }
  zuhoerer--;
  return ok;
}
