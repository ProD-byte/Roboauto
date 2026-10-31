#pragma once
#include "config.h"

// Buzzer vorbereiten (einmal in setup aufrufen)
void buzzerStarten();

// Startklang beim Einschalten (blockiert ca. 0,6 s). Ruft fuer jede Zwischenstufe
// beiJedemSchritt(0..1) auf, damit der Ring passend mitlaufen kann.
void buzzerStartklang(void (*beiJedemSchritt)(float));

// Klaenge (laufen im Hintergrund, blockieren nichts) - Stil: gleitende "Hightech"-Toene
void buzzerVerbunden();   // zwei aufsteigende Chirps
void buzzerGetrennt();    // absteigender Gleitton
void buzzerWlanOk();      // drei schnelle, steigende Blips
void buzzerWlanFehler();  // tiefer, fallender Ton
void buzzerAlarm();       // schnelle Auf-Ab-Sweeps (Aufprall/Umkippen)
void buzzerWarnung();     // zwei tiefe Blips (gesperrt / nicht moeglich)
void buzzerTick();        // ganz kurzer Klick als Bestaetigung
void buzzerEin();         // kurzer Chirp nach oben  (Funktion an)
void buzzerAus();
void buzzerProbe();       // kurzer Ton zum Testen der Lautstaerke (auch bei "nur wichtige")
void buzzerPiepsen(int hz, int ms);   // blockierend - nur fuer Sonderfaelle (WLAN-Update)         // kurzer Chirp nach unten (Funktion aus)

// Hupe: true solange die Taste gedrueckt ist (Zwei-Ton-Hupe, hat Vorrang vor Klaengen)
void buzzerHupe(bool an, int frequenz = HUPE_FREQUENZ);

// Polizei-Sirene: true solange die Taste gedrueckt ist
void buzzerSirene(bool an);

// Musik (Jukebox): Frequenz der aktuellen Note, 0 = Pause, -1 = Musik aus.
// Klaenge, Hupe und Sirene haben Vorrang, danach laeuft die Musik weiter.
void buzzerMusik(int hz);

// In jedem loop-Durchlauf aufrufen
void buzzerUpdate();
