#pragma once

// LED-Ring vorbereiten (einmal in setup aufrufen)
void ringStarten();

// Startanimation beim Einschalten, p = Fortschritt 0..1 (wird vom Startklang aufgerufen)
void ringStartanimation(float p);

// Zeigt den Verbindungsstatus an (in jedem loop-Durchlauf aufrufen):
// Wer steuert gerade?
enum RingQuelle { RING_NIEMAND = 0, RING_APP = 1, RING_WEB = 2, RING_AUTOWLAN = 3 };

//  - niemand: dunkelblau atmend mit langsam kreisendem Lichtpunkt (Bereitschaft)
//  - App per Bluetooth: drehender Regenbogen
//  - Webseite / App ueber den Router (Heim-WLAN): tuerkiser Komet, der im Kreis laeuft
//  - Webseite / App direkt im WLAN "Roboauto" des Autos: gruener Komet
//  - tankanzeige=true (Tank-Knopf gehalten): Akkustand statt Regenbogen
//  - modus (wenn verbunden): Polizei = Blaulicht, Team = Farbspiel in Teamfarbe,
//    Normal = Regenbogen (Bluetooth) bzw. Komet (WLAN). Beim Verbinden zeigt ringVerbunden()
//    die Farbe kurz in jedem Modus.
void ringStatus(RingQuelle quelle, bool tankanzeige, int akkuWert, int modus);

// Kurzes Signal ueber die normale Anzeige legen (ca. 1,2 s):
// LEDs fuellen sich nacheinander in der Farbe, kurz halten, dann zurueck zur Statusanzeige
void ringSignal(uint32_t farbe);

// Signal beim Verbinden in der Farbe der Verbindungsart (Bluetooth = Regenbogen, Router = tuerkis, Auto-WLAN = gruen)
void ringVerbunden(RingQuelle quelle);

// Fortschrittsbalken (0..100 %) sofort anzeigen - fuer das WLAN-Update
void ringFortschritt(int prozent, uint32_t farbe);

// Einrichtungsmodus: statt blauem Lauflicht ein orange "atmender" Ring (wartet auf WLAN-Daten)
void ringEinrichtung(bool an);

// Alarm (Aufprall/umgekippt): Ring blinkt schnell rot, hat Vorrang vor allem anderen
void ringAlarm(bool an);

// Aufnahme laeuft (1) = roter Punkt atmet, Abspielen (2) = gruener Komet kreist, 0 = aus
void ringAufnahme(int zustand);

// Kalibrierung: weisser Ring, der sich im Uhrzeigersinn fuellt (fortschritt 0..1, <0 = aus)
void ringKalibrierung(float fortschritt);

// Musik-Lichtshow: hz = aktueller Ton (0 = Pause), note = Notenzaehler, an = Musik laeuft
void ringMusik(bool an, int hz, int note);

// Aktuelle Farbe einer Ring-LED (fuer das Live-Auto auf der Webseite)
uint32_t ringAnzeigeFarbe(int i);
