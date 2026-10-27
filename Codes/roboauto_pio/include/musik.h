#pragma once
#include <Arduino.h>

// ============================================================
//  Jukebox: Chiptune-Stuecke ueber den Buzzer, mit Lichtshow auf dem LED-Ring.
//  Nur gemeinfreie Melodien (Volkslied / Klassik) und eigene Kompositionen.
// ============================================================
int   musikAnzahl();
void  musikStarten(int nr);     // 0..musikAnzahl()-1 ; laeuft schon dasselbe Stueck -> stoppen
void  musikStopp();
int   musikNummer();            // -1 = nichts laeuft
int   musikFortschritt();       // 0..100 %
int   musikTon();               // aktuelle Frequenz in Hz (0 = Pause)
int   musikNotenNummer();
float musikViertel();           // Position im Lied in Viertelnoten (fuer Disco-Lichter im Takt)       // zaehlt jede neue Note hoch (fuer die Lichtshow)
void  musikUpdate();            // in jedem loop-Durchlauf aufrufen
