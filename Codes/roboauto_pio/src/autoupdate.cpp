#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <Preferences.h>
#include <esp_ota_ops.h>
#include "config.h"
#include "autoupdate.h"
#include "zertifikate.h"
#include "einstellungen.h"
#include "antrieb.h"
#include "ledring.h"
#include "buzzer.h"

#ifndef FIRMWARE_BUILD
#define FIRMWARE_BUILD 0          // GitHub setzt hier die Build-Nummer ein, selbst gebaut = 0
#endif

static UpdateZustand zustand = UPD_RUHE;
static int neuBuild = 0, prozent = 0, groesse = 0;
static String datei, md5;
static bool sucheGewuenscht = false, installGewuenscht = false;
static bool probeLauf = false, zurueckGemeldet = false;
static int gesperrt = 0;                       // Build, die schon einmal kaputt war -> nicht nochmal automatisch
static unsigned long wlanSeit = 0, letzteSuche = 0;
static bool ersteSucheErledigt = false;

int  autoUpdateBuild()    { return FIRMWARE_BUILD; }
int  autoUpdateNeu()      { return neuBuild; }
int  autoUpdateZustand()  { return zustand; }
int  autoUpdateProzent()  { return prozent; }
bool autoUpdateZurueckgesetzt() { return zurueckGemeldet; }
void autoUpdateSuchen()       { sucheGewuenscht = true; }
void autoUpdateInstallieren() { installGewuenscht = true; }

// ------------------------------------------------------------
//  Rueckfall-Schutz (laeuft ganz am Anfang von setup)
// ------------------------------------------------------------
void autoUpdateStart() {
  Preferences p;
  p.begin("update", false);
  gesperrt  = p.getInt("gesperrt", 0);
  probeLauf = p.getBool("probe", false);
  if (probeLauf) {
    int starts = p.getInt("starts", 0) + 1;
    p.putInt("starts", starts);
    Serial.printf(">> Auto-Update: neue Version im Probelauf (Start %d von %d)\n", starts, UPDATE_PROBE_STARTS);
    if (starts > UPDATE_PROBE_STARTS) {
      // Die neue Version kommt nicht sauber hoch -> zurueck zur alten
      p.putBool("probe", false);
      p.putInt("starts", 0);
      p.putInt("gesperrt", p.getInt("neu", 0));
      p.putBool("zurueck", true);
      p.end();
      const esp_partition_t* alt = esp_ota_get_next_update_partition(NULL);
      if (alt && esp_ota_set_boot_partition(alt) == ESP_OK) {
        Serial.println(">> Auto-Update: neue Version startet nicht sauber - schalte auf die alte zurueck!");
        delay(200);
        ESP.restart();
      }
      return;
    }
  }
  zurueckGemeldet = p.getBool("zurueck", false);
  if (zurueckGemeldet) {
    p.putBool("zurueck", false);
    Serial.printf(">> Auto-Update: auf die alte Version zurueckgeschaltet (Build %d ist gesperrt)\n", gesperrt);
  }
  p.end();
  Serial.printf(">> Firmware: Build %d%s\n", FIRMWARE_BUILD, FIRMWARE_BUILD == 0 ? " (selbst gebaut)" : "");
}

// Probelauf erfolgreich: lief lange genug und hat WLAN -> Version gilt als gut
static void bestaetigen() {
  probeLauf = false;
  Preferences p;
  p.begin("update", false);
  p.putBool("probe", false);
  p.putInt("starts", 0);
  p.end();
  esp_ota_mark_app_valid_cancel_rollback();
  Serial.println(">> Auto-Update: neue Version laeuft stabil - bestaetigt");
}

// ------------------------------------------------------------
//  Kleine Helfer fuer version.json  {"build":57,"datei":"roboauto-update.bin","groesse":123,"md5":"..."}
// ------------------------------------------------------------
static String jsonText(const String& j, const char* name) {
  String k = String("\"") + name + "\"";
  int i = j.indexOf(k); if (i < 0) return "";
  i = j.indexOf(':', i); if (i < 0) return "";
  i++;
  while (i < (int)j.length() && (j[i] == ' ' || j[i] == '"')) i++;
  int e = i;
  while (e < (int)j.length() && j[e] != '"' && j[e] != ',' && j[e] != '}') e++;
  return j.substring(i, e);
}

static bool suchen() {
  zustand = UPD_SUCHT;
  WiFiClientSecure client;
  client.setCACert(UPDATE_ZERTIFIKATE);
  HTTPClient http;
  http.setTimeout(8000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  String url = String(UPDATE_URL) + "version.json?t=" + String(millis());   // ?t= gegen Zwischenspeicher
  if (!http.begin(client, url)) { zustand = UPD_FEHLER; return false; }
  int code = http.GET();
  if (code != 200) {
    Serial.printf(">> Auto-Update: Suche fehlgeschlagen (%d %s)\n", code, http.errorToString(code).c_str());
    http.end();
    zustand = UPD_FEHLER;
    return false;
  }
  String j = http.getString();
  http.end();
  neuBuild = jsonText(j, "build").toInt();
  datei    = jsonText(j, "datei");
  md5      = jsonText(j, "md5");
  groesse  = jsonText(j, "groesse").toInt();
  if (neuBuild <= 0 || datei.length() == 0) { zustand = UPD_FEHLER; return false; }
  zustand = neuBuild > FIRMWARE_BUILD ? UPD_VERFUEGBAR : UPD_AKTUELL;
  Serial.printf(">> Auto-Update: auf GitHub Build %d, hier Build %d -> %s\n", neuBuild, FIRMWARE_BUILD,
                zustand == UPD_VERFUEGBAR ? "Update verfuegbar" : "aktuell");
  return true;
}

// ------------------------------------------------------------
//  Herunterladen und installieren (blockiert ca. 30-60 s, Motoren sind aus)
// ------------------------------------------------------------
static void installieren() {
  antriebStopp();
  zustand = UPD_LAEDT;
  prozent = 0;
  Serial.printf(">> Auto-Update: lade Build %d ...\n", neuBuild);
  buzzerPiepsen(1800, 80);
  ringFortschritt(0, 0x8000FF);

  WiFiClientSecure client;
  client.setCACert(UPDATE_ZERTIFIKATE);
  HTTPClient http;
  http.setTimeout(15000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  bool ok = false;
  if (http.begin(client, String(UPDATE_URL) + datei + "?b=" + String(neuBuild)) && http.GET() == 200) {
    int laenge = http.getSize();
    if (laenge > 0 && (groesse == 0 || laenge == groesse) && Update.begin(laenge, U_FLASH)) {
      if (md5.length() == 32) Update.setMD5(md5.c_str());          // prueft am Ende, ob alles heil angekommen ist
      Update.onProgress([](size_t fertig, size_t gesamt) {
        int p = gesamt ? fertig * 100 / gesamt : 0;
        if (p != prozent) { prozent = p; ringFortschritt(p, 0x8000FF); }
      });
      size_t geschrieben = Update.writeStream(*http.getStreamPtr());
      ok = geschrieben == (size_t)laenge && Update.end(true);
      if (!ok) { Serial.printf(">> Auto-Update: Fehler %s\n", Update.errorString()); Update.abort(); }
    } else {
      Serial.printf(">> Auto-Update: Download ungueltig (Laenge %d)\n", laenge);
    }
  } else {
    Serial.println(">> Auto-Update: Download fehlgeschlagen");
  }
  http.end();

  if (!ok) {
    zustand = UPD_FEHLER;
    ringFortschritt(100, 0xFF0000);
    buzzerPiepsen(300, 400);
    return;
  }
  // Neue Version merken und im Probelauf starten
  Preferences p;
  p.begin("update", false);
  p.putBool("probe", true);
  p.putInt("starts", 0);
  p.putInt("neu", neuBuild);
  p.end();
  Serial.println(">> Auto-Update: fertig - starte neu");
  ringFortschritt(100, 0x00FF40);
  buzzerPiepsen(2600, 150);
  delay(300);
  ESP.restart();
}

// ------------------------------------------------------------
void autoUpdateLoop(bool wlanDa, bool ruhig) {
  unsigned long jetzt = millis();
  // Probelauf: nach einer Minute mit WLAN (oder 3 Minuten ohne) gilt die neue Version als gut
  if (probeLauf && (jetzt > 3 * UPDATE_BESTAETIGEN_MS || (wlanDa && jetzt > UPDATE_BESTAETIGEN_MS))) bestaetigen();

  if (!wlanDa) { wlanSeit = 0; return; }
  if (wlanSeit == 0) wlanSeit = jetzt;

  // Suchen: kurz nach dem Verbinden, dann regelmaessig - oder wenn die Webseite es will
  bool faellig = (!ersteSucheErledigt && jetzt - wlanSeit > UPDATE_ERSTE_SUCHE_MS) ||
                 (ersteSucheErledigt && jetzt - letzteSuche > UPDATE_INTERVALL_MS);
  if (sucheGewuenscht || (faellig && ruhig && einst().autoUpdate)) {
    if (sucheGewuenscht) antriebStopp();          // Suche blockiert kurz (1-2 s) -> sicherheitshalber anhalten
    sucheGewuenscht = false;
    ersteSucheErledigt = true;
    letzteSuche = jetzt;
    suchen();
  }

  // Installieren: per Knopf sofort, automatisch nur in Ruhe und nur bei GitHub-Versionen
  if (zustand == UPD_VERFUEGBAR) {
    bool automatisch = einst().autoUpdate && ruhig && FIRMWARE_BUILD > 0 && neuBuild != gesperrt;
    if (installGewuenscht || automatisch) installieren();
  }
  installGewuenscht = false;
}
