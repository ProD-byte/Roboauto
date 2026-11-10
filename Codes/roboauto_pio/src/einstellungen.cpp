#include <Arduino.h>
#include <Preferences.h>
#include "config.h"
#include "einstellungen.h"
#include "web.h"

static Einstellungen e;

// Motor-Verdrahtung hat ein eigenes "Standard" - sie haengt am Auto, nicht am Geschmack
static void motorStandardwerte() {
  e.motorLInv    = MOTOR_L_INVERTIERT;
  e.motorRInv    = MOTOR_R_INVERTIERT;
  e.motorTausch  = false;
  e.kamDreh      = KAMERA_DREHUNG;        // nur der XIAO hat eine Kamera
}

static void standardwerte() {
  e.tempo        = MAX_TEMPO;
  e.lenkung      = MAX_LENKUNG;
  e.lenkungStand = MAX_LENKUNG_STAND;
  e.geradeaus    = GERADEAUS_AKTIV;
  e.crash        = CRASH_AKTIV;
  e.crashG       = CRASH_STOSS_G;
  e.ringHell     = LED_RING_HELLIGKEIT;
  e.lichtHell    = LICHT_HELLIGKEIT;
  e.toene        = TOENE_ALLE;
  e.figurGroesse = 100;
  e.lautstaerke  = 100;
  e.autoUpdate   = true;
  e.disco        = true;
}

const Einstellungen& einst() { return e; }

void einstellungenLaden() {
  standardwerte();
  motorStandardwerte();
  Preferences p;
  p.begin("einstellung", true);
  e.tempo        = p.getInt("tempo", e.tempo);
  e.lenkung      = p.getInt("lenk", e.lenkung);
  e.lenkungStand = p.getInt("lenkStand", e.lenkungStand);
  e.geradeaus    = p.getBool("gerade", e.geradeaus);
  e.crash        = p.getBool("crash", e.crash);
  e.crashG       = p.getFloat("crashG", e.crashG);
  e.ringHell     = p.getInt("ringHell", e.ringHell);
  e.lichtHell    = p.getInt("lichtHell", e.lichtHell);
  e.toene        = p.getInt("toene", e.toene);
  e.figurGroesse = p.getInt("figur", e.figurGroesse);
  e.motorLInv    = p.getBool("motL", e.motorLInv);
  e.motorRInv    = p.getBool("motR", e.motorRInv);
  e.motorTausch  = p.getBool("motTausch", e.motorTausch);
  e.lautstaerke  = p.getInt("laut", e.lautstaerke);
  e.autoUpdate   = p.getBool("autoUpd", e.autoUpdate);
  e.disco        = p.getBool("disco", e.disco);
  e.kamDreh      = p.getInt("kamDreh", e.kamDreh);
  p.getString("name", e.name, sizeof(e.name));   // fehlt -> bleibt leer
  p.end();
}

void einstellungenSpeichern() {
  Preferences p;
  p.begin("einstellung", false);
  p.putInt("tempo", e.tempo);
  p.putInt("lenk", e.lenkung);
  p.putInt("lenkStand", e.lenkungStand);
  p.putBool("gerade", e.geradeaus);
  p.putBool("crash", e.crash);
  p.putFloat("crashG", e.crashG);
  p.putInt("ringHell", e.ringHell);
  p.putInt("lichtHell", e.lichtHell);
  p.putInt("toene", e.toene);
  p.putInt("figur", e.figurGroesse);
  p.putBool("motL", e.motorLInv);
  p.putBool("motR", e.motorRInv);
  p.putBool("motTausch", e.motorTausch);
  p.putInt("laut", e.lautstaerke);
  p.putBool("autoUpd", e.autoUpdate);
  p.putBool("disco", e.disco);
  p.putInt("kamDreh", e.kamDreh);
  p.putString("name", e.name);
  p.end();
}

void einstellungenStandard() {
  standardwerte();
  einstellungenSpeichern();
  ringHelligkeit(e.ringHell);
  lichtHelligkeit(e.lichtHell);
  Serial.println(">> Einstellungen auf Standard zurueckgesetzt (Motoren bleiben)");
}

void einstellungenMotorStandard() {
  motorStandardwerte();
  einstellungenSpeichern();
  Serial.println(">> Motoren auf Standard zurueckgesetzt");
}

// Name fuer Bluetooth und WLAN: nur Buchstaben, Ziffern, Leerzeichen, - und _ (max. EIGENNAME_MAX Zeichen)
static void setzeName(const String& wert) {
  int n = 0;
  for (unsigned i = 0; i < wert.length() && n < EIGENNAME_MAX; i++) {
    char c = wert[i];
    if (isalnum((unsigned char)c) || c == '-' || c == '_' || (c == ' ' && n > 0)) e.name[n++] = c;
  }
  while (n > 0 && e.name[n - 1] == ' ') n--;     // Leerzeichen am Ende weg
  e.name[n] = 0;
}

// Jeder Wert wird auf einen sinnvollen Bereich begrenzt - kaputte Werte sind so nicht moeglich
bool einstellungSetzen(const String& name, const String& wert) {
  int   i = wert.toInt();
  float f = wert.toFloat();
  if      (name == "tempo")        e.tempo        = constrain(i, 40, 255);
  else if (name == "lenkung")      e.lenkung      = constrain(i, 20, 255);
  else if (name == "lenkungStand") e.lenkungStand = constrain(i, 40, 255);
  else if (name == "geradeaus")    e.geradeaus    = i != 0;
  else if (name == "crash")        e.crash        = i != 0;
  else if (name == "crashG")       e.crashG       = constrain(f, 1.0f, 6.0f);
  else if (name == "ringHell")     { e.ringHell  = constrain(i, 5, 255); ringHelligkeit(e.ringHell); }
  else if (name == "lichtHell")    { e.lichtHell = constrain(i, 5, 255); lichtHelligkeit(e.lichtHell); }
  else if (name == "toene")        e.toene        = constrain(i, 0, 2);
  else if (name == "figurGroesse") e.figurGroesse = constrain(i, 50, 200);
  else if (name == "motorL")       e.motorLInv    = i != 0;
  else if (name == "motorR")       e.motorRInv    = i != 0;
  else if (name == "motorTausch")  e.motorTausch  = i != 0;
  else if (name == "lautstaerke")  e.lautstaerke  = constrain(i, 5, 100);
  else if (name == "autoUpdate")   e.autoUpdate   = i != 0;
  else if (name == "disco")        e.disco        = i != 0;
  else if (name == "kamDreh")      e.kamDreh      = ((i % 360 + 360) % 360) / 90 * 90;
  else if (name == "name")         setzeName(wert);
  else return false;
  return true;
}

String einstellungenJson() {
  char json[640];
  snprintf(json, sizeof(json),
           "{\"tempo\":%d,\"lenkung\":%d,\"lenkungStand\":%d,\"geradeaus\":%d,\"crash\":%d,"
           "\"crashG\":%.1f,\"ringHell\":%d,\"lichtHell\":%d,\"toene\":%d,\"figurGroesse\":%d,"
           "\"motorL\":%d,\"motorR\":%d,\"motorTausch\":%d,\"lautstaerke\":%d,\"autoUpdate\":%d,\"disco\":%d,\"kamDreh\":%d,"
           "\"name\":\"%s\",\"autoName\":\"%s\"}",
           e.tempo, e.lenkung, e.lenkungStand, e.geradeaus, e.crash,
           e.crashG, e.ringHell, e.lichtHell, e.toene, e.figurGroesse,
           e.motorLInv, e.motorRInv, e.motorTausch, e.lautstaerke, e.autoUpdate, e.disco, e.kamDreh,
           e.name, autoName());
  return String(json);
}
