#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <Preferences.h>
#include "config.h"
#include "autoupdate.h"
#include "musik.h"
#include "buzzer.h"
#include "web.h"
#include "webseite.h"
#include "webseite_gz.h"   // gepackte Weboberflaeche (erzeugt von scripts/webseite_gz.py)
#include "wlan_zugang.h"
#include "akku.h"
#include "antrieb.h"
#include "lichter.h"
#include "ledring.h"
#include "figuren.h"
#include "aufnahme.h"
#include "balance.h"
#include "gyro.h"
#include "einstellungen.h"

static WebServer server(80);
static bool serverLaeuft = false;
static bool staVerbunden = false;           // im Heim-WLAN
static bool setupAktiv = false;             // eigenes Einrichtungs-WLAN offen
static DNSServer dns;                       // leitet im Einrichtungsmodus alle Adressen aufs Auto
static String wlanName, wlanPasswort;       // gespeicherte Zugangsdaten
static unsigned long neustartUm = 0;        // nach dem Speichern kurz warten, dann neu starten
static unsigned long letzterNeuversuch = 0; // Einrichtungsmodus: wann zuletzt das Heim-WLAN probiert

static int x = 0, y = 0;
static int tasten = 0;                        // gehaltene Tasten (Bits)
static int aktion = 0;                        // offene Einmal-Aktion
static int modus = 0;                         // nur fuer die Anzeige
static unsigned long letzterFahrbefehl = 0;   // Totmann: ohne neue Befehle -> Stopp
static unsigned long letzteAbfrage = 0;       // Seite offen?
static unsigned long wlanStart = 0;
static int  ereignis = 0;                     // fuer webEreignis()
static bool fehlerGemeldet = false;

#ifndef OTA_PASSWORT
#define OTA_PASSWORT ""
#endif

// ------------------------------------------------------------
//  Updates ueber WLAN (ArduinoOTA): VS Code schickt die neue Firmware,
//  das Auto schreibt sie in den freien Programmplatz und startet neu.
// ------------------------------------------------------------
static void starteOta() {
  ArduinoOTA.setHostname(WEB_NAME);
  if (strlen(OTA_PASSWORT) > 0) ArduinoOTA.setPassword(OTA_PASSWORT);

  ArduinoOTA.onStart([]() {
    antriebStopp();                       // Sicherheit: waehrend des Updates nicht fahren
    Serial.println(">> WLAN-UPDATE startet...");
    buzzerPiepsen(1800, 80);
    ringFortschritt(0, 0x8000FF);
  });
  ArduinoOTA.onProgress([](unsigned int fertig, unsigned int gesamt) {
    static int letzte = -1;
    int prozent = gesamt ? fertig * 100 / gesamt : 0;
    if (prozent != letzte) {
      letzte = prozent;
      ringFortschritt(prozent, 0x8000FF); // violetter Balken fuellt sich
      if (prozent % 10 == 0) Serial.printf(">> Update: %d %%\n", prozent);
    }
  });
  ArduinoOTA.onEnd([]() {
    Serial.println(">> WLAN-UPDATE fertig - starte neu");
    ringFortschritt(100, 0x00FF40);       // gruen = geschafft
    buzzerPiepsen(2600, 150);
    delay(50);
  });
  ArduinoOTA.onError([](ota_error_t fehler) {
    Serial.printf(">> WLAN-UPDATE FEHLER %u (Passwort? Verbindung?)\n", fehler);
    ringFortschritt(100, 0xFF0000);       // rot = fehlgeschlagen
    buzzerPiepsen(300, 400);
  });
  ArduinoOTA.begin();
  Serial.println(">> WLAN-Updates bereit");
}

// ------------------------------------------------------------
//  Zugangsdaten im Speicher des ESP32 (bleiben auch nach Updates erhalten)
// ------------------------------------------------------------
// Das Auto merkt sich mehrere WLANs (zu Hause, bei Freunden ...). Liste: zuletzt benutztes zuerst.
constexpr int WLAN_MAX = 8;
static String netzName[WLAN_MAX], netzPass[WLAN_MAX];
static int netzAnzahl = 0;

static void speichereListe() {
  Preferences p;
  p.begin("roboauto", false);
  p.putInt("anzahl", netzAnzahl);
  for (int i = 0; i < WLAN_MAX; i++) {
    String ks = "s" + String(i), kp = "p" + String(i);
    if (i < netzAnzahl) { p.putString(ks.c_str(), netzName[i]); p.putString(kp.c_str(), netzPass[i]); }
    else if (p.isKey(ks.c_str())) { p.remove(ks.c_str()); p.remove(kp.c_str()); }
  }
  p.end();
}

static void ladeZugang() {
  Preferences p;
  p.begin("roboauto", false);
  // Einmalig: falls noch in include/wlan_zugang.h eingetragen -> in den Speicher uebernehmen
  if (!p.getBool("migriert", false)) {
    if (strlen(WLAN_NAME) > 0) {
      p.putString("ssid", WLAN_NAME);
      p.putString("pass", WLAN_PASSWORT);
    }
    p.putBool("migriert", true);
  }
  int anzahl = p.getInt("anzahl", -1);
  bool umstellen = anzahl < 0;
  if (umstellen) {                                  // alte Version: nur ein Netz unter "ssid"/"pass"
    String alt = p.getString("ssid", "");
    netzAnzahl = 0;
    if (alt.length()) { netzName[0] = alt; netzPass[0] = p.getString("pass", ""); netzAnzahl = 1; }
  } else {
    netzAnzahl = constrain(anzahl, 0, WLAN_MAX);
    for (int i = 0; i < netzAnzahl; i++) {
      netzName[i] = p.getString(("s" + String(i)).c_str(), "");
      netzPass[i] = p.getString(("p" + String(i)).c_str(), "");
    }
  }
  p.end();
  if (umstellen) speichereListe();
  Serial.printf(">> WLAN: %d Netzwerk(e) gespeichert\n", netzAnzahl);
  for (int i = 0; i < netzAnzahl; i++) Serial.printf("   %d. %s\n", i + 1, netzName[i].c_str());
}

static int netzFinde(const String& name) {
  for (int i = 0; i < netzAnzahl; i++) if (netzName[i] == name) return i;
  return -1;
}

static void netzNachVorne(int i) {
  if (i <= 0 || i >= netzAnzahl) return;
  String n = netzName[i], pw = netzPass[i];
  for (int k = i; k > 0; k--) { netzName[k] = netzName[k - 1]; netzPass[k] = netzPass[k - 1]; }
  netzName[0] = n; netzPass[0] = pw;
}

// Neues Netz merken (oder Passwort aendern) - kommt an die erste Stelle
static void merkeNetz(const String& name, const String& pass) {
  int i = netzFinde(name);
  if (i < 0) {
    if (netzAnzahl == WLAN_MAX) netzAnzahl--;      // Liste voll: das am laengsten nicht benutzte faellt raus
    i = netzAnzahl++;
    netzName[i] = name;
  }
  netzPass[i] = pass;
  netzNachVorne(i);
  speichereListe();
}

static void netzEntfernen(int i) {
  if (i < 0 || i >= netzAnzahl) return;
  for (int k = i; k < netzAnzahl - 1; k++) { netzName[k] = netzName[k + 1]; netzPass[k] = netzPass[k + 1]; }
  netzAnzahl--;
  speichereListe();
}

void webWlanVergessen() {
  netzAnzahl = 0;
  speichereListe();
  Serial.println(">> Alle WLAN-Zugangsdaten geloescht - starte im Einrichtungsmodus neu");
  neustartUm = millis() + 800;
}

// ------------------------------------------------------------
//  Verbinden: erst suchen, dann das staerkste bekannte Netz nehmen.
//  Klappt es nicht, das naechste bekannte probieren. Am Ende: Einrichtungsmodus.
// ------------------------------------------------------------
enum WlanPhase { W_AUS, W_SUCHE, W_VERBINDE };
static WlanPhase phase = W_AUS;
static int kandidat[WLAN_MAX], kandAnzahl = 0, kandPos = 0, kandAktuell = -1;

static void starteSuche() {
  WiFi.scanNetworks(true);
  phase = W_SUCHE;
}

static void sucheAuswerten() {
  int n = WiFi.scanComplete();
  int staerke[WLAN_MAX];
  kandAnzahl = 0; kandPos = 0;
  for (int j = 0; j < netzAnzahl; j++) {
    int best = -999;
    for (int i = 0; i < n; i++) if (WiFi.SSID(i) == netzName[j] && WiFi.RSSI(i) > best) best = WiFi.RSSI(i);
    if (best == -999) continue;
    int k = kandAnzahl++;                           // nach Signalstaerke einsortieren
    while (k > 0 && staerke[k - 1] < best) { kandidat[k] = kandidat[k - 1]; staerke[k] = staerke[k - 1]; k--; }
    kandidat[k] = j; staerke[k] = best;
  }
  // Nichts Bekanntes in Reichweite: das zuletzt benutzte trotzdem probieren (koennte ein verstecktes Netz sein)
  if (kandAnzahl == 0 && netzAnzahl > 0 && !setupAktiv) kandidat[kandAnzahl++] = 0;
  Serial.printf(">> WLAN-Suche: %d Netze gefunden, davon %d bekannt\n", max(n, 0), kandAnzahl);
}

// true = Versuch gestartet, false = keine Kandidaten mehr
static bool verbindeNaechsten() {
  if (kandPos >= kandAnzahl) { phase = W_AUS; kandAktuell = -1; return false; }
  kandAktuell = kandidat[kandPos++];
  wlanName = netzName[kandAktuell];
  wlanPasswort = netzPass[kandAktuell];
  WiFi.begin(wlanName.c_str(), wlanPasswort.c_str());
  wlanStart = millis();
  phase = W_VERBINDE;
  Serial.printf(">> WLAN: verbinde mit \"%s\" ...\n", wlanName.c_str());
  return true;
}

// ------------------------------------------------------------
//  Einrichtungsmodus: eigenes WLAN "Roboauto-Setup" + Einrichtungsseite
// ------------------------------------------------------------
static void starteEinrichtung() {
  WiFi.disconnect();                        // keine Verbindungsversuche mehr (stoert sonst das eigene WLAN)
  WiFi.mode(WIFI_AP_STA);                   // AP fuer das Handy, STA nur fuer die Netzwerksuche
  WiFi.softAP(SETUP_WLAN_NAME);
  dns.start(53, "*", WiFi.softAPIP());      // jede Adresse -> Auto => Seite oeffnet sich von selbst
  WiFi.scanNetworks(true);                  // Netzwerke im Hintergrund suchen
  setupAktiv = true;
  letzterNeuversuch = millis();
  ereignis = 3;
  Serial.printf(">> EINRICHTUNG: Mit dem Handy ins WLAN \"%s\" gehen, die Seite oeffnet sich\n"
                "   von selbst (sonst http://%s aufrufen)\n",
                SETUP_WLAN_NAME, WiFi.softAPIP().toString().c_str());
}

// HTML-Sonderzeichen in Netzwerknamen entschaerfen
// Fahrtrichtung fuer den Kompass: 0..360 Grad im Uhrzeigersinn, 0 = Richtung beim Einschalten
static float kurs() {
  float k = fmodf(-gyroWinkel(), 360.0f);
  return k < 0 ? k + 360.0f : k;
}

static String html(const String& t) {
  String r;
  for (unsigned i = 0; i < t.length(); i++) {
    char c = t[i];
    if (c == '<') r += "&lt;"; else if (c == '>') r += "&gt;";
    else if (c == '&') r += "&amp;"; else if (c == '"') r += "&quot;"; else r += c;
  }
  return r;
}

// Signalstaerke als 4 kleine Balken (SVG)
static String signalSymbol(int balken) {
  String s = "<svg width='22' height='18' viewBox='0 0 22 18'>";
  for (int i = 0; i < 4; i++) {
    int h = 5 + i * 4;
    s += "<rect x='" + String(i * 6) + "' y='" + String(18 - h) + "' width='4' height='" + String(h) +
         "' rx='1.5' fill='" + (i < balken ? "#4f9cff" : "#323a45") + "'/>";
  }
  return s + "</svg>";
}

// Seite = Kopf + Inhalt + Fuss (Design und Skript stehen in webseite.h)
static void sendeSeite(const String& inhalt) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "text/html", String(FPSTR(EINRICHTUNG_KOPF)) + inhalt + String(FPSTR(EINRICHTUNG_FUSS)));
}

static void sendeEinrichtungsSeite() {
  String s = "<div class='karte'>";
  if (staVerbunden) {
    s += "<div class='meldung ok'>&#9989;<span><span data-en='Currently connected to'>Aktuell verbunden mit</span> <b>" +
         html(wlanName) + "</b> (" + WiFi.localIP().toString() + ")</span></div>";
  }
  s += "<div class='schritte'><span class='an' data-en='1 &middot; Network'>1 &middot; Netzwerk</span>"
       "<span data-en='2 &middot; Password'>2 &middot; Passwort</span><span data-en='3 &middot; Done'>3 &middot; Fertig</span></div>"
       "<form method='POST' action='/speichern'>"
       "<div class='zeile'><h2 data-en='Choose your Wi-Fi'>WLAN ausw&auml;hlen</h2>"
       "<a class='neu' href='/scan' data-en='&#8635; Search again'>&#8635; Neu suchen</a></div><div class='netze'>";
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING || n == WIFI_SCAN_FAILED) {
    s += "<div id='sucht' class='leise' data-en='Searching for networks&hellip;'>Suche nach Netzwerken&hellip;</div>"
         "<div class='lade'></div><div class='lade'></div><div class='lade'></div>";
    if (n == WIFI_SCAN_FAILED) WiFi.scanNetworks(true);
  } else {
    // doppelte Namen nur einmal (staerkstes Signal), nach Signalstaerke sortiert
    int idx[30]; int anzahl = 0;
    for (int i = 0; i < n && anzahl < 30; i++) {
      if (WiFi.SSID(i).length() == 0) continue;
      int doppelt = -1;
      for (int j = 0; j < anzahl; j++) if (WiFi.SSID(idx[j]) == WiFi.SSID(i)) doppelt = j;
      if (doppelt < 0) idx[anzahl++] = i;
      else if (WiFi.RSSI(i) > WiFi.RSSI(idx[doppelt])) idx[doppelt] = i;
    }
    for (int a = 1; a < anzahl; a++)
      for (int b = a; b > 0 && WiFi.RSSI(idx[b]) > WiFi.RSSI(idx[b - 1]); b--) { int t = idx[b]; idx[b] = idx[b - 1]; idx[b - 1] = t; }
    if (anzahl == 0) s += "<div class='leise' data-en='No networks found.'>Keine Netzwerke gefunden.</div>";
    for (int k = 0; k < anzahl && k < 20; k++) {
      int i = idx[k];
      String name = html(WiFi.SSID(i));
      bool offen = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
      bool bekannt = netzFinde(WiFi.SSID(i)) >= 0;
      int balken = constrain(map(WiFi.RSSI(i), -90, -45, 1, 4), 1, 4);
      s += "<label class='netz'><input type='radio' name='ssid' value='" + name + "' data-offen='" + (offen ? "1" : "0") + "' data-bekannt='" + (bekannt ? "1" : "0") + "'>" +
           signalSymbol(balken) + "<span class='name'>" + name + "</span>" +
           (bekannt ? "<span class='marke' data-en='saved'>gespeichert</span>" : "") +
           (offen ? "" : "<span title='gesch&uuml;tzt'>&#128274;</span>") + "<span class='haken'>&#10003;</span></label>";
    }
  }
  s += "</div><details class='hand'><summary data-en='Hidden network? Enter the name by hand'>Verstecktes Netz? Name von Hand eingeben</summary>"
       "<input class='text' id='hand' name='ssid_hand' placeholder='WLAN-Name' data-en-ph='Wi-Fi name' autocapitalize='off' autocorrect='off'></details>"
       "<label class='feld' for='pw' data-en='Password'>Passwort</label>"
       "<div class='pwfeld'><input class='text' id='pw' name='pass' type='password' placeholder='WLAN-Passwort' data-en-ph='Wi-Fi password' autocapitalize='off' autocorrect='off'>"
       "<button type='button' class='auge' id='auge' aria-label='Passwort zeigen'>&#128065;</button></div>"
       "<button class='haupt' id='los' type='submit' disabled data-en='Save &amp; connect'>Speichern &amp; verbinden</button></form></div>"
       "<p class='hinweis' data-en='The car only supports 2.4&nbsp;GHz Wi-Fi.'>Das Auto kann nur 2,4-GHz-WLAN.</p>";
  if (netzAnzahl > 0) {
    s += "<div class='karte'><div class='zeile'><h2 data-en='Saved networks'>Gespeicherte Netzwerke</h2><span class='leise'>" +
         String(netzAnzahl) + " / " + String(WLAN_MAX) + "</span></div><div class='gespeichert'>";
    for (int i = 0; i < netzAnzahl; i++) {
      bool aktiv = staVerbunden && netzName[i] == wlanName;
      s += "<div class='gsp'><span class='name'>" + html(netzName[i]) + "</span>" +
           (aktiv ? "<span class='marke gruen' data-en='connected'>verbunden</span>" : "") +
           "<a class='weg' href='/entfernen?i=" + String(i) + "' data-name='" + html(netzName[i]) + "' title='vergessen'>&#10005;</a></div>";
    }
    s += "</div><p class='hinweis' style='text-align:left;margin-top:12px' data-en='The car remembers up to 8 networks and automatically connects to the strongest known one &ndash; at home, at friends&rsquo; places, anywhere.'>"
         "Das Auto merkt sich bis zu 8 Netzwerke und verbindet sich beim Einschalten automatisch mit dem st&auml;rksten bekannten &ndash; zu Hause, bei Freunden, &uuml;berall.</p></div>";
  }
  if (staVerbunden) s += "<p class='hinweis'><a href='/' data-en='&larr; back to the controls'>&larr; zur&uuml;ck zur Steuerung</a></p>";
  sendeSeite(s);
}

void webStarten() {
  ladeZugang();
  WiFi.setHostname(WEB_NAME);

  // --- Seiten ---
  server.on("/", HTTP_GET, []() {
    if (setupAktiv) sendeEinrichtungsSeite();
    else {
      // Browser soll die Seite nicht zwischenspeichern - sonst laeuft nach einem Update noch die alte
      server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
      // gzip-gepackt: der Browser entpackt selbst, spart ca. 70 KB Flash
      server.sendHeader("Content-Encoding", "gzip");
      server.send_P(200, "text/html", (const char*)WEBSEITE_GZ, WEBSEITE_GZ_LAENGE);
    }
  });
  server.on("/wlan", HTTP_GET, sendeEinrichtungsSeite);
  server.on("/scan", HTTP_GET, []() {
    WiFi.scanNetworks(true);
    server.sendHeader("Location", "/wlan");
    server.send(302, "text/plain", "");
  });
  server.on("/speichern", HTTP_POST, []() {
    String name = server.arg("ssid_hand");
    if (name.length() == 0) name = server.arg("ssid");
    String pass = server.arg("pass");
    if (name.length() == 0) {
      sendeSeite("<div class='karte erfolg'><div class='kreis rot'><span style='font-size:40px'>&#10005;</span></div>"
                 "<h2 data-en='Please choose a Wi-Fi'>Bitte ein WLAN ausw&auml;hlen</h2>"
                 "<a class='neu' href='/wlan' data-en='&larr; back'>&larr; zur&uuml;ck</a></div>");
      return;
    }
    int bekannt = netzFinde(name);
    if (pass.length() == 0 && bekannt >= 0) pass = netzPass[bekannt];   // leer gelassen = gespeichertes Passwort behalten
    merkeNetz(name, pass);
    Serial.printf(">> Neue WLAN-Daten gespeichert: \"%s\" - starte neu\n", name.c_str());
    sendeSeite("<div class='karte erfolg'>"
               "<div class='schritte' style='width:100%'><span class='fertig' data-en='1 &middot; Network'>1 &middot; Netzwerk</span>"
               "<span class='fertig' data-en='2 &middot; Password'>2 &middot; Passwort</span><span class='fertig' data-en='3 &middot; Done'>3 &middot; Fertig</span></div>"
               "<div class='kreis'><span style='font-size:42px;color:#3ccf6e'>&#10003;</span></div>"
               "<h2 data-en='Saved!'>Gespeichert!</h2>"
               "<div class='leise'><span data-en='The car restarts and connects to'>Das Auto startet neu und verbindet sich mit</span> <b style='color:#eef1f5'>" + html(name) + "</b>.</div>"
               "<ol class='liste'><li data-en='Switch your phone/PC back to your normal Wi-Fi'>Handy/PC wieder ins normale WLAN wechseln</li>"
               "<li data-en='Open this address:'>Diese Adresse &ouml;ffnen:</li></ol>"
               "<div class='adresse'>http://" + String(WEB_NAME) + ".local</div>"
               "<p class='hinweis'><span data-en='If it does not work, the car opens the Wi-Fi'>Klappt es nicht, &ouml;ffnet das Auto nach 15&nbsp;s wieder das WLAN</span> &bdquo;" + String(SETUP_WLAN_NAME) + "&ldquo;.</p></div>");
    neustartUm = millis() + 1500;
  });
  server.on("/entfernen", HTTP_GET, []() {
    int i = server.arg("i").toInt();
    if (i >= 0 && i < netzAnzahl) Serial.printf(">> WLAN \"%s\" vergessen\n", netzName[i].c_str());
    netzEntfernen(i);
    server.sendHeader("Location", "/wlan");
    server.send(302, "text/plain", "");
  });
  server.on("/vergessen", HTTP_GET, []() {
    sendeSeite("<div class='karte erfolg'><div class='kreis'><span style='font-size:38px'>&#128465;</span></div>"
               "<h2 data-en='All saved Wi-Fi networks deleted'>Alle gespeicherten WLANs gel&ouml;scht</h2>"
               "<div class='leise'><span data-en='The car restarts and opens the Wi-Fi'>Das Auto startet neu und &ouml;ffnet das WLAN</span> &bdquo;" + String(SETUP_WLAN_NAME) + "&ldquo;.</div></div>");
    webWlanVergessen();
  });

  // --- Steuerung ---
  server.on("/fahr", HTTP_GET, []() {
    x = constrain(server.arg("x").toInt(), -JOYSTICK_MAX, JOYSTICK_MAX);
    y = constrain(server.arg("y").toInt(), -JOYSTICK_MAX, JOYSTICK_MAX);
    tasten = server.arg("t").toInt();
    letzterFahrbefehl = millis();
    letzteAbfrage = millis();
    server.send(200, "text/plain", "ok");
  });
  server.on("/aktion", HTTP_GET, []() {
    aktion = server.arg("a").toInt();
    letzteAbfrage = millis();
    server.send(200, "text/plain", "ok");
  });
  server.on("/status", HTTP_GET, []() {
    letzteAbfrage = millis();
    char ringText[8 * 7 + 1] = "";                   // 8 Ring-LEDs als "rrggbb,rrggbb,..."
    for (int i = 0; i < LED_RING_ANZAHL && i < 8; i++)
      snprintf(ringText + strlen(ringText), sizeof(ringText) - strlen(ringText), i ? ",%06lx" : "%06lx",
               (unsigned long)ringAnzeigeFarbe(i));
    char json[900];
    snprintf(json, sizeof(json),
             "{\"akku\":%d,\"prozent\":%d,\"usb\":%d,\"eco\":%d,\"links\":%d,\"rechts\":%d,\"rssi\":%d,"
             "\"licht\":%d,\"fern\":%d,\"warn\":%d,\"wende\":%d,\"modus\":%d,\"figur\":%d,"
             "\"rec\":%d,\"play\":%d,\"reclen\":%.1f,\"playpos\":%.1f,\"balance\":%d,"
             "\"nicken\":%.1f,\"rollen\":%.1f,\"kurs\":%.1f,\"dreh\":%.0f,\"kal\":%d,\"mt\":%d,\"lied\":%d,\"liedpos\":%d,\"ton\":%d,\"fw\":%d,\"fwneu\":%d,\"upd\":%d,\"updp\":%d,"
             "\"lampen\":\"%06lx,%06lx,%06lx,%06lx\",\"ring\":\"%s\"}",
             akkuWert(), akkuProzent(), akkuUsb(), akkuEco(), antriebLinks(), antriebRechts(), WiFi.RSSI(),
             lichtIstAn(), lichtIstFernlicht(), lichtIstWarnblinker(), antriebWendeAktiv(), modus, figurNummer(),
             aufnahmeLaeuft(), abspielenLaeuft(), aufnahmeSekunden(), abspielenSekunden(), balanceAktiv(),
             -gyroNeigungY(), gyroNeigungX(), kurs(), gyroDrehrate(),   // nicken: Schnauze hoch = positiv
             gyroKalibriertGerade(), antriebTestSeite() + 1, musikNummer(), musikFortschritt(), musikTon(),
             autoUpdateBuild(), autoUpdateNeu(), autoUpdateZustand(), autoUpdateProzent(),
             (unsigned long)lichtAnzeigeFarbe(0), (unsigned long)lichtAnzeigeFarbe(1),
             (unsigned long)lichtAnzeigeFarbe(2), (unsigned long)lichtAnzeigeFarbe(3), ringText);
    server.send(200, "application/json", json);
  });

  // Einstellungen lesen: /einstellungen   aendern: /einstellungen?tempo=150&toene=1 ...
  server.on("/einstellungen", HTTP_GET, []() {
    if (server.hasArg("standard")) {
      einstellungenStandard();
    } else if (server.hasArg("motorStandard")) {
      einstellungenMotorStandard();
    } else if (server.args() > 0) {
      for (int i = 0; i < server.args(); i++) einstellungSetzen(server.argName(i), server.arg(i));
      einstellungenSpeichern();
      if (server.hasArg("lautstaerke")) buzzerProbe();   // neue Lautstaerke gleich hoerbar
      Serial.println(">> Einstellungen gespeichert: " + einstellungenJson());
    }
    server.send(200, "application/json", einstellungenJson());
  });

  // Auto-Update: /update?a=suchen  oder  /update?a=installieren
  server.on("/update", HTTP_GET, []() {
    String a = server.arg("a");
    if (a == "suchen") autoUpdateSuchen();
    if (a == "installieren") { autoUpdateSuchen(); autoUpdateInstallieren(); }
    letzteAbfrage = millis();
    server.send(200, "text/plain", "ok");
  });

  server.on("/verlauf", HTTP_GET, []() {
    server.send(200, "application/json", akkuVerlaufJson());
  });

  // Alles andere: im Einrichtungsmodus auf die Einrichtungsseite umleiten.
  // Das loest auf Handys automatisch das "Im Netzwerk anmelden"-Fenster aus.
  server.onNotFound([]() {
    if (setupAktiv) {
      server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/");
      server.send(302, "text/plain", "");
    } else {
      server.send(404, "text/plain", "Nicht gefunden");
    }
  });
  // WICHTIG: Erst das WLAN einschalten, DANN den Webserver starten.
  // Andersherum stuerzt der ESP32 beim Start ab (Netzwerk-Stack noch nicht bereit).
  if (netzAnzahl == 0) {
    Serial.println(">> WLAN: noch keine Zugangsdaten gespeichert");
    starteEinrichtung();
  } else {
    WiFi.mode(WIFI_STA);
    starteSuche();                                 // im Hintergrund suchen, dann bekanntes Netz waehlen
  }
  server.begin();
}

// Kein bekanntes WLAN erreichbar -> Fehler melden und Einrichtungsmodus oeffnen
static void wlanFehlschlag() {
  if (fehlerGemeldet) return;
  fehlerGemeldet = true;
  Serial.println(">> WLAN: kein bekanntes Netz erreichbar (Passwort pruefen, nur 2,4 GHz!) - Einrichtungsmodus");
  starteEinrichtung();
  ereignis = 2;                           // Fehler hat Vorrang vor "Einrichtung offen"
}

void webUpdate() {
  server.handleClient();
  if (setupAktiv) dns.processNextRequest();

  if (neustartUm != 0 && (long)(millis() - neustartUm) >= 0) {
    antriebStopp();
    ESP.restart();
  }

  // Einrichtungsmodus, aber es gibt gespeicherte Daten: regelmaessig erneut das Heim-WLAN probieren.
  // Nur wenn gerade kein Handy mit "Roboauto-Setup" verbunden ist (der Versuch stoert das eigene WLAN kurz).
  // Klappt es, wird das Einrichtungs-WLAN geschlossen und alles laeuft normal weiter.
  // --- Verbindungsablauf: Suche auswerten / naechstes bekanntes Netz probieren ---
  if (phase == W_SUCHE && WiFi.scanComplete() != WIFI_SCAN_RUNNING) {
    sucheAuswerten();
    if (!verbindeNaechsten() && !setupAktiv) wlanFehlschlag();
  }
  if (phase == W_VERBINDE) {
    if (WiFi.status() == WL_CONNECTED) {
      phase = W_AUS;
      if (kandAktuell > 0) { netzNachVorne(kandAktuell); speichereListe(); }   // zuletzt benutzt -> nach vorne
      kandAktuell = 0;
    } else if (millis() - wlanStart > WLAN_TIMEOUT_MS) {
      Serial.printf(">> WLAN: \"%s\" klappt nicht\n", wlanName.c_str());
      WiFi.disconnect();
      if (!verbindeNaechsten() && !setupAktiv) wlanFehlschlag();
    }
  }

  if (setupAktiv && netzAnzahl > 0) {
    if (WiFi.status() == WL_CONNECTED) {
      dns.stop();
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_STA);
      setupAktiv = false;
      Serial.println(">> Heim-WLAN doch erreichbar - Einrichtungsmodus beendet");
    } else if (phase == W_AUS && millis() - letzterNeuversuch > WLAN_NEUVERSUCH_MS && WiFi.softAPgetStationNum() == 0) {
      letzterNeuversuch = millis();
      Serial.println(">> Einrichtungsmodus: suche erneut nach bekannten WLANs ...");
      starteSuche();
    }
  }

  // Erstmals im Heim-WLAN -> Name im Netz, Updates ueber WLAN, Meldung
  if (!setupAktiv && !staVerbunden && WiFi.status() == WL_CONNECTED) {
    staVerbunden = true;
    MDNS.begin(WEB_NAME);
    MDNS.addService("http", "tcp", 80);
    starteOta();
    serverLaeuft = true;
    ereignis = 1;                           // -> Ring + Piepen in main.cpp
    Serial.printf(">> WLAN verbunden! Im Browser oeffnen: http://%s  oder  http://%s.local\n",
                  WiFi.localIP().toString().c_str(), WEB_NAME);
  }
  if (staVerbunden) ArduinoOTA.handle();


  // Totmann-Schalter: kommt laenger kein Fahrbefehl, gilt der Stick als losgelassen
  if (millis() - letzterFahrbefehl > WEB_TOTMANN_MS) { x = 0; y = 0; tasten = 0; }
}

bool webEinrichtung() { return setupAktiv; }
bool webWlanDa()      { return staVerbunden && !setupAktiv && WiFi.status() == WL_CONNECTED; }

bool webVerbunden() { return (serverLaeuft || setupAktiv) && letzteAbfrage != 0 && millis() - letzteAbfrage < WEB_GETRENNT_MS; }
int  webEreignis()  { int e = ereignis; ereignis = 0; return e; }
bool webFaehrt()    { return (serverLaeuft || setupAktiv) && millis() - letzterFahrbefehl < WEB_TOTMANN_MS && (x != 0 || y != 0); }
void webSetzeModus(int m) { modus = m; }
int  webTasten()     { return (serverLaeuft || setupAktiv) ? tasten : 0; }
int  webAktion()     { int a = aktion; aktion = 0; return a; }
int  webX()         { return x; }
int  webY()         { return y; }
