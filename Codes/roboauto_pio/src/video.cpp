#include <Arduino.h>
#include <SD.h>
#include <freertos/idf_additions.h>
#include <esp_heap_caps.h>
#include "config.h"
#include "video.h"
#include "kamera.h"
#include "karte.h"
#include "mikro.h"
#include "zeit.h"

// ============================================================
//  AVI-Datei (RIFF):  RIFF "AVI " { LIST "hdrl" {avih, LIST strl (Bild), [LIST strl (Ton)]}
//                                   LIST "movi" { "00dc" JPEG, "01wb" Ton, ... }  idx1 }
//  Der Kopf wird am Anfang mit Nullen reserviert und am Ende mit den echten Werten
//  (Bildzahl, Bildrate, Laenge) ueberschrieben. Das Inhaltsverzeichnis (idx1) sammelt sich
//  waehrenddessen im PSRAM.
// ============================================================

constexpr unsigned long VIDEO_MAX_MS   = 30UL * 60 * 1000;   // hoechstens 30 min je Datei
constexpr unsigned long VIDEO_BILD_MS  = 45;                 // hoechstens ca. 22 Bilder/s

struct Eintrag { uint32_t id, flags, ort, laenge; };

static volatile bool startWunsch = false, stoppWunsch = false, laeuft = false, mitTon = false, wunschTon = false;
static volatile unsigned long startMs = 0;
static volatile int zaehler = 0;

bool  videoLaeuft()   { return laeuft; }
bool  videoMitTon()   { return laeuft && mitTon; }
float videoSekunden() { return laeuft ? (millis() - startMs) / 1000.0f : 0; }
int   videoZaehler()  { return zaehler; }

bool videoAufnahmeStarten(bool ton) {
  if (!karteDa() || !kameraDa() || laeuft || startWunsch) return false;
  wunschTon = ton && mikroDa();
  startWunsch = true;
  return true;
}
void videoAufnahmeStoppen() { if (laeuft) stoppWunsch = true; }

// ---------- kleine Helfer zum Zusammenbauen des Kopfs ----------
struct Puffer {
  uint8_t d[400]; size_t n = 0;
  void u32(uint32_t v) { memcpy(d + n, &v, 4); n += 4; }
  void u16(uint16_t v) { memcpy(d + n, &v, 2); n += 2; }
  void cc(const char* s) { memcpy(d + n, s, 4); n += 4; }
};

static size_t kopfGroesse(bool ton) { return ton ? 314 : 212; }

static void kopfBauen(Puffer& k, bool ton, uint32_t breite, uint32_t hoehe, uint32_t bilder, float fps,
                      uint32_t maxBild, uint32_t tonBytes, uint32_t moviGroesse, uint32_t dateiGroesse) {
  uint32_t rate = (uint32_t)(fps * 1000 + 0.5f);                    // Bildrate in 1/1000 Bildern pro Sekunde
  uint32_t strlBild = 4 + 64 + 48, strlTon = 4 + 64 + 26;
  k.cc("RIFF"); k.u32(dateiGroesse - 8); k.cc("AVI ");
  k.cc("LIST"); k.u32(4 + 64 + 8 + strlBild + (ton ? 8 + strlTon : 0)); k.cc("hdrl");
  k.cc("avih"); k.u32(56);
  k.u32(fps > 0 ? (uint32_t)(1000000 / fps) : 50000); k.u32(fps > 0 ? (uint32_t)(dateiGroesse / (bilder / fps + 0.001f)) : 0);
  k.u32(0); k.u32(0x10 | 0x100);                                    // hat Index, verzahnt
  k.u32(bilder); k.u32(0); k.u32(ton ? 2 : 1); k.u32(maxBild + 8); k.u32(breite); k.u32(hoehe);
  k.u32(0); k.u32(0); k.u32(0); k.u32(0);
  // --- Bild ---
  k.cc("LIST"); k.u32(strlBild); k.cc("strl");
  k.cc("strh"); k.u32(56); k.cc("vids"); k.cc("MJPG"); k.u32(0); k.u16(0); k.u16(0); k.u32(0);
  k.u32(1000); k.u32(rate); k.u32(0); k.u32(bilder); k.u32(maxBild + 8); k.u32(0xFFFFFFFF); k.u32(0);
  k.u16(0); k.u16(0); k.u16(breite); k.u16(hoehe);
  k.cc("strf"); k.u32(40); k.u32(40); k.u32(breite); k.u32(hoehe); k.u16(1); k.u16(24); k.cc("MJPG");
  k.u32(breite * hoehe * 3); k.u32(0); k.u32(0); k.u32(0); k.u32(0);
  // --- Ton (PCM 16 kHz, 16 Bit, mono) ---
  if (ton) {
    k.cc("LIST"); k.u32(strlTon); k.cc("strl");
    k.cc("strh"); k.u32(56); k.cc("auds"); k.u32(0); k.u32(0); k.u16(0); k.u16(0); k.u32(0);
    k.u32(2); k.u32(MIKRO_RATE * 2); k.u32(0); k.u32(tonBytes / 2); k.u32(8192); k.u32(0xFFFFFFFF); k.u32(2);
    k.u16(0); k.u16(0); k.u16(0); k.u16(0);
    k.cc("strf"); k.u32(18); k.u16(1); k.u16(1); k.u32(MIKRO_RATE); k.u32(MIKRO_RATE * 2); k.u16(2); k.u16(16); k.u16(0);
  }
  k.cc("LIST"); k.u32(moviGroesse); k.cc("movi");
}

static void videoAufgabe(void*) {
  Eintrag* index = nullptr;
  size_t indexPlatz = 0;
  uint8_t* ton = (uint8_t*)heap_caps_malloc(8192, MALLOC_CAP_SPIRAM);   // Tonpuffer im PSRAM
  for (;;) {
    if (!startWunsch) { delay(20); continue; }
    startWunsch = false;
    stoppWunsch = false;
    bool tonAn = ton && wunschTon && mikroAufnahme(true);

    char tag[16], uhr[24], pfad[64];
    zeitTag(tag, sizeof(tag)); zeitUhr(uhr, sizeof(uhr));
    SD.mkdir("/videos");
    snprintf(pfad, sizeof(pfad), "/videos/%s", tag); SD.mkdir(pfad);
    snprintf(pfad, sizeof(pfad), "/videos/%s/video_%s.avi", tag, uhr);
    File f = SD.open(pfad, FILE_WRITE);
    if (!f) { Serial.println(">> Video: kann nicht auf die SD-Karte schreiben"); if (tonAn) mikroAufnahme(false); continue; }

    const size_t kopf = kopfGroesse(tonAn);
    uint8_t nullen[400] = {};
    f.write(nullen, kopf + 12);                       // Kopf + "LIST xxxx movi" vorerst leer
    const uint32_t moviOrt = kopf + 8;                // Bezugspunkt fuers Inhaltsverzeichnis: das "movi"
    uint32_t ort = kopf + 12, bilder = 0, tonBytes = 0, maxBild = 0, breite = 0, hoehe = 0, eintraege = 0;
    bool ok = true;
    mitTon = tonAn;
    startMs = millis();
    laeuft = true;
    Serial.printf(">> Video-Aufnahme: %s%s\n", pfad, tonAn ? " (mit Ton)" : "");

    auto eintragen = [&](const char* id, uint32_t laenge) {
      if (eintraege == indexPlatz) {
        size_t neu = indexPlatz ? indexPlatz * 2 : 4096;
        Eintrag* p = (Eintrag*)heap_caps_realloc(index, neu * sizeof(Eintrag), MALLOC_CAP_SPIRAM);
        if (!p) { ok = false; return; }
        index = p; indexPlatz = neu;
      }
      Eintrag& e = index[eintraege++];
      memcpy(&e.id, id, 4); e.flags = 0x10; e.ort = ort - moviOrt; e.laenge = laenge;
    };
    // An Stelle 'wo' schreiben (bis zu drei Teile hintereinander). Grosse SD-Karten sind beim internen Aufraeumen
    // manchmal laenger als 500 ms beschaeftigt - dann gibt der SD-Treiber auf, und FatFs sperrt die Datei.
    // Deshalb: Datei schliessen, kurz warten, neu oeffnen und dasselbe Stueck noch einmal an dieselbe Stelle.
    int schreibfehler = 0;
    auto schreibeAn = [&](uint32_t wo, const uint8_t* a, size_t na, const uint8_t* b, size_t nb, const uint8_t* c, size_t nc) {
      for (int versuch = 0; versuch < 5; versuch++) {
        if (versuch > 0) {
          schreibfehler++;
          Serial.printf(">> Video: Schreibfehler an Stelle %u, Versuch %d (SD-Karte beschaeftigt?)\n", (unsigned)wo, versuch + 1);
          f.close();
          delay(200 * versuch);
          f = SD.open(pfad, "r+");
          if (!f) continue;
        }
        if ((uint32_t)f.position() != wo && !f.seek(wo)) continue;
        if (f.write(a, na) != na) continue;
        if (nb && f.write(b, nb) != nb) continue;
        if (nc && f.write(c, nc) != nc) continue;
        return true;
      }
      return false;
    };
    static const uint8_t fuell = 0;
    auto stueck = [&](const char* id, const uint8_t* daten, uint32_t laenge) {
      if (!ok) return;
      uint8_t k[8]; memcpy(k, id, 4); memcpy(k + 4, &laenge, 4);
      if (!schreibeAn(ort, k, 8, daten, laenge, &fuell, laenge & 1)) { ok = false; return; }
      eintragen(id, laenge);                          // (merkt sich 'ort' = Anfang dieses Stuecks)
      ort += 8 + laenge + (laenge & 1);
    };

    unsigned long letzterFlush = millis();
    while (ok && !stoppWunsch && millis() - startMs < VIDEO_MAX_MS && karteDa()) {
      unsigned long t = millis();
      camera_fb_t* fb = kameraBild();
      if (fb) {
        if (!breite) { breite = fb->width; hoehe = fb->height; }
        if (fb->width == breite && fb->height == hoehe) {      // Aufloesung waehrend der Aufnahme gewechselt -> Bild auslassen
          stueck("00dc", fb->buf, fb->len);
          maxBild = max(maxBild, (uint32_t)fb->len);
          bilder++;
        }
        kameraBildFrei(fb);
      }
      if (tonAn) {
        size_t n = mikroAufnahmeLesen(ton, 8192);
        if (n) { stueck("01wb", ton, n); tonBytes += n; }
      }
      if (millis() - letzterFlush > 2000) { f.flush(); letzterFlush = millis(); }   // bei Stromausfall max. 2 s weg
      long rest = (long)VIDEO_BILD_MS - (long)(millis() - t);
      if (rest > 0) delay(rest);
    }
    float dauer = (millis() - startMs) / 1000.0f;
    if (tonAn) {                                          // restlichen Ton noch mitnehmen
      size_t n = mikroAufnahmeLesen(ton, 8192);
      if (n) { stueck("01wb", ton, n); tonBytes += n; }
      mikroAufnahme(false);
    }
    laeuft = false;

    // Inhaltsverzeichnis anhaengen, dann den Kopf mit den echten Werten schreiben - auch nach einem
    // Abbruch: so bleibt alles bis dahin Aufgenommene ein abspielbares Video
    uint32_t moviGroesse = ort - moviOrt;
    uint32_t idxLaenge = eintraege * sizeof(Eintrag);
    uint8_t idxKopf[8]; memcpy(idxKopf, "idx1", 4); memcpy(idxKopf + 4, &idxLaenge, 4);
    bool fertig = schreibeAn(ort, idxKopf, 8, (const uint8_t*)index, idxLaenge, nullptr, 0);
    uint32_t dateiGroesse = ort + 8 + idxLaenge;
    float fps = dauer > 0 ? bilder / dauer : 0;
    Puffer k;
    kopfBauen(k, tonAn, breite, hoehe, bilder, fps, maxBild, tonBytes, moviGroesse, dateiGroesse);
    fertig = schreibeAn(0, k.d, k.n, nullptr, 0, nullptr, 0) && fertig;
    if (f) f.close();
    if (index) { heap_caps_free(index); index = nullptr; indexPlatz = 0; }
    zaehler = zaehler + 1;
    Serial.printf(">> Video gespeichert: %.1f s, %u Bilder (%.1f/s)%s%s%s\n", dauer, (unsigned)bilder, fps,
                  tonAn ? ", mit Ton" : "", ok ? "" : " - ABGEBROCHEN (SD-Karte antwortet nicht mehr)",
                  fertig ? "" : " - Kopf/Index NICHT geschrieben");
    if (schreibfehler) Serial.printf(">> Video: %d Schreibfehler unterwegs (jeweils wiederholt)\n", schreibfehler);
  }
}

void videoStarten() {
  if (!karteDa() && !karteUnlesbar()) return;
  // Stapelspeicher im PSRAM - der interne Arbeitsspeicher wird fuer WLAN, Bluetooth und Kamera gebraucht
  xTaskCreatePinnedToCoreWithCaps(videoAufgabe, "video", 8192, nullptr, 1, nullptr, 0, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}
