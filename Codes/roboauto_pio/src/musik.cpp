#include <Arduino.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "musik.h"
#include "buzzer.h"

// ------------------------------------------------------------
//  Notenschrift (kleine eigene Sprache, gut lesbar):
//    e6:4    Note e in Oktave 6, Viertelnote      (#/b fuer Halbtoene: c#6, bb5)
//    a5:8.   punktierte Achtel
//    p:8     Pause
//    [a5c6e6]:4   Akkord - wird als schnelles Arpeggio gespielt (typischer Chiptune-Klang)
//    T150    Tempo in Schlaegen pro Minute (Viertel)
// ------------------------------------------------------------
// Weisse Taube (ProDMusicKI) - Chiptune-Fassung, aus der MP3 herausgehoert (172 BPM, cis-Moll / E-Dur)
#define TAUBE_INTRO "g#4:8 [g#5b5d#6]:4. g#4:8 [g#5b5d#6]:4. e4:8 [e5g#5b5]:4. e4:8 [e5g#5b5]:4. " \
                    "b4:8 [f#5b5d#6]:4. b4:8 [f#5b5d#6]:4. c#5:8 [e5g#5c#6]:4. c#5:8 [e5g#5c#6]:4. "
#define TAUBE_HOOK  "c#6:4 d#6:2. d#6:8 c#6:8 d#6:4. c#6:8 g#5:4 d#6:8 e6:8 d#6:8 b5:8 c#6:4 d#6:4 p:8 b5:8 g#5:2. " \
                    "e5:4 g#5:8 b5:8 g#5:2 g#5:4 e6:8 d#6:8 b5:2 e6:4 p:8 f#6:8 d#6:2 " \
                    "d#6:8 f#5:8 d#6:8 f#5:8 e6:8 b5:8 f#5:8 d#6:8 b5:8 f#6:8 d#6:8 b5:8 p:8 d#6:8 e6:4 "
#define TAUBE_REFRAIN "g#5:8 d#6:4 c#6:8 g#5:2 c#6:2 c#7:4 c#6:4 p:8 c#6:4. b5:8 e6:4. e6:4. d#6:4. d#6:4 " \
                      "d#6:4 p:4 e6:4 p:4 p:2 a5:4 a5:4 p:8 c#6:8 c#6:8 b5:4 b5:8 b5:8 e6:8 e6:2 d#6:2 " \
                      "d#6:4. b5:8 d#6:8 b5:4. b5:8 f#5:8 b5:4 c#6:2 d#6:8 e6:8 d#6:8 c#6:2 p:8 "
#define TAUBE_GROOVE "c#5:16 c#6:16 [c#6e6g#6]:8 c#5:16 c#6:16 [c#6e6g#6]:8 c#5:16 g#5:16 c#6:16 e6:16 g#6:16 e6:16 c#6:16 g#5:16 " \
                     "a4:16 a5:16 [a5c#6e6]:8 a4:16 a5:16 [a5c#6e6]:8 a4:16 e5:16 a5:16 c#6:16 e6:16 c#6:16 a5:16 e5:16 " \
                     "e5:16 e6:16 [e6g#6b6]:8 e5:16 e6:16 [e6g#6b6]:8 e5:16 b5:16 e6:16 g#6:16 b6:16 g#6:16 e6:16 b5:16 " \
                     "b4:16 b5:16 [b5d#6f#6]:8 b4:16 b5:16 [b5d#6f#6]:8 b4:16 f#5:16 b5:16 d#6:16 f#6:16 d#6:16 b5:16 f#5:16 "
static const char LIED_TAUBE[] =
  "T172 " TAUBE_INTRO TAUBE_HOOK TAUBE_INTRO TAUBE_REFRAIN TAUBE_GROOVE
  "T176 " TAUBE_HOOK TAUBE_REFRAIN TAUBE_GROOVE
  "c#6:4 d#6:2. [g#5b5d#6g#6]:1";


// ---------- Freie Chiptunes von OpenGameArt.org (Lizenz CC0 = gemeinfrei) ----------
// Aus den MIDI-Dateien umgerechnet: Melodie + Bass in den Pausen, lange Toene als Akkord-Arpeggio.
// Quellen: opengameart.org/content/15-melodic-rpg-chiptunes, /theme-song-8-bit, /leap-8bit
// RPG-Titelthema (Aureolus Omicron, CC0)
static const char LIED_RPG_TITEL[] =
  "T180 e5:16 p:8. e5:16 p:8. e5:16 p:8. e5:16 p:16 e5:16 p:16 a5:16 p:8. a5:16 p:8. a5:16 p:8. a5:16 p:16 e6:8 "
  "f5:16 [c6e6f6]:16 c5:16 [c6e6f6]:16 f5:16 [c6e6f6]:16 c5:16 [c6e6f6]:16 f5:16 [c6e6f6]:16 c5:16 [c6e6f6]:16 "
  "f5:16 [c6e6f6]:16 a6:8 f5:16 [f6g6b6]:16 f5:16 [f6g6b6]:16 f5:16 [f6g6b6]:16 f5:16 [f6g6b6]:16 f5:16 "
  "[f6g6b6]:16 f5:16 [f6g6b6]:16 f5:16 [f6g6b6]:16 g6:8 e5:16 [g6d7e7]:16 g5:16 [g6d7e7]:16 e5:16 [g6d7e7]:16 "
  "g5:16 [g6d7e7]:16 e5:16 [g6d7e7]:16 g5:16 [g6d7e7]:16 e5:16 [g6d7e7]:16 e7:16 d#7:16 c#7:16 c7:8. a5:16 "
  "[e6g6a6]:16 e5:16 [e6g6a6]:16 a5:16 [g6a6b6]:16 g5:16 [g6a6b6]:16 a5:16 [g6a6c7]:16 g5:16 [g6a6c7]:16 f5:16 "
  "[e6f6a6]:16 e5:16 [e6f6a6]:16 f5:16 [c6e6f6]:16 c5:16 [c6e6f6]:16 f5:16 [e6f6g6]:16 e5:16 [e6f6g6]:16 f5:16 "
  "[e6f6g6]:16 a6:8 g6:8 d6:8 f5:16 [f5g5b5]:16 f4:16 [f5g5b5]:16 f5:16 [b5d6e6]:16 b4:16 [b5d6e6]:16 f5:16 "
  "[f6g6a6]:16 f5:16 [f6g6a6]:16 e5:16 [d6e6g6]:16 d5:16 [d6e6g6]:16 e5:16 [d6e6g6]:16 d5:16 [d6e6g6]:16 e5:16 "
  "[d6e6g6]:16 a6:8 g6:8 f#6:8 a5:16 [c6e6g6]:16 c5:16 [c6e6g6]:16 a5:16 [c6e6g6]:16 c5:16 [c6e6g6]:16 a5:16 "
  "[c6e6g6]:16 c5:16 [c6e6g6]:16 a5:16 [c6e6g6]:16 e6:8 f5:16 [c6e6f6]:16 c5:16 [c6e6f6]:16 f5:16 [c6e6f6]:16 "
  "c5:16 [c6e6f6]:16 f5:16 [c6e6f6]:16 c5:16 [c6e6f6]:16 f5:16 [c6e6f6]:16 a6:8 f5:16 [f6g6b6]:16 f5:16 "
  "[f6g6b6]:16 f5:16 [f6g6b6]:16 f5:16 [f6g6b6]:16 f5:16 [f6g6b6]:16 f5:16 [f6g6b6]:16 f5:16 [f6g6b6]:16 g6:8 "
  "e5:16 [g6d7e7]:16 g5:16 [g6d7e7]:16 e5:16 [g6d7e7]:16 g5:16 [g6d7e7]:16 e5:16 [g6d7e7]:16 g5:16 [g6d7e7]:16 "
  "e5:16 [g6d7e7]:16 e7:16 d#7:16 c#7:16 c7:8. a5:16 [e6g6a6]:16 e5:16 [e6g6a6]:16 a5:16 [g6a6b6]:16 g5:16 "
  "[g6a6b6]:16 a5:16 [g6a6c7]:16 g5:16 [g6a6c7]:16 f5:16 [e6f6a6]:16 e5:16 [e6f6a6]:16 f5:16 [c6e6f6]:16 c5:16 "
  "[c6e6f6]:16 f5:16 [e6f6g6]:16 e5:16 [e6f6g6]:16 f5:16 [e6f6g6]:16 a6:8 g6:8 d6:8 f5:16 [f5g5b5]:16 f4:16 "
  "[f5g5b5]:16 f5:16 [b5d6e6]:16 b4:16 [b5d6e6]:16 f5:16 [f6g6a6]:16 f5:16 [f6g6a6]:16 e5:16 [d6e6g6]:16 d5:16 "
  "[d6e6g6]:16 e5:16 [d6e6g6]:16 d5:16 [d6e6g6]:16 e5:16 [d6e6g6]:16 a6:8 g6:8 f#6:8 a5:16 [c6e6g6]:16 c5:16 "
  "[c6e6g6]:16 a5:16 [c6e6g6]:16 c5:16 [c6e6g6]:16 a5:16 [c6e6g6]:16 c5:16 [c6e6g6]:16 a5:16 [c6e6g6]:16 g6:8 "
  "d5:16 [c6d6f6]:16 c5:16 [c6d6f6]:16 d5:16 [c6d6f6]:16 c5:16 [c6d6f6]:16 d5:16 [c6d6f6]:16 f6:8 g6:8 a6:8 "
  "g5:16 [f6g6b6]:16 f5:16 [f6g6b6]:16 g5:16 [f6g6b6]:16 f5:16 [f6g6b6]:16 g5:16 [g6b6d7]:16 g5:16 [g6b6d7]:16 "
  "g5:16 [g6b6d7]:16 g5:16 [g6b6d7]:16 c5:16 [g6b6c7]:16 g5:16 [g6b6c7]:16 c5:16 [g6b6c7]:16 g5:16 [g6b6c7]:16 "
  "c5:16 [g6b6c7]:16 d7:8 c7:8 b6:8 f5:16 [e6f6a6]:16 e5:16 [e6f6a6]:16 f5:16 [e6f6a6]:16 e5:16 [e6f6a6]:16 "
  "f5:16 [e6f6a6]:16 b6:8 a6:8 g6:8 d5:16 [c6d6f6]:16 c5:16 [c6d6f6]:16 d5:16 [c6d6f6]:16 c5:16 [c6d6f6]:16 "
  "d5:16 [c6d6f6]:16 f6:8 g6:8 a6:8 g5:16 [f6g6b6]:16 f5:16 [f6g6b6]:16 g5:16 [f6g6b6]:16 f5:16 [f6g6b6]:16 "
  "g5:16 [g6b6d7]:16 g5:16 [g6b6d7]:16 g5:16 [g6b6d7]:16 g5:16 [g6b6d7]:16 c5:16 [g6b6c7]:16 g5:16 [g6b6c7]:16 "
  "c5:16 [g6b6c7]:16 g5:16 [g6b6c7]:16 c5:16 [g6b6c7]:16 d7:8 c7:8 b6:8 f5:16 [e6f6a6]:16 e5:16 [e6f6a6]:16 "
  "f5:16 [e6f6a6]:16 e5:16 [e6f6a6]:16 f5:16 [e6f6a6]:16 a6:8 b6:8 c7:8";

// Luftschiff / Airship (Aureolus Omicron, CC0)
static const char LIED_LUFTSCHIFF[] =
  "T126 g#5:16 [d#5g#5c6]:16 g#5:16 [d#5g#5c6]:16 g#5:16 [d#5g#5c6]:16 g#5:16 [d#5g#5c6]:16 g#5:16 [g#5d6]:16 "
  "g#5:16 [g#5d6]:16 g#5:16 [g#5d6]:16 g#5:16 [g#5d6]:16 g#5:16 [g#5c6d#6]:16 g#5:16 [g#5c6d#6]:16 g#5:16 "
  "[g#5c6d#6]:16 g#5:16 [g#5c6d#6]:16 g#5:16 [g#5b5f6]:16 g#5:16 [g#5b5f6]:16 g#5:16 [g#5b5f6]:16 g#5:16 "
  "[g#5b5f6]:16 g#5:16 [g#5a#5f#6]:16 g#5:16 [g#5a#5f#6]:16 g#5:16 [g#5a#5f#6]:16 g#5:16 [g#5a#5f#6]:16 g#5:16 "
  "[g#5f6]:16 g#5:16 [g#5f6]:16 g#5:16 [g#5f6]:16 g#5:16 [g#5f6]:16 g#5:16 [g#5c6d#6]:16 g#5:16 [g#5c6d#6]:16 "
  "g#5:16 [g#5c6d#6]:16 g#5:16 [g#5c6d#6]:16 g#5:16 [g#5d6f6]:16 g#5:16 [g#5d6f6]:16 g#5:16 [g#5d6f6]:16 g#5:16 "
  "[g#5d6f6]:16 c#6:16 [c#6f6]:16 c#6:16 [c#6f6]:16 c#6:16 [c#6f6]:16 c#6:16 [c#6f6]:16 c#6:16 [c#6d#6f#6]:16 "
  "c#6:16 [c#6d#6f#6]:16 c#6:16 [c#6d#6f#6]:16 c#6:16 [c#6d#6f#6]:16 c#6:16 [c#6f6g#6]:16 c#6:16 [c#6f6g#6]:16 "
  "c#6:16 [c#6f6g#6]:16 c#6:16 [c#6f6g#6]:16 c#6:16 [c#6d#6a#6]:16 c#6:16 [c#6d#6a#6]:16 c#6:16 [c#6d#6a#6]:16 "
  "c#6:16 [c#6d#6a#6]:16 d#6:16 [d6d#6g6]:16 d#6:16 [d6d#6g6]:16 d#6:16 [d6d#6g6]:16 d#6:16 [d6d#6g6]:16 d#6:16 "
  "[d6d#6g6]:16 d#6:16 [d6d#6g6]:16 d#6:16 [d6d#6g6]:16 d#6:16 [d6d#6g6]:16 d6:16 [d6d#6g6]:16 d6:16 "
  "[d6d#6g6]:16 d6:16 [d6d#6g6]:16 d6:16 [d6d#6g6]:16 a#5:16 [d6d#6g6]:16 f#6:16 f6:16 e6:16 d#6:16 d6:16 c#6:16 "
  "g#5:16 [d#5g#5c6]:16 g#5:16 [d#5g#5c6]:16 g#5:16 [d#5g#5c6]:16 g#5:16 [d#5g#5c6]:16 g#5:16 [g#5d6]:16 g#5:16 "
  "[g#5d6]:16 g#5:16 [g#5d6]:16 g#5:16 [g#5d6]:16 g#5:16 [g#5c6d#6]:16 g#5:16 [g#5c6d#6]:16 g#5:16 [g#5c6d#6]:16 "
  "g#5:16 [g#5c6d#6]:16 g#5:16 [g#5b5f6]:16 g#5:16 [g#5b5f6]:16 g#5:16 [g#5b5f6]:16 g#5:16 [g#5b5f6]:16 g#5:16 "
  "[g#5a#5f#6]:16 g#5:16 [g#5a#5f#6]:16 g#5:16 [g#5a#5f#6]:16 g#5:16 [g#5a#5f#6]:16 g#5:16 [g#5f6]:16 g#5:16 "
  "[g#5f6]:16 g#5:16 [g#5f6]:16 g#5:16 [g#5f6]:16 g#5:16 [g#5c6d#6]:16 g#5:16 [g#5c6d#6]:16 g#5:16 [g#5c6d#6]:16 "
  "g#5:16 [g#5c6d#6]:16 g#5:16 [g#5d6f6]:16 g#5:16 [g#5d6f6]:16 g#5:16 [g#5d6f6]:16 g#5:16 [g#5d6f6]:16 c#6:16 "
  "[c#6f6]:16 c#6:16 [c#6f6]:16 c#6:16 [c#6f6]:16 c#6:16 [c#6f6]:16 c#6:16 [c#6d#6f#6]:16 c#6:16 [c#6d#6f#6]:16 "
  "c#6:16 [c#6d#6f#6]:16 c#6:16 [c#6d#6f#6]:16 c#6:16 [c#6f6g#6]:16 c#6:16 [c#6f6g#6]:16 c#6:16 [c#6f6g#6]:16 "
  "c#6:16 [c#6f6g#6]:16 c#6:16 [c#6d#6a#6]:16 c#6:16 [c#6d#6a#6]:16 c#6:16 [c#6f6c7]:16 c#6:16 [c#6f6c7]:16 "
  "d#6:16 [d#6g6d7]:16 d#6:16 [d#6g6d7]:16 d#6:16 [d#6g6d7]:16 d#6:16 [d#6g6d7]:16 d#6:16 [d#6g6d7]:16 d#6:16 "
  "[d#6g6d7]:16 d#6:16 [d#6g6d7]:16 d#6:16 [d#6g6d7]:16 d6:16 [d#6g6d7]:16 d6:16 [d#6g6d7]:16 d6:16 [d#6g6d7]:16 "
  "d6:16 [d#6g6d7]:16 a#5:16 [d#6g6d7]:16 d6:16 [d#6g6d7]:16 g6:16 [d#6g6d7]:16 d6:16 a#5:16 c7:8 a#6:8 g#6:8 "
  "a#6:8 c7:8 a#6:8 g#6:8 a#6:8 c7:8 a#6:8 g#6:8 a#6:8 c7:8 a#6:8 g#6:8 a#6:8 a#6:8 g#6:8 f#6:8 g#6:8 a#6:8 "
  "g#6:8 f#6:8 g#6:8 a#6:8 g#6:8 f#6:8 g#6:8 a#6:8 g#6:8 f#6:8 g#6:8 g#6:8 f#6:8 e6:8 f#6:8 g#6:8 f#6:8 e6:8 "
  "f#6:8 f#6:8 e6:8 d6:8 e6:8 f#6:8 e6:8 d6:8 e6:8 e6:8 d6:8 c6:8 d6:8 e6:8 d6:8 c6:8 d6:8 d6:8 c6:8 a#5:8 c6:8 "
  "d6:8 c6:8 d#5:16 [d#5g5a#5]:16 d#5:16 [d#5g5a#5]:16";

// Kampf 2 / Battle 2 (Aureolus Omicron, CC0)
static const char LIED_ENDKAMPF[] =
  "T144 d#5:16 [d#6f#6]:16 d#5:16 [d#6f#6]:16 d#6:16 [d#6f#6]:16 d#5:16 [d#6f#6]:16 d#5:16 [d#6f#6]:16 d#6:16 "
  "[d#6f#6]:16 e6:16 [a5e6g6]:16 a4:16 [a5e6g6]:16 d#5:16 [d#6f#6]:16 d#5:16 [d#6f#6]:16 d#6:16 [d#6f#6]:16 "
  "d#5:16 [d#6f#6]:16 d#5:16 [d#6f#6]:16 d#6:16 [d#6f#6]:16 e6:16 [a5e6g6]:16 a4:16 [a5e6g6]:16 g#5:16 "
  "[g#6b6]:16 g#5:16 [g#6b6]:16 g#5:16 [g#6b6]:16 g#5:16 [g#6b6]:16 g#5:16 [g#6b6]:16 g#5:16 [g#6b6]:16 a5:16 "
  "[d6a6c7]:16 d5:16 [d6a6c7]:16 g#5:16 [g#6b6]:16 g#5:16 [g#6b6]:16 g#5:16 [g#6b6]:16 g#5:16 [g#6b6]:16 g#5:16 "
  "[g#6b6]:16 g#5:16 [g#6b6]:16 a5:16 [g#6b6]:16 a5:16 [g#6b6]:16 a#4:16 [a#5c#6]:16 a#4:16 [a#5c#6]:16 a#5:16 "
  "[a#5c#6]:16 a#4:16 [a#5c#6]:16 a#4:16 [a#5c#6]:16 a#5:16 [a#5c#6]:16 b5:16 [e5b5d6]:16 e4:16 [e5b5d6]:16 "
  "a#4:16 [a#5c#6]:16 a#4:16 [a#5c#6]:16 a#5:16 [f5a#5d#6]:16 f4:16 [f5a#5d#6]:16 a#4:16 [a#5f6]:16 a#5:16 "
  "[a#5f6]:16 b5:16 [b5e6g6]:16 b5:16 [b5e6g6]:16 e5:16 [e6g6]:16 b5:16 [e6g6]:16 c6:16 [e6g6]:16 e5:16 "
  "[e6g6]:16 d6:16 [e6g6]:16 c6:16 [e6g6]:16 e5:16 [e6g6]:16 b5:16 [e6g6]:16 d5:16 [d6f6]:16 a5:16 [d6f6]:16 "
  "a#5:16 [d6f6]:16 d5:16 [d6f6]:16 c6:16 [d6f6]:16 a#5:16 [d6f6]:16 d5:16 [d6f6]:16 a5:16 [d6f6]:16 c5:16 "
  "[c6d#6]:16 g5:16 [c6d#6]:16 g#5:16 [c6d#6]:16 c5:16 [c6d#6]:16 a#5:16 [c6d#6]:16 g#5:16 [c6d#6]:16 c5:16 "
  "[c6d#6]:16 g5:16 [c6d#6]:16 a#4:16 [a#5c#6]:16 f5:16 [a#5c#6]:16 f#5:16 [a#5c#6]:16 a#4:16 [a#5c#6]:16 g#5:16 "
  "[g#5a#5d#6]:16 a#5:16 [g#5a#5d#6]:16 a#4:16 [a#5f6]:16 f5:16 [a#5f6]:16 e5:16 [e6g6]:16 b5:16 [e6g6]:16 c6:16 "
  "[e6g6]:16 e5:16 [e6g6]:16 d6:16 [e6g6]:16 c6:16 [e6g6]:16 e5:16 [e6g6]:16 g5:16 g6:16 a4:16 [a5d6f6]:16 a5:16 "
  "[a5d6f6]:16 a#5:16 [a5d6f6]:16 a4:16 [a5d6f6]:16 c6:16 [a5d6f6]:16 a#5:16 [a5d6f6]:16 a4:16 [a5d6f6]:16 a5:16 "
  "a#5:16 c5:16 [c6d#6]:16 g5:16 [c6d#6]:16 g#5:16 [c6d#6]:16 c5:16 [c6d#6]:16 a#5:16 [c6d#6]:16 g#5:16 "
  "[c6d#6]:16 c5:16 [c6d#6]:16 c6:16 d#6:16 a#4:16 [a#5c#6]:16 f5:16 [a#5c#6]:16 f#5:16 [a#5c#6]:16 a#4:16 "
  "[a#5c#6]:16 g#5:16 [a#5c#6d#6]:16 a#5:16 [a#5c#6d#6]:16 f6:16 c6:16 f6:16 b5:16 [d#6f#6a#6d#7]:2";

// Stadt / Town (Aureolus Omicron, CC0)
static const char LIED_STADT[] =
  "T108 a5:16 p:8. a5:8 b5:8 c#6:8 p:8 e6:8 p:8 [e5g#5b5]:4 p:4 [g#5b5e6]:4 p:4 f#5:16 p:8. a5:8 b5:8 c#6:8 p:8 "
  "f#6:8 p:8 f#6:8 p:8 e6:8 p:8 [f#5a#5c#6]:4 p:4 b5:16 p:8. b5:8 c#6:8 d6:8 p:8 f#6:8 p:8 f#6:8 p:8 d#6:8 p:8 "
  "b5:8 p:8 f#6:8 p:8 e5:16 p:8. e6:8 d6:8 c#6:8 p:8 b5:8 p:8 [e5g#5d6]:4 p:4 [e5g#5b5]:4 p:4 a5:16 p:8. c#6:8 "
  "d6:8 e6:8 p:8 a6:8 p:8 [e6g#6b6]:4 p:4 [b5e6g#6]:4 p:4 f#5:16 p:8. f#6:8 g#6:8 a6:8 p:8 c#7:8 p:8 a#6:8 p:8 "
  "b6:8 p:8 [f#6a#6c#7]:4 p:4 b5:16 p:8. d6:8 e6:8 f#6:8 p:8 b6:8 p:8 a6:8 p:8 f#6:8 p:8 d#6:8 p:8 a6:8 p:8 "
  "e5:16 p:8. g6:8 f#6:8 e6:8 p:8 g6:8 a6:8 [e6g#6b6]:4 p:4 [d6e6g#6]:4 p:4 c#7:8 p:8 c#7:8 b6:8 a6:8 p:8 e6:8 "
  "p:8 [b5e6g#6]:4 p:4 e6:8 p:8 b5:8 p:8 a5:8 p:8 a5:8 b5:8 c#6:8 p:8 f#6:8 p:8 f#6:8 p:8 g#6:8 p:8 a#6:8 p:8 "
  "f#6:8 p:8 f#6:8 p:8 f#6:8 g6:8 a6:8 p:8 b6:8 p:8 b6:8 p:8 c#7:8 p:8 d#7:8 p:8 b6:8 p:8 g6:8 p:8 b6:8 a6:8 "
  "g6:8 p:8 f#6:8 p:8 e6:8 p:8 f#6:8 p:8 g#6:8 p:8 b6:8 p:8";

// Theme Song 8-bit (nene, CC0)
static const char LIED_HYMNE[] =
  "T130 a4:16 [a5d6]:16 a4:16 [a5d6]:16 a4:16 [a5d6]:16 d6:8 f6:8. e6:8. f6:8 a4:16 [a5d6]:16 a4:16 [a5d6]:16 "
  "a4:16 [a5d6]:16 d6:8 f6:8. e6:8. c6:8 a4:16 [a5d6]:16 a4:16 [a5d6]:16 a4:16 [a5d6]:16 d6:8 f6:8. e6:8. f6:8 "
  "d6:16 d6:16 d6:16 d6:16 e6:16 e6:16 f6:16 f6:16 g6:16 g6:16 g6:16 g6:16 f6:16 f6:16 e6:16 e6:16 d5:16 "
  "[a5d6]:16 d5:16 [a5d6]:16 d5:16 [a5d6]:16 d6:8 f6:8. e6:8. f6:8 d5:16 [a5d6]:16 d5:16 [a5d6]:16 a4:16 "
  "[a5d6]:16 d6:8 f6:8. e6:8. c6:8 a#5:16 [f5a#5d6]:16 a#5:16 [f5a#5d6]:16 a#5:16 [f5a#5d6]:16 d6:8 f6:8. e6:8. "
  "f6:8 d6:16 d6:16 d6:16 d6:16 e6:16 e6:16 f6:16 f6:16 g6:16 g6:16 g6:16 g6:16 f6:16 f6:16 e6:16 e6:16 d5:16 "
  "[a5d6]:16 d5:16 [a5d6]:16 d5:16 [a5d6]:16 d6:8 f6:8. e6:8. f6:8 d5:16 [a5d6]:16 d5:16 [a5d6]:16 d5:16 "
  "[a5d6]:16 d6:8 f6:8. e6:8. c6:8 d5:16 [a5d6]:16 d5:16 [a5d6]:16 d5:16 [a5d6]:16 d6:8 f6:8. e6:8. f6:8 d6:16 "
  "d6:16 d6:16 d6:16 e6:16 e6:16 f6:16 f6:16 g6:16 g6:16 g6:16 g6:16 f6:16 f6:16 e6:16 e6:16 d5:16 [a5d6]:16 "
  "d5:16 [a5d6]:16 d5:16 [a5d6]:16 d6:8 f6:8. e6:8. f6:8 d5:16 [a5d6]:16 d5:16 [a5d6]:16 d5:16 [a5d6]:16 d6:8 "
  "f6:8. e6:8. c6:8 a#5:16 [f5a#5d6]:16 a#5:16 [f5a#5d6]:16 a#5:16 [f5a#5d6]:16 d6:8 f6:8. e6:8. f6:8 d6:16 "
  "d6:16 d6:16 d6:16 e6:16 e6:16 f6:16 f6:16 g6:16 g6:16 g6:16 g6:16 f6:16 f6:16 e6:16 e6:16 d5:16 [f5a5d6]:16 "
  "d5:16 [f5a5d6]:16 d5:16 [f5a5d6]:16 d6:8 f6:8. e6:8. f6:8 d5:16 [f5a5d6]:16 d5:16 [f5a5d6]:16 d5:16 "
  "[f5a5d6]:16 d6:8 f6:8. e6:8. c6:8 d5:16 [f5a5d6]:16 d5:16 [f5a5d6]:16 d5:16 [f5a5d6]:16 d6:8 f6:8. e6:8. f6:8 "
  "d6:16 d6:16 d6:16 d6:16 e6:16 e6:16 f6:16 f6:16 g6:16 g6:16 g6:16 g6:16 f6:16 f6:16 e6:16 e6:16 [d6f6a6d7]:2";

// Leap 8-bit (nene, CC0)
static const char LIED_LEAP[] =
  "T155 a#6:8 f6:8 a#5:8 a#6:8 f6:8 a#5:8 c#7:8 c7:8 a#6:8 f6:8 a#5:8 a#6:8 f6:8 a#5:8 f6:8 a#6:8 c#7:8 c7:8 "
  "g#6:8 c#6:8 c6:8 g#5:8 g#6:8 c7:8 c#7:8 c7:8 g#5:8 d#6:8 g#6:8 c7:8 c#7:8 c7:8 a#6:8 f6:8 a#5:8 a#6:8 f6:8 "
  "a#5:8 c#7:8 c7:8 a#6:8 f6:8 a#5:8 a#6:8 f6:8 a#5:8 f6:8 a#6:8 c#7:8 c7:8 g#6:8 c#6:8 c6:8 g#5:8 g#6:8 c7:8 "
  "c#7:8 c7:8 g#5:8 d#6:8 g#6:8 c7:8 c#7:8 c7:8 a#6:8 f6:8 a#5:8 a#6:8 f6:8 a#5:8 c#7:8 c7:8 a#6:8 f6:8 a#5:8 "
  "a#6:8 f6:8 a#5:8 f6:8 a#6:8 c#7:8 c7:8 g#6:8 c#6:8 c6:8 g#5:8 g#6:8 c7:8 c#7:8 c7:8 g#5:8 d#6:8 g#6:8 c7:8 "
  "c#7:8 c7:8 a#6:8 f6:8 a#5:8 a#6:8 f6:8 a#5:8 c#7:8 c7:8 a#6:8 f6:8 a#5:8 a#6:8 f6:8 a#5:8 f6:8 a#6:8 c#7:8 "
  "c7:8 g#6:8 c#6:8 c6:8 g#5:8 g#6:8 c7:8 c#7:8 c7:8 g#5:8 d#6:8 g#6:8 c7:8 c#7:8 c7:8 [a#5d6f6a#6]:2";


// ---------- Beliebte freie Chiptunes von OpenGameArt.org (meistfavorisiert, mit MIDI) ----------
// Quellen: opengameart.org/content/jrpg-collection (CC-BY 4.0, Yubatake), /long-away-home-8bit, /boss-battle-2-8-bit, /boss-battle-5-8-bit (CC0, nene)
// JRPG-Hauptthema (Yubatake, CC-BY 4.0)
static const char LIED_JRPG_THEMA[] =
  "T100 p:2 c5:8 e5:8 g5:8 c6:8 [c5g5b5]:4 a5:8 g5:8 [a4f5]:4 [f4d5]:4 [g4c5e5]:4 p:8 e5:8 [a4f5]:4 g5:8 f5:8 "
  "[g4c5e5]:4 [a4c5]:4 [b4d5]:1 [f5a5]:4 g5:8 a5:8 [g5b5]:4 c6:8 d6:8 [e5c6]:4 [c5e5g5]:4 [a4f5]:4 p:8 f5:8 "
  "[g4c5e5]:4 f5:8 e5:8 [g4b4d5]:4 c5:8 b4:8 [e4c5]:1 [g4b4d5]:4 [d5g5]:4 g5:8 f#5:8 g5:8 a5:8 [g5b5]:4 g5:4 "
  "[a4f5g5]:4 a5:8 b5:8 [e5c6]:4. b5:8 [e5g5c6]:4 [e5g5d6]:4 [g5c6e6]:2. [a5d6e6]:4 [a#5d#6]:4 d#6:8 f6:8 "
  "[a#5g6]:4 [g#5d#6]:4 [a#5d6]:4. c6:8 [d5f5a#5]:4 [f5a#5d6]:4 [g#5c6]:4 c6:8 d6:8 [c6d#6]:4 [g#5c6]:4 "
  "[g5b5d6]:2. c6:8 d6:8 [g5c6e6]:4 [g5c6]:4 [g5b5d6]:4 [f5b5]:4 [e5a5c6]:4 b5:8 a5:8 [g5b5]:4 [d5g5]:4 [f5a5]:4 "
  "g5:8 f5:8 e5:8 g5:8 [e5g5c6]:4 [d5a5c6]:4 b5:8 c6:8 [g5b5d6]:4 c6:8 d6:8 [g5c6e6]:4 [g5c6]:4 [g5b5d6]:4 "
  "[f5b5]:4 [e5a5c6]:4 b5:8 a5:8 [e5g5b5]:4 [e5g5]:4 [c5f5a5]:4 g5:8 f5:8 e5:8 g5:8 c6:8 d6:8 [d5e5c6]:4 "
  "[d5g5b5]:4 [e5c6]:4. p:8 c5:8 e5:8 g5:8 c6:8 [c5g5b5]:4 a5:8 g5:8 [a4f5]:4 [f4d5]:4 [g4c5e5]:4 p:8 e5:8 "
  "[a4f5]:4 g5:8 f5:8 [g4c5e5]:4 [a4c5]:4 [b4d5]:1 [f5a5]:4 g5:8 a5:8 [g5b5]:4 c6:8 d6:8 [e5c6]:4 [c5e5g5]:4 "
  "[a4f5]:4 p:8 f5:8 [g4c5e5]:4 f5:8 e5:8 [g4b4d5]:4 c5:8 b4:8 [e4c5]:2. c6:8 d6:8 [g5c6e6]:4 [g5c6]:4 [g5b5d6]:4 "
  "[f5b5]:4 [e5a5c6]:4 b5:8 a5:8 [g5b5]:4 [d5g5]:4 [f5a5]:4 g5:8 f5:8 e5:8 g5:8 [e5g5c6]:4 [d5a5c6]:4 b5:8 c6:8 "
  "[g5b5d6]:4 c6:8 d6:8 [g5c6e6]:4 [g5c6]:4 [g5b5d6]:4 [f5b5]:4 [e5a5c6]:4 b5:8 a5:8 [e5g5b5]:4 [e5g5]:4 "
  "[c5f5a5]:4 g5:8 f5:8 e5:8 g5:8 c6:8 d6:8 [d5e5c6]:4 [d5g5b5]:4 [g5c6]:4 c6:8 d6:8 [g5c6e6]:4 [g5c6]:4 "
  "[g5c6d6]:4 [c5f5b5]:4 [e5c6]:4 b5:8 a5:8 [c5e5b5]:4 [c5e5g5]:4 [c5a5]:4 g5:8 f5:8 e5:8 g5:8 [e5c6]:4 [f5c6]:4 "
  "b5:8 c6:8 [g5b5d6]:4 c6:8 d6:8 [g5c6e6]:4 [g5c6]:4 d6:8 e6:8 [b5f6]:4 [a5c6e6]:4 d6:8 c6:8 [e5g5d6]:4 "
  "[e5g5b5]:4 [c5f5a5]:4 g5:8 f5:8 e5:8 g5:8 c6:8 d6:8 [d5g5c6]:4 [f5g5b5]:4 [f5c6]:4. p:8 c5:8 e5:8 g5:8 c6:8 "
  "[c5g5b5]:4 a5:8 g5:8 [a4f5]:4 [f4d5]:4 [g4c5e5]:4 p:8 e5:8 [a4f5]:4 g5:8 f5:8 [g4c5e5]:4 [a4c5]:4 [b4d5]:1 "
  "[f5a5]:4 g5:8 a5:8 [g5b5]:4 c6:8 d6:8 [e5c6]:4 [c5e5g5]:4 [a4f5]:4 p:8 f5:8 [g4c5e5]:4 f5:8 e5:8 [g4b4d5]:4 "
  "c5:8 b4:8 [a4c5]:2. p:8 f5:8 [f4c5e5]:4 f5:8 e5:8 [g4b4d5]:4 c5:8 b4:8 [e4c5]:2 p:16 [f4g#4]:2 p:8 [e4g4c5]:1";
// JRPG-Kampf (Yubatake, CC-BY 4.0)
static const char LIED_JRPG_KAMPF[] =
  "T150 g#4:16 g4:16 p:16 g#4:16 a4:16 a#4:16 p:16 b4:16 g#4:16 g4:16 p:16 g#4:16 f#4:16 f4:16 p:16 e4:16 g#4:16 "
  "g4:16 p:16 g#4:16 a4:16 a#4:16 p:16 b4:16 g#4:16 g4:16 p:16 g#4:16 f#4:16 f4:16 p:16 e4:16 g#4:16 g#4:16 p:16 "
  "g#4:16 [a4c#5d#5]:4 g#4:16 g#4:16 p:16 g#4:16 [f#4d5e5]:4 g#4:16 g#4:16 p:16 g#4:16 [a4d#5f5]:4 g#4:16 g#4:16 "
  "p:16 g#4:16 f#5:8. f5:16 g#4:16 g4:16 p:16 g#4:16 a4:16 a#4:16 p:16 b4:16 g#4:16 g4:16 p:16 g#4:16 f#4:16 "
  "f4:16 p:16 e4:16 g#4:16 g4:16 p:16 g#4:16 a4:16 a#4:16 p:16 b4:16 g#4:16 g4:16 p:16 g#4:16 f#4:16 f4:16 p:16 "
  "e4:16 g#4:16 g#4:16 p:16 g#4:16 [a4c#5d#5]:4 g#4:16 g#4:16 p:16 g#4:16 [f#4d5e5]:4 g#4:16 g#4:16 p:16 g#4:16 "
  "[a4d#5f5]:4 g#4:16 g#4:16 p:16 g#4:16 g#5:8. g5:16 g#4:16 g4:16 p:16 g#4:16 a4:16 a#4:16 p:16 b4:16 g#4:16 "
  "g4:16 p:16 g#4:16 f#4:16 f4:16 p:16 e4:16 g#4:16 g4:16 p:16 g#4:16 a4:16 a#4:16 p:16 b4:16 g#4:16 g4:16 p:16 "
  "g#4:16 g#4:16 b4:16 p:16 d#5:16 [d#5g#5]:2. p:8. e4:16 [g#4d5g5]:4. p:16 b4:16 [g#4c#5f#5]:4. p:16 f5:16 "
  "g#4:16 g4:16 p:16 g#4:16 a4:16 a#4:16 p:16 b4:16 g#4:16 g4:16 p:16 g#4:16 f#4:16 f4:16 p:16 e4:16 g#4:16 g4:16 "
  "p:16 g#4:16 a4:16 a#4:16 p:16 b4:16 g#4:16 g4:16 p:16 g#4:16 g#4:16 b4:16 p:16 d#5:16 [d#5g#5]:2. p:8. e4:16 "
  "[f5g#5a#5]:4. p:16 b4:16 [g#5c#6]:4. p:16 c6:16 g#4:16 g4:16 p:16 g#4:16 a4:16 a#4:16 p:16 b4:16 g#4:16 g4:16 "
  "p:16 g#4:16 f#4:16 f4:16 p:16 e4:16 g#4:16 g4:16 p:16 g#4:16 a4:16 a#4:16 p:16 b4:16 g#4:16 g4:16 p:16 g#4:16 "
  "f#4:16 f4:16 p:16 e4:16 g#4:16 g4:16 p:16 g#5:16 a4:16 a#4:16 p:16 g5:16 g#4:16 g4:16 p:16 f#5:16 f#4:16 f4:16 "
  "p:16 f5:16 g#4:16 g4:16 p:16 e5:16 a4:16 a#4:16 p:16 d#5:16 g#4:16 g4:16 p:16 d5:16 c#5:16 d5:16 p:16 d#5:16 "
  "g#4:16 g4:16 p:16 g#5:16 a4:16 a#4:16 p:16 g5:16 g#4:16 g4:16 p:16 f#5:16 f#4:16 f4:16 p:16 f5:16 g#4:16 g4:16 "
  "p:16 e5:16 a4:16 a#4:16 p:16 d#5:16 g#4:16 f#4:16 p:16 d5:16 c#5:16 d5:16 p:16 d#5:16 b4:16 a#4:16 p:16 c5:16 "
  "d5:16 c#5:16 p:16 c5:16 b4:16 a#4:16 p:16 b4:16 g4:16 g#4:16 p:16 a4:16 b4:16 a#4:16 p:16 c5:16 d5:16 c#5:16 "
  "p:16 c5:16 b4:16 a#4:16 p:16 b4:16 g4:16 g#4:16 p:16 a4:16 b4:16 b4:16 p:16 b4:16 [d5e5f#5]:4 b4:16 b4:16 p:16 "
  "b4:16 [f5g5]:4 b4:16 b4:16 p:16 b4:16 [d5f#5g#5]:4 b4:16 b4:16 p:16 b4:16 a5:8. g#5:16 b4:16 a#4:16 p:16 c5:16 "
  "d5:16 c#5:16 p:16 c5:16 b4:16 a#4:16 p:16 b4:16 g4:16 g#4:16 p:16 a4:16 b4:16 a#4:16 p:16 c5:16 d5:16 c#5:16 "
  "p:16 c5:16 b4:16 a#4:16 p:16 b4:16 g4:16 g#4:16 p:16 a4:16 b4:16 b4:16 p:16 b4:16 [d5e5f#5]:4 b4:16 b4:16 p:16 "
  "b4:16 [f5g5]:4 b4:16 b4:16 p:16 b4:16 [d5f#5g#5]:4 b4:16 b4:16 p:16 b4:16 b5:8. a#5:16 b4:16 a#4:16 p:16 c5:16 "
  "d5:16 c#5:16 p:16 c5:16 b4:16 a#4:16 p:16 b4:16 g4:16 g#4:16 p:16 a4:16 b4:16 a#4:16 p:16 c5:16 d5:16 c#5:16 "
  "p:16 c5:16 b4:16 a#4:16 p:16 b4:16 b4:16 d5:16 p:16 f#5:16 [f#5b5]:2. p:8. a4:16 [b4f5a#5]:4. p:16 c5:16 "
  "[b4e5a5]:4. p:16 g#5:16 b4:16 a#4:16 p:16 c5:16 d5:16 c#5:16 p:16 c5:16 b4:16 a#4:16 p:16 b4:16 g4:16 g#4:16 "
  "p:16 a4:16 b4:16 a#4:16 p:16 c5:16 d5:16 c#5:16 p:16 c5:16 b4:16 a#4:16 p:16 b4:16 b4:16 d5:16 p:16 f#5:16 "
  "[f#5b5]:2. p:8. a4:16 [g#5b5c#6]:4. p:16 c5:16 [b5e6]:4. p:16 d#6:16 b4:16 a#4:16 p:16 c5:16 d5:16 c#5:16 p:16 "
  "c5:16 b4:16 a#4:16 p:16 b4:16 g4:16 g#4:16 p:16 a4:16 b4:16 a#4:16 p:16 c5:16 d5:16 c#5:16 p:16 c5:16 b4:16 "
  "a#4:16 p:16 b4:16 g4:16 g#4:16 p:16 a4:16 [f#5b5]:4 b5:8 p:16 c5:16 [b4d5f#5]:4 f#5:8 p:16 a4:16 [b4e5g5]:4 "
  "g5:8 p:16 c5:16 [b4c#5e5]:4 e5:8 p:16 a4:16 [b4d5f#5]:4 f#5:8 p:16 c5:16 b4:8 p:16 b4:16 b4:16 c#5:16 p:16 "
  "d5:16 [g4b4c#5]:4. p:16 b4:16 b4:16 a#4:16 p:16 b4:16 g4:16 g#4:16 p:16 a4:16 d5:8. c#5:16 f#5:8. f5:16 b5:8. "
  "a#5:16 d6:8. c#6:16 [b5d6f#6]:2";
// Long Away Home (nene, CC0)
static const char LIED_HEIMWEG[] =
  "T150 [c#5f#5a5]:4 g#5:8 f#5:1 p:4. [d5a5]:1 p:4 [b4e5g#5]:2 [e5b5]:2 [f#5c#6]:1 p:2. [a5d6f#6]:1 p:4 [b5e6]:2 "
  "[g#5c#6d6]:4 [g#5c#6d6]:4 a5:8 f#5:8 g#5:8 f#5:8 a5:8 f#5:8 b5:8 f#5:8 a5:8 f#5:8 g#5:8 f#5:8 a5:8 f#5:8 b5:8 "
  "f#5:8 a5:8 d5:8 g#5:8 d5:8 a5:8 d5:8 b5:8 d5:8 [b4e5a5]:4 [e5b5]:4 [b4e5g#5]:2 a5:8 f#5:8 g#5:8 f#5:8 a5:8 "
  "f#5:8 b5:8 f#5:8 a5:8 f#5:8 g#5:8 f#5:8 a5:8 f#5:8 b5:8 f#5:8 a5:8 d5:8 g#5:8 d5:8 a5:8 d5:8 b5:8 d5:8 "
  "[b4e5a5]:4 [e5b5]:4 [c#5g#5]:2 [c#5f#5]:1 d5:8 c#5:8 [f#4b4]:1 p:4 [g4b4e5]:2 e5:8 f#5:8 [b4f#5]:1 p:4 f#5:8 "
  "f#5:8 f#5:8 f#5:8 [a4e5]:4 [a4d5]:4 d5:8 [a4c#5]:4 a4:8 a4:2 p:8 a4:8 c#5:8 d5:8 [g4e5]:4 [g4d5]:4 d5:8 "
  "[g4c#5]:4 g4:8 g4:2 p:8 g4:8 c#5:8 d5:8 d5:8 c#5:8 g4:8 d5:8 c#5:8 g4:8 d5:8 c#5:8 g4:8 d5:8 c#5:8 g4:8 d5:8 "
  "c#5:8 [e4a4]:4 d5:8 c#5:8 b4:1 p:4 [g4b4e5]:2 e5:8 f#5:8 [b4f#5]:1 p:4 f#5:8 f#5:8 f#5:8 f#5:8 [a4e5]:4 "
  "[a4d5]:4 d5:8 [a4c#5]:4 a4:8 a4:2 p:8 a4:8 c#5:8 d5:8 [a4e5]:4 [g4d5]:4 d5:8 [g4c#5]:4 g4:8 g4:2 p:8 g4:8 "
  "c#5:8 d5:8 d5:8 c#5:8 g4:8 d5:8 c#5:8 g4:8 d5:8 c#5:8 g4:8 d5:8 c#5:8 g4:8 d5:8 c#5:8 [e4a4]:4";
// Boss Battle #2 (nene, CC0)
static const char LIED_BOSS2[] =
  "T145 d#5:8. d#5:16 c#5:16 d#5:16 p:16 d#5:16 d#5:16 d#5:16 p:16 d#5:16 e4:16 d#5:16 p:16 c#5:16 a#5:16 d#5:16 "
  "p:16 d#5:16 a#5:16 d#5:8 d#5:16 b4:8. c#5:16 c#5:4 d#5:8. p:2. d#5:16 a#5:16 d#5:16 p:16 d#5:16 [a4e5]:4 "
  "[b4f#5]:4 [c#5f#5]:4 [d#5f#5a#5]:4 p:16 d#5:16 p:16 d#5:16 d#5:16 d#5:16 p:16 d#5:16 e4:16 d#5:16 p:16 c#5:16 "
  "[d#5b5]:4 p:16 d#5:16 p:16 d#5:16 b5:4 [c#5g#5]:4 [d#5a#5]:4 p:16 d#5:16 p:16 d#5:16 d#5:16 d#5:16 p:16 d#5:16 "
  "e4:16 d#5:16 p:16 c#5:16 d#5:16 d#5:16 p:16 d#5:16 [a4e5]:4 [b4f#5]:4 [c#5f#5]:4 [d#5a#5]:4 p:16 d#5:16 p:16 "
  "d#5:16 d#5:16 d#5:16 p:16 d#5:16 e4:16 d#5:16 p:16 c#5:16 [d#5b5]:4 p:16 d#5:16 p:16 d#5:16 b5:4 [c#5g#5]:4 "
  "[d#5a#5]:4 p:16 d#5:16 p:16 d#5:16 d#5:16 d#5:16 p:16 d#5:16 e4:16 d#5:16 p:16 c#5:16 d#5:16 d#5:16 p:16 "
  "d#5:16 [a4e5]:4 [b5f#6]:4 f#4:8 p:8 d#5:16 f#5:16 p:16 a#5:16 d#6:16 a#5:16 p:16 f#5:16 d#5:16 f#5:16 p:16 "
  "a#5:16 d#6:16 a#5:16 p:16 f#5:16 d#5:16 f#5:16 p:16 a#5:16 d#6:16 f#6:16 p:16 a#6:16 d#7:16 a#6:16 p:16 f#6:16 "
  "d#6:16 a#5:16 p:16 f5:16 d#5:16 f#5:16 p:16 a#5:16 d#6:16 a#5:16 p:16 f#5:16 d#5:16 f#5:16 p:16 a#5:16 d#6:16 "
  "a#5:16 p:16 f#5:16 d#5:16 f#5:16 p:16 a#5:16 d#6:16 a#5:16 p:16 f#5:16 b4:16 d#5:16 p:16 f#5:16 a#5:16 c#6:16 "
  "p:16 f6:16 d#5:16 f#5:16 p:16 a#5:16 d#6:16 a#5:16 p:16 f#5:16 d#5:16 f#5:16 p:16 a#5:16 d#6:16 a#5:16 p:16 "
  "f#5:16 d#5:16 f#5:16 p:16 a#5:16 d#6:16 f#6:16 p:16 a#6:16 d#7:16 a#6:16 p:16 f#6:16 d#6:16 a#5:16 p:16 f5:16 "
  "d#5:16 f#5:16 p:16 a#5:16 d#6:16 a#5:16 p:16 f#5:16 d#5:16 f#5:16 p:16 a#5:16 d#6:16 a#5:16 p:16 f#5:16 d#5:16 "
  "f#5:16 p:16 a#5:16 d#6:16 a#5:16 p:16 f#5:16 b4:16 d#5:16 p:16 f#5:16 a#5:16 c#6:16 p:16 f6:16 [d#5a#5]:2";
// Boss Battle #5 (nene, CC0)
static const char LIED_BOSS5[] =
  "T113 e4:16 e4:16 f#4:16 e4:16 e4:16 g4:16 e4:16 e4:16 [a4e5]:2 [e4d5]:2 [a4c5]:2 e4:16 e4:16 f#4:16 e4:16 "
  "e4:16 g4:16 e4:16 e4:16 a4:16 p:16 e4:16 e4:16 e4:16 p:16 e4:16 e4:16 [c5f5]:4 e5:8 [f5a#5]:4 a5:8 [a#5c#6]:4 "
  "e4:16 e4:16 f#4:16 e4:16 e4:16 g4:16 e4:16 e4:16 [a4e5]:2 [e4d5]:2 [a4c5]:2 e4:16 e4:16 f#4:16 e4:16 e4:16 "
  "g4:16 e4:16 e4:16 a4:16 p:16 e4:16 e4:16 e4:16 p:16 e4:16 e4:16 [c5f5]:4 e5:8 [f5a#5]:4 a5:8 [a5e6]:4 e4:16 "
  "e4:16 e4:16 e4:16 e4:16 e4:16 e4:16 e4:16 e4:16 e4:16 e4:16 e4:16 e4:16 e4:16 e4:16 e4:16 g4:16 g4:16 g4:16 "
  "g4:16 g4:16 g4:16 g4:16 g4:16 g4:16 g4:16 g4:16 g4:16 g4:16 g4:16 g4:16 g4:16 g4:16 g4:16 g4:16 g4:16 g4:16 "
  "g4:16 g4:16 g4:16 g4:16 g4:16 g4:16 g4:16 g4:16 g4:16 g4:16 g4:16 g4:16 g4:16 g4:16 g4:16 g4:16 g4:16 g4:16 "
  "g4:16 a4:16 a4:16 a4:16 a4:16 a4:16 a4:16 a4:16 a4:16 [b4e5]:2 [b4e5g5]:4 [e5b5]:4 [c5g5a5]:2 [c5g5]:4 "
  "[g4c5f#5]:4 [g4b4d5]:2 [g4d5f#5]:4 [d5g5a5]:4 [g5d6]:2. [a5e6]:4 [b5f#6]:4. p:16 g6:2 p:8 [g5d6]:4. [a5e6]:2 "
  "p:16 [b5f#6]:4. p:16 g6:2 p:16 [g5d6]:4. p:16 [a5e6]:2 p:16 f#6:16 b5:16 f#6:16 b5:16 e6:16 f#6:16 b5:16 g6:16 "
  "b5:16 e6:16 g6:16 b5:16 f#6:16 b5:16 e6:16 f#6:8 d6:16 d6:16 g5:16 c#6:16 d6:16 g5:16 e6:16 a5:16 d6:16 e6:16 "
  "a5:16 c#6:16 a5:16 c#6:16 d6:16 f#6:16 b5:16 f#6:16 b5:16 e6:16 f#6:16 b5:16 g6:16 b5:16 e6:16 g6:16 b5:16 "
  "f#6:16 b5:16 e6:16 f#6:16 d6:16 g5:16 d6:16 g5:16 c#6:16 d6:16 g5:16 e6:8 e6:16 e6:16 e6:16 [a6e7]:4";

// ---------- Melancholisch & traumhaft (OpenGameArt.org, frei) ----------
// Quellen: opengameart.org/content/4-music-box-tracks, /15-melodic-rpg-chiptunes, /jrpg-collection, /lullaby-1, /destiny-formation
// Traurige Spieluhr - "Music Box: Sad Tune" (Aureolus Omicron, CC0)
static const char LIED_SPIELUHR_TRAURIG[] =
  "T80 [c6d#6]:4. [c6g6]:4. f6:8 d#6:8 [b5g#6]:4. [g#6b6]:4. a#6:8 g#6:8 [a#5f6]:4. [a#5f6g#6]:4. f#6:8 f6:8 "
  "[a5f#6]:4. [f#6a6]:4. [a5c#6]:4 [g#5d#6]:4. [f#5g#5f6]:4. [g#5c#6f#6]:4 g#6:4. [f#5g#5c6]:4. [g#5c6d#6]:4 "
  "[g5f6]:4. [f5g5c6]:4. [g5c6f6]:4 [g5f6]:4. [f5g5b5]:4. [g5b5f6]:4 [c6d#6]:4. [c6g6]:4. f6:8 d#6:8 [b5g#6]:4. "
  "[g#6b6]:4. a#6:8 g#6:8 [a#5f6]:4. [a#5f6g#6]:4. f#6:8 f6:8 [a5f#6]:4. [f#6a6]:4. [a5c#6]:4 [g#5d#6]:4. "
  "[f#5g#5f6]:4. [g#5c#6f#6]:4 g#6:4. [f#5g#5c6]:4. [g#5c6d#6]:4 [g5f6]:4. [f5g5c6]:4. [g5c6f6]:4 [g5f6]:4. "
  "[f5g5b5]:4. [g5b5f6]:4 [c6d#6]:2 p:8 d#6:8 d6:8 d#6:8 [g#5f6]:4 c6:8 a#5:8 [f5g#5]:4 [g#5c6]:4 [a#5d6]:2 p:8 "
  "d6:8 c6:8 d6:8 [g5d#6]:4 a#5:8 g#5:8 [d#5g5]:4 [g5a#5]:4 [g#5c6]:2 p:8 c6:8 a#5:8 c6:8 d6:8 g#5:8 g5:8 g#5:8 "
  "d6:8 g#5:8 g5:8 g#5:8 c6:8 a5:8 g#5:8 a5:8 c6:8 a5:8 g#5:8 a5:8 c6:8 g5:8 f#5:8 g5:8 c6:8 g5:8 f#5:8 g5:8 b4:8 "
  "d5:8 g5:8 b5:8 d6:8 b5:8 d5:8 b4:8 g5:2.";
// Bittersuesse Geschichte - "Bittersweet Story" (Aureolus Omicron, CC0)
static const char LIED_BITTERSUESS[] =
  "T72 [d5f#5]:2 p:16 a4:16 f#5:16 g5:16 a5:8 b5:8 [f#5a5c#6]:2 p:16 c#5:16 c#5:16 d5:16 e5:8 f#5:8 [g4b4d5]:4 "
  "[g5b5d6]:2 g5:8 a5:8 [d5e5g5a#5]:2 p:16 d5:16 e5:16 f#5:16 g5:8 a#5:8 [e5a#5c6]:2 p:8. c5:16 [c5e5a#5]:4 "
  "[d5f5a5]:4 [a4d5f5g5]:4 [a4d5f5]:4 [d5f5a5]:4 [b4d5g5]:4 [g4b4d5e5]:4 [g4b4d5f5]:4. g5:8 [g4a4c#5e5]:2 p:8. "
  "g4:16 [g4a4c#5f5]:4 [d5f#5]:2 p:16 a4:16 f#5:16 g5:16 a5:8 b5:8 [f#5a5c#6]:2 p:16 c#5:16 c#5:16 d5:16 e5:8 "
  "f#5:8 [g4b4d5]:4 [g5b5d6]:2 g5:8 a5:8 [d5e5g5a#5]:2 p:16 d5:16 e5:16 f#5:16 g5:8 a#5:8 [e5a#5c6]:2 p:8. c5:16 "
  "[c5e5a#5]:4 [d5f5a5]:4 [a4d5f5g5]:4 [a4d5f5]:4 [d5f5a5]:4 g5:8 f5:8 [g4b4d5e5]:4 [g4b4d5f5]:4. g5:8 "
  "[g4a4c#5e5]:2 p:8. g4:16 f5:16 e5:16 d5:16 e5:16 [a4d5f5]:2 p:16 d5:16 f5:16 g5:16 a5:8 c6:8 [f#5a5c6d6]:4 "
  "[c5f#5a5]:2 [a4c5f#5g#5]:4 [a#4g5]:2 p:16 d5:16 g5:16 a5:16 a#5:8 c6:8 [f5g5b5d6]:2 p:8. b4:16 [f4g4b4d5]:4 "
  "[g4c5d#5]:2 p:16 c5:16 d#5:16 f5:16 g5:8 a#5:8 [e5g5a#5c6]:4 [a#4e5g5]:2 [g4a#4e5f#5]:4 [g#4f5]:2 p:16 c5:16 "
  "f5:16 g5:16 g#5:8 a#5:8 [d#5f5a5c6]:2 p:8. a4:16 [a4d#5f5]:4 [c#5f5a#5]:2 [a#4c#5f5g#5]:2 [c5d#5g5]:2 "
  "[g4c5d#5f#5]:2 [a4d5f5]:2 [d5f5a5]:2 [a#4d#5g5]:2 [d#5g5a#5]:2 [d5f5a5]:2 [a4d5f5g5]:2 [a4d5f5]:2 [a4d5f5g5]:2 "
  "[c#5e5a5]:2 p:8. e4:16 [a4c#5e5]:4 [a4c#5e5]:2.";
// Prinzessin - "JRPG Princess" (Yubatake, CC-BY 4.0)
static const char LIED_PRINZESSIN[] =
  "T110 p:2. f#5:8 g#5:8 [c#5f#5a#5]:4 [f#5c#6]:4 [c#5f5g#5]:4 [c#5f5a#5]:4 [a#4d#5f#5]:2 [a#4c#5]:4 g#5:8 a#5:8 "
  "[d#5f#5b5]:4 [f#5d#6]:4 [b4d#5a#5]:4 [d#5b5]:4 [f5g#5]:2 [f4c#5]:4 a#5:8 b5:8 [f#5a#5c#6]:2 [c#5f5b5]:4 "
  "[f5c#6]:4 [c#5e5b5]:4 [c#5e5a#5]:4 [e5g5a5]:4 [e5g5a#5]:4 [d#5g#5b5]:2 [d#5f#5a#5]:4 [d#5f#5b5]:4 [c#5f5g#5]:2 "
  "[f4c#5]:4 f#5:8 g#5:8 [c#5f#5a#5]:4 [f#5c#6]:4 [c#5f5g#5]:4 [c#5f5a#5]:4 [a#4d#5f#5]:2 [a#4c#5]:4 g#5:8 a#5:8 "
  "[d#5f#5b5]:4 [f#5d#6]:4 [b4d#5a#5]:4 [d#5b5]:4 [f5g#5]:2 [f4c#5]:4 a#5:8 b5:8 [f#5a#5c#6]:2 [c#5f5b5]:4 "
  "[f5c#6]:4 [c#5e5b5]:4 [c#5e5a#5]:4 [e5g5a5]:4 [e5g5a#5]:4 [d#5g#5b5]:2 [f#5b5d#6]:4 [f#5b5]:4 [c#5f5g#5]:2 "
  "[f4c#5]:4 f#5:8 f5:8 [a4d5f#5]:2 [b4e5g#5]:4 [b4e5]:4 [a4e5]:2 [g#4c#5]:4 f#5:8 f5:8 [a4d5f#5]:2 [b4e5g#5]:4 "
  "[e5b5]:4 [c#5f#5a#5]:2 [c#5f#5]:4 g#5:8 a#5:8 [d5g5b5]:2 [g5b5d6]:4 [g5b5]:4 [f#5a5]:4 [a4f#5g5]:4 [d5a5]:4 "
  "[d5a5]:4 [e5b5]:4 [b4e5a5]:4 [e5g#5b5]:4 [e5g#5d6]:4 [f#5c#6]:4 [a#4c#5]:4 [f#5a#5]:4 b5:8 c#6:8 [f#5b5d#6]:4 "
  "[b5f#6]:4 [f#5a#5c#6]:4 [f#5a#5d#6]:4 [d#5g#5b5]:2 [f#4d#5]:4 c#6:8 d#6:8 [g#5b5e6]:4 [b5g#6]:4 [e5g#5d#6]:4 "
  "[g#5e6]:4 [a#5c#6]:2 [a#4f#5]:4 d#6:8 e6:8 [b5d#6f#6]:2 [f#5a#5e6]:4 [a#5f#6]:4 [f#5a5e6]:4 [f#5a5d#6]:4 "
  "[a5c6d6]:4 [a5c6d#6]:4 [g#5c#6e6]:2 [g#5b5d#6]:4 [g#5b5e6]:4 [f#5a#5c#6]:2 [a#4f#5]:4 b5:8 c#6:8 [f#5b5d#6]:4 "
  "[b5f#6]:4 [f#5a#5c#6]:4 [f#5a#5d#6]:4 [d#5g#5b5]:2 [f#4d#5]:4 c#6:8 d#6:8 [g#5b5e6]:4 [b5g#6]:4 [e5g#5d#6]:4 "
  "[g#5e6]:4 [a#5c#6]:2 [a#4f#5]:4 d#6:8 e6:8 [b5d#6f#6]:2 [a#5d#6g6]:4 [d#6a#6]:4 [e6g#6c#7]:4 [e6g#6b6]:4 "
  "[b5e6a#6]:4 [b5e6g#6]:4 [g#5e6f#6]:4 [g#5e6]:4 [c#5e5g#5]:4 p:16 [c#5e5a#5]:4 p:16 [g#5b5c#6]:2 p:8 "
  "[d#5f#5g#5b5]:1";
// Wiegenlied - "Lullaby" (Yubatake, CC-BY 4.0)
static const char LIED_WIEGENLIED[] =
  "T38 p:2 p:8 a#5:8 c6:8 c5:8 a#5:16 g#5:16 g5:8 f4:8 f5:8 d#5:8 c5:8 g#4:8 d#5:8 p:8 d#5:16 f5:16 g5:8 g5:8 "
  "g5:8 a#5:8 a#4:16 g#5:16 g5:16 f5:16 e5:8 f5:8 g5:8 g#5:8 c5:8 f5:8 d5:8 d#5:8 f5:8 a#5:8 f4:16 c6:16 a#5:16 "
  "g#5:16 g#5:8 g5:8 d#5:8 f5:8 a#4:8 f#5:8 g5:8 g5:8 g5:8 a#5:8 a#4:16 g#5:16 g5:16 f5:16 e5:8 f5:8 g5:8 g#5:8 "
  "c5:8 c6:8 a#5:8 d6:8 c6:8 a#5:8 f4:16 g#5:16 g5:16 f5:16 d#5:8 c5:8 g#4:8 d#5:8 a#4:8 d#5:16 f5:16 g5:8 g5:8 "
  "g5:8 a#5:8 a#4:16 g#5:16 g5:16 f5:16 e5:8 f5:8 g5:8 g#5:8 c5:8 f5:8 d5:8 d#5:8 f5:8 a#5:8 f4:16 c6:16 a#5:16 "
  "g#5:16 g#5:8 g5:8 d#5:8 f5:8 a#4:8 f#5:8 g5:8 g5:8 g5:8 a#5:8 a#4:16 g#5:16 g5:16 f5:16 e5:8 f5:8 g5:8 g#5:8 "
  "c5:8 c6:8 a#5:8 d6:8 c6:8 a#5:8 f4:16 g#5:16 g5:16 f5:16 d#5:8 c5:8 g#4:8 d#5:8 a#4:8 d#5:8 c6:8 d#5:8 c6:8 "
  "c6:8 d6:8 c6:8 c6:8 a#5:8 a5:8 a#5:8 a#4:8 d#5:8 b5:8 d#5:8 b5:8 b5:8 c#6:8 g#4:16 b5:16 a#5:8 a#4:8 g4:8 "
  "d#5:8 a#4:8 d#5:8 c6:8 d#5:8 c6:8 c6:8 d6:8 c6:8 c6:8 a#5:8 a5:8 a#5:8 d5:8 a#5:8 a5:8 a#5:8 c6:8 d6:8 d#6:8 "
  "e6:8 g6:8 f6:8 f5:16 e6:16 f6:8 a#5:8 d#6:16 f6:16 g6:8 g6:8 g6:8 a#6:8 a#5:16 g#6:16 g6:16 f6:16 e6:8 f6:8 "
  "g6:8 g#6:8 c6:8 f6:8 d6:8 d#6:8 f6:8 a#6:8 f5:16 c7:16 a#6:16 g#6:16 g#6:8 g6:8 d#6:8 f6:8 a#5:8 f#6:8 g6:8 "
  "g6:8 g6:8 a#6:8 a#5:16 g#6:16 g6:16 f6:16 e6:8 c#7:8 c7:8 g#6:8 c6:8 a6:8 a#6:8 d7:8 c7:8 a#6:8 f5:8 d7:8 "
  "d#7:8 c6:8 g#5:8 d#5:8 g5:8 a#5:8 c6:8 c5:8 a#5:16 g#5:16 a#4:16 g5:8 f4:8 f5:8 g#4:16 d#5:8 p:16 c5:8 p:8 "
  "g#4:8 p:8. d#5:16 g5:8 p:8 d#6:8";
// Schicksal - "Destiny Formation" (Bobjt, CC0)
static const char LIED_SCHICKSAL[] =
  "T103 f#5:2. f#5:8 f#5:8 f5:2. d#5:8 f5:8 d#5:1 [a#4d#5]:2 [g#4f5]:2 [f#4d#5]:4 a#4:8 d#5:16 p:16 d#5:8 d#5:16 "
  "p:16 a#4:8 g#4:8 [f4c#5]:4 g#4:8 c#5:8 f5:4 [f4c#5]:4 d#5:8 b4:8 f#4:8 d#5:8 f#5:8 d#5:8 b4:8 a#4:8 [f#4a#4]:4 "
  "a#4:16 p:16 d#5:16 p:16 g#4:4 [f4g#4]:4 [f#4d#5]:4 a#4:8 d#5:16 p:16 b4:8 c#5:8 b4:8 a#4:8 [f4g#4c#5]:4 f#4:8 "
  "f4:8 d#5:8 f4:8 f#4:8 g#4:8 [f#4b4d#5]:4 d#5:8 c#5:8 b4:8 d#5:16 p:16 a#4:8 g#4:8 [f#4a#4]:4 [f#4a#4]:4 g#4:8 "
  "a#4:8 g#4:8 f#4:8 [d#5f#5]:4 a#4:8 d#5:16 p:16 d#5:8 d#5:16 p:16 f#5:8 f#5:8 f5:4 g#4:8 c#5:8 f5:4 d#5:8 f5:8 "
  "d#5:8 b4:8 f#4:8 d#5:8 f#5:8 d#5:8 b4:8 a#4:8 [a#4d#5]:2 [g#4f5]:4 [f4g#4]:4 [d#5f#5]:4 a#5:8 d#5:16 p:16 b5:8 "
  "c#6:8 b5:8 a#5:8 [c#5f5g#5]:4 f#5:8 f5:8 d#5:8 f5:8 f#5:8 g#5:8 [b4d#5f#5]:4 d#5:8 c#5:8 b4:8 d#5:16 p:16 "
  "a#4:8 g#4:8 [f#4a#4d#5]:4 a#4:4 f5:8 a#4:8 g#4:8 f#4:8 [a#4f#5]:2. f#5:16 p:16 d#5:16 p:16 f#5:16 p:16 d#5:16 "
  "p:16 f#4:16 p:16 d#5:16 p:16 f#5:16 p:16 d#5:16 p:16 f#4:16 p:16 f#4:16 p:16 f4:16 p:16 c#5:16 p:16 g#5:16 "
  "p:16 c#5:16 p:16 f4:16 p:16 c#5:16 p:16 d#5:16 p:16 f4:16 p:16 d#5:16 p:16 b4:16 p:16 b5:16 p:16 b4:16 p:16 "
  "d#5:16 p:16 b4:16 p:16 d#5:16 p:16 b4:16 p:16 a#4:16 p:16 d#5:16 p:16 a#4:16 p:16 d#5:16 p:16 g#5:16 p:8. "
  "g#4:16 p:16 f4:16";
// Spieluhr-Walzer - "Music Box: Spooky Waltz" (Aureolus Omicron, CC0)
static const char LIED_WALZER[] =
  "T108 e5:8 b4:8 e5:8 f#5:8 g5:8 e5:8 g5:8 f5:8 d#5:8 f5:8 g5:8 d#5:8 g5:8 f#5:8 g5:8 a5:8 b5:8 g5:8 f5:8 c#6:8 "
  "[c#5a#5]:2 e5:8 b4:8 e5:8 f#5:8 g5:8 e5:8 g5:8 f5:8 d#5:8 f5:8 g5:8 d#5:8 g5:8 f#5:8 g5:8 a5:8 b5:8 g5:8 "
  "[c#5a#5]:4 [a#4c#5]:2 e5:8 b4:8 e5:8 f#5:8 g5:8 e5:8 g5:8 f5:8 d#5:8 f5:8 g5:8 d#5:8 g5:8 f#5:8 g5:8 a5:8 b5:8 "
  "g5:8 f5:8 c#6:8 [c#5a#5]:2 e5:8 b4:8 e5:8 f#5:8 g5:8 e5:8 g5:8 f5:8 d#5:8 f5:8 g5:8 d#5:8 g5:8 f#5:8 g5:8 a5:8 "
  "b5:8 g5:8 [c#5a#5]:4 [a#4c#5]:2 a#5:8 a5:8 a#5:8 a5:8 a#5:8 a5:8 [d#5a#5]:4 [g4d#5]:2 d#6:8 d6:8 d#6:8 d6:8 "
  "d#6:8 d6:8 [g#5d#6]:4 [f#5g#5]:2 g#5:8 g5:8 g#5:8 g5:8 g#5:8 g5:8 [c#5g#5]:4 [f4c#5]:4 [b5c#6]:4 c#6:8 c6:8 "
  "c#6:8 c6:8 c#6:8 c6:8 [f#5c#6]:4 [e5f#5]:4 g5:16 g#5:16 a5:16 a#5:16 b5:4. d5:8 d5:8 b5:8 [b4a#5]:4. d5:8 d5:8 "
  "a#5:8 [b4a5]:4. d5:8 d5:8 a5:8 [b4g#5]:4. d5:8 d5:8 g#5:8 g5:8 d5:8 b4:8 d5:8 d5:8 g5:8 f#5:8 b5:8 b5:8 f#5:8 "
  "d5:8 f#5:8 f#5:8 a5:8 a5:8 f#5:8 d#5:8 a5:8 [b4a5]:4 [a4b4d#5]:4 [e5g5b5e6]:2";

static const char* const LIEDER[] = { LIED_TAUBE, LIED_RPG_TITEL, LIED_LUFTSCHIFF, LIED_ENDKAMPF,
                                      LIED_STADT, LIED_HYMNE, LIED_LEAP,
                                      LIED_JRPG_THEMA, LIED_JRPG_KAMPF, LIED_HEIMWEG, LIED_BOSS2, LIED_BOSS5,
                                      LIED_SPIELUHR_TRAURIG, LIED_BITTERSUESS, LIED_PRINZESSIN, LIED_WIEGENLIED, LIED_SCHICKSAL, LIED_WALZER };
static const int ANZAHL = sizeof(LIEDER) / sizeof(LIEDER[0]);

// ------------------------------------------------------------
static int nummer = -1;
static const char* pos = nullptr;          // Lesestelle im Text
static int tempo = 120;
static int akkord[6]; static int akkordN = 0;
static unsigned long noteStart = 0, noteDauer = 0, klangDauer = 0;
static int aktTon = 0;
static int notenZaehler = 0;
static int gesamtLaenge = 1;
static float viertelBisher = 0, noteViertel = 0;   // Taktposition fuer die Disco-Lichter

static float frequenz(char name, int vorz, int oktave) {
  static const int HALBTON[7] = {9, 11, 0, 2, 4, 5, 7};   // a b c d e f g
  int midi = 12 * (oktave + 1) + HALBTON[name - 'a'] + vorz;
  return 440.0f * powf(2.0f, (midi - 69) / 12.0f);
}

// eine Note "c#6" ab p lesen, liefert Frequenz und schiebt p weiter
static int leseNote(const char*& p) {
  char n = *p++;
  int vorz = 0;
  if (*p == '#') { vorz = 1; p++; }
  else if (*p == 'b' && isdigit((unsigned char)p[1])) { vorz = -1; p++; }
  int okt = 0;
  while (isdigit((unsigned char)*p)) okt = okt * 10 + (*p++ - '0');
  return (int)(frequenz(n, vorz, okt) + 0.5f);
}

// naechsten Baustein lesen. false = Lied zu Ende
static bool naechsteNote() {
  while (true) {
    while (*pos == ' ') pos++;
    if (*pos == 0) return false;
    if (*pos == 'T') { pos++; tempo = atoi(pos); while (isdigit((unsigned char)*pos)) pos++; continue; }
    akkordN = 0;
    if (*pos == 'p') { pos++; }
    else if (*pos == '[') {
      pos++;
      while (*pos && *pos != ']' && akkordN < 6) akkord[akkordN++] = leseNote(pos);
      if (*pos == ']') pos++;
    } else if (*pos >= 'a' && *pos <= 'g') {
      akkord[akkordN++] = leseNote(pos);
    } else { pos++; continue; }                     // unbekanntes Zeichen ueberspringen
    int teiler = 4;
    if (*pos == ':') { pos++; teiler = atoi(pos); while (isdigit((unsigned char)*pos)) pos++; }
    float ms = 60000.0f / tempo * 4.0f / max(1, teiler);
    float viertel = 4.0f / max(1, teiler);
    if (*pos == '.') { ms *= 1.5f; viertel *= 1.5f; pos++; }
    viertelBisher += noteViertel;
    noteViertel = viertel;
    noteStart = millis();
    noteDauer = (unsigned long)ms;
    // Chiptune-Artikulation: kurze Noten leicht abgesetzt, lange fast gebunden
    klangDauer = akkordN ? (unsigned long)(ms * (ms < 200 ? 0.80f : 0.92f)) : 0;
    if (akkordN) notenZaehler++;
    return true;
  }
}

int musikAnzahl() { return ANZAHL; }
int musikNummer() { return nummer; }
int musikTon() { return nummer >= 0 ? aktTon : 0; }
int musikNotenNummer() { return notenZaehler; }
float musikViertel() {
  if (nummer < 0 || noteDauer == 0) return 0;
  float anteil = min(1.0f, (millis() - noteStart) / (float)noteDauer);
  return viertelBisher + anteil * noteViertel;
}
int musikFortschritt() {
  if (nummer < 0) return 0;
  return constrain((int)((pos - LIEDER[nummer]) * 100L / gesamtLaenge), 0, 100);
}

void musikStopp() {
  if (nummer < 0) return;
  nummer = -1;
  aktTon = 0;
  buzzerMusik(-1);
  Serial.println(">> Musik: Stopp");
}

void musikStarten(int nr) {
  if (nr < 0 || nr >= ANZAHL) return;
  if (nr == nummer) { musikStopp(); return; }        // nochmal klicken = aus
  nummer = nr;
  pos = LIEDER[nr];
  gesamtLaenge = max(1, (int)strlen(LIEDER[nr]));
  tempo = 120;
  viertelBisher = 0; noteViertel = 0;
  Serial.printf(">> Musik: Lied %d\n", nr);
  if (!naechsteNote()) musikStopp();
}

void musikUpdate() {
  if (nummer < 0) return;
  unsigned long jetzt = millis();
  unsigned long t = jetzt - noteStart;
  if (t >= noteDauer) {
    if (!naechsteNote()) { musikStopp(); return; }
    t = 0;
  }
  int f = 0;
  if (akkordN && t < klangDauer) {
    if (akkordN > 1) {
      f = akkord[(t / 38) % akkordN];                      // Arpeggio: alle 38 ms der naechste Akkordton
    } else {
      f = akkord[0];
      if (noteDauer >= 400 && t > 150)                      // lange Noten: leichtes Vibrato
        f = (int)(f * (1.0f + 0.012f * sinf(((t - 150) / 10 * 10) * 0.0377f)));   // nur alle 10 ms neu
    }
  }
  aktTon = f;
  buzzerMusik(f);
}
