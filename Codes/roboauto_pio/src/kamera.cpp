#include <Arduino.h>
#include <esp_camera.h>
#include <esp_http_server.h>
#include <atomic>
#include "config.h"
#include "kamera.h"
#include "medien.h"

// ============================================================
//  Kamera des XIAO ESP32S3 Sense (OV2640 oder OV3660).
//  Das Live-Video laeuft auf dem Medien-Server (Port 81) in einer eigenen Aufgabe -
//  so bremst es die Steuerung auf Port 80 nicht aus.
// ============================================================

static bool da = false;
static std::atomic<int> zuschauer{0};       // mehrere Browser = mehrere Aufgaben gleichzeitig
static int stufe = 1;

static const framesize_t GROESSEN[] = { FRAMESIZE_QVGA, FRAMESIZE_VGA, FRAMESIZE_SVGA };

bool kameraStarten() {
  camera_config_t k = {};
  k.pin_pwdn  = -1;   k.pin_reset = -1;   k.pin_xclk = 10;
  k.pin_sccb_sda = 40; k.pin_sccb_scl = 39;
  k.pin_d7 = 48; k.pin_d6 = 11; k.pin_d5 = 12; k.pin_d4 = 14;
  k.pin_d3 = 16; k.pin_d2 = 18; k.pin_d1 = 17; k.pin_d0 = 15;
  k.pin_vsync = 38; k.pin_href = 47; k.pin_pclk = 13;
  k.xclk_freq_hz = 20000000;
  // Kanal 7 / Timer 3: Motoren und Buzzer bekommen vom Arduino-Kern Kanal 0..4 und Timer 0..1
  k.ledc_channel = LEDC_CHANNEL_7;
  k.ledc_timer   = LEDC_TIMER_3;
  k.pixel_format = PIXFORMAT_JPEG;
  k.frame_size   = GROESSEN[stufe];
  k.jpeg_quality = KAMERA_JPEG_QUALI;
  k.fb_count     = 3;                        // Live-Video und SD-Karte holen sich gleichzeitig Bilder
  k.fb_location  = CAMERA_FB_IN_PSRAM;
  k.grab_mode    = CAMERA_GRAB_LATEST;       // immer das neueste Bild (keine Verzoegerung)

  esp_err_t fehler = esp_camera_init(&k);
  if (fehler != ESP_OK) {
    Serial.printf(">> FEHLER: Kamera nicht gefunden (0x%x) - steckt das Sense-Board richtig?\n", fehler);
    return false;
  }
  sensor_t* s = esp_camera_sensor_get();
  if (s && KAMERA_DREHEN) { s->set_vflip(s, 1); s->set_hmirror(s, 1); }
  da = true;
  Serial.printf(">> Kamera bereit (Sensor %s)\n", s && s->id.PID == OV3660_PID ? "OV3660" : "OV2640");
  return true;
}

bool kameraDa()          { return da; }
int  kameraZuschauer()   { return zuschauer; }
int  kameraAufloesungStufe() { return stufe; }

void kameraAufloesung(int neu) {
  if (!da) return;
  stufe = constrain(neu, 0, 2);
  sensor_t* s = esp_camera_sensor_get();
  if (s) s->set_framesize(s, GROESSEN[stufe]);
}

camera_fb_t* kameraBild() { return da ? esp_camera_fb_get() : nullptr; }
void kameraBildFrei(camera_fb_t* bild) { if (bild) esp_camera_fb_return(bild); }

// ---------- Seiten fuer den Medien-Server (medien.cpp) ----------
#define GRENZE "roboautobild"
static const char* STREAM_TYP   = "multipart/x-mixed-replace;boundary=" GRENZE;
static const char* STREAM_TRENN = "\r\n--" GRENZE "\r\n";
static const char* STREAM_KOPF  = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

esp_err_t kameraStreamSenden(httpd_req_t* req) {
  if (!da) { httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "keine Kamera"); return ESP_FAIL; }
  httpd_resp_set_type(req, STREAM_TYP);
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  zuschauer++;
  esp_err_t ok = ESP_OK;
  char kopf[64];
  while (ok == ESP_OK) {
    unsigned long start = millis();
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) { delay(20); continue; }
    size_t n = snprintf(kopf, sizeof(kopf), STREAM_KOPF, (unsigned)fb->len);
    ok = httpd_resp_send_chunk(req, STREAM_TRENN, strlen(STREAM_TRENN));
    if (ok == ESP_OK) ok = httpd_resp_send_chunk(req, kopf, n);
    if (ok == ESP_OK) ok = httpd_resp_send_chunk(req, (const char*)fb->buf, fb->len);
    esp_camera_fb_return(fb);
    long rest = (long)KAMERA_STREAM_MS - (long)(millis() - start);   // Bildrate begrenzen (WLAN schonen)
    if (rest > 0) delay(rest);
  }
  zuschauer--;
  return ok;                                   // Browser hat die Seite verlassen
}

esp_err_t kameraBildSenden(httpd_req_t* req) {
  camera_fb_t* fb = kameraBild();
  if (!fb) { httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "kein Bild"); return ESP_FAIL; }
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=roboauto.jpg");
  esp_err_t ok = httpd_resp_send(req, (const char*)fb->buf, fb->len);
  kameraBildFrei(fb);
  return ok;
}
