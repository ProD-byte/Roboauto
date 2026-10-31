#include <Arduino.h>
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
  }
}

bool mikroStarten() {
  i2s.setPinsPdmRx(MIKRO_TAKT_PIN, MIKRO_DATEN_PIN);
  if (!i2s.begin(I2S_MODE_PDM_RX, MIKRO_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO)) {
    Serial.println(">> FEHLER: Mikrofon startet nicht");
    return false;
  }
  puffer = xStreamBufferCreate(MIKRO_RATE, 1024);   // MIKRO_RATE Bytes = eine halbe Sekunde Ton (16 Bit)
  xTaskCreatePinnedToCore(mikroAufgabe, "mikro", 4096, nullptr, 3, nullptr, 0);
  da = true;
  Serial.println(">> Mikrofon bereit");
  return true;
}

bool mikroDa()       { return da; }
int  mikroPegel()    { return pegel; }
int  mikroZuhoerer() { return zuhoerer; }
bool mikroDoppelKlatsch() { bool k = doppelKlatsch; doppelKlatsch = false; return k; }

// ---------- Live-Ton: WAV-Datei ohne Ende (der Browser spielt sie einfach ab) ----------
static void wavKopf(uint8_t* k) {
  const uint32_t rate = MIKRO_RATE, bytesProSek = MIKRO_RATE * 2, riesig = 0x7FFFFFFF;
  memcpy(k, "RIFF", 4);       memcpy(k + 4, &riesig, 4);  memcpy(k + 8, "WAVEfmt ", 8);
  uint32_t fmtLaenge = 16;    memcpy(k + 16, &fmtLaenge, 4);
  uint16_t pcm = 1, kanaele = 1, block = 2, bits = 16;
  memcpy(k + 20, &pcm, 2);    memcpy(k + 22, &kanaele, 2);
  memcpy(k + 24, &rate, 4);   memcpy(k + 28, &bytesProSek, 4);
  memcpy(k + 32, &block, 2);  memcpy(k + 34, &bits, 2);
  memcpy(k + 36, "data", 4);  memcpy(k + 40, &riesig, 4);
}

esp_err_t mikroTonSenden(httpd_req_t* req) {
  if (!da) { httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "kein Mikrofon"); return ESP_FAIL; }
  // Der Tonpuffer hat nur einen Ausgang -> immer nur ein Zuhoerer gleichzeitig
  if (zuhoerer > 0) { httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "es hoert schon jemand zu"); return ESP_FAIL; }
  httpd_resp_set_type(req, "audio/wav");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  uint8_t kopf[44];
  wavKopf(kopf);
  esp_err_t ok = httpd_resp_send_chunk(req, (const char*)kopf, sizeof(kopf));
  xStreamBufferReset(puffer);
  zuhoerer++;
  uint8_t daten[2048];
  while (ok == ESP_OK) {
    size_t n = xStreamBufferReceive(puffer, daten, sizeof(daten), pdMS_TO_TICKS(500));
    if (n > 0) ok = httpd_resp_send_chunk(req, (const char*)daten, n & ~1u);
  }
  zuhoerer--;
  return ok;
}
