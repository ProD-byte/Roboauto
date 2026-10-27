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

static const char* const LIEDER[] = { LIED_TAUBE, LIED_RPG_TITEL, LIED_LUFTSCHIFF, LIED_ENDKAMPF,
                                      LIED_STADT, LIED_HYMNE, LIED_LEAP };
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
