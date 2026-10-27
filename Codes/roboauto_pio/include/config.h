#pragma once
// ============================================================
//  Zentrale Einstellungen: Pins und Fahrwerte
// ============================================================

// --- Motoren (H-Bruecke) ---
constexpr int MOTOR_L_VOR    = 1;
constexpr int MOTOR_L_RUECK  = 4;
constexpr int MOTOR_R_VOR    = 13;
constexpr int MOTOR_R_RUECK  = 2;

// Drehrichtung je Motor umkehren, falls er falsch herum angeschlossen ist.
// Im Fahrtest ermittelt: der rechte Motor laeuft "verkehrt herum".
constexpr bool MOTOR_L_INVERTIERT = false;
constexpr bool MOTOR_R_INVERTIERT = false;   // neue Autos: nichts umgekehrt - per Assistent einstellen
// (Startwerte - auf der Webseite unter Einstellungen > Motoren aenderbar)
constexpr int  MOTORTEST_TEMPO = 120;     // Motortest: Staerke
constexpr unsigned long MOTORTEST_MS = 1500;   // Motortest: Dauer

// --- Weitere Pins (kommen in spaeteren Schritten dazu) ---
constexpr int LED_RING_PIN   = 5;
constexpr int BUZZER_PIN     = 6;
constexpr int FRONT_LED_PIN  = 7;
constexpr int REAR_LED_PIN   = 10;
constexpr int GYRO_SDA_PIN   = 8;
constexpr int GYRO_SCL_PIN   = 9;
constexpr int AKKU_PIN       = 3;

// --- LED-Ring ---
constexpr int LED_RING_ANZAHL     = 8;
constexpr int LED_RING_HELLIGKEIT = 40;   // 0..255 (wie in V5)

// --- Front- und Ruecklichter (je 2 NeoPixel) ---
constexpr int FRONT_LED_ANZAHL   = 2;
constexpr int REAR_LED_ANZAHL    = 2;
constexpr int LICHT_HELLIGKEIT   = 40;    // 0..255 (wie in V5)
constexpr int REAR_LED_RUECKFAHR = 1;     // diese Ruecklicht-LED wird beim Rueckwaertsfahren weiss

// Farben (wie in V5)
constexpr uint32_t FARBE_FAHRLICHT  = 0x966400;  // gelb, stromsparend
constexpr uint32_t FARBE_FERNLICHT  = 0xFFFFFF;  // weiss
constexpr uint32_t FARBE_RUECKLICHT = 0x1E0000;  // rot gedimmt
constexpr uint32_t FARBE_BREMSLICHT = 0xFF0000;  // rot hell
constexpr uint32_t FARBE_RUECKFAHR  = 0xC8C8C8;  // weiss
constexpr uint32_t FARBE_AUS        = 0x000000;

// Welche LED ist links? (0 oder 1) - die andere ist automatisch rechts
constexpr int FRONT_LED_LINKS = 0;
constexpr int REAR_LED_LINKS  = 0;

constexpr uint32_t FARBE_BLINKER    = 0xE63C00;  // orange

constexpr unsigned long BREMSLICHT_DAUER = 400;  // ms
constexpr unsigned long BLINK_TAKT       = 250;  // ms an / ms aus

// --- Hupe ---
constexpr int HUPE_FREQUENZ = 300;               // Hz (wie V5)

// --- Licht-Modi (Select in Dabble bzw. Knoepfe auf der Webseite schalten durch) ---
enum Modus { MODUS_NORMAL = 0, MODUS_POLIZEI, MODUS_BLAU, MODUS_GRUEN, MODUS_GELB, MODUS_ROT, MODUS_ANZAHL };
constexpr const char* MODUS_NAME[MODUS_ANZAHL] = { "Normal", "Polizei", "Team Blau", "Team Gruen", "Team Gelb", "Team Rot" };

// Teams: Grundfarbe (Farbton 0..65535) und wie weit der Farbton hin- und herschwingt
//                                   Normal Polizei  Blau    Gruen   Gelb   Rot
constexpr uint16_t TEAM_FARBTON[MODUS_ANZAHL] = { 0, 0,     40000,  21000,  9500,  0     };
constexpr uint16_t TEAM_SCHWANK[MODUS_ANZAHL] = { 0, 0,      4000,   7000,  2000,  2200  };
constexpr int      TEAM_HUPE_HZ[MODUS_ANZAHL] = { HUPE_FREQUENZ, 0, 600, 1200, 900, 450 };   // eigener Hupenton je Team

// --- Polizei-Modus ---
constexpr int SIRENE_MIN_HZ = 600;
constexpr int SIRENE_MAX_HZ = 1400;
constexpr unsigned long SIRENE_RUNDE_MS = 400;   // einmal hoch und runter (kleiner = hektischer)
constexpr unsigned long DOPPELKLICK_ZEIT = 350;  // ms

// --- Akku (ADC-Rohwerte 0..4095, wie in V5 kalibriert) ---
constexpr int AKKU_LAEDT       = 2300;  // darueber: nur USB/Netzteil (kein Akku) -> Ring komplett blau.
                                        // (vorher 2000: ein ganz voller Akku lag schon darueber)
constexpr int AKKU_ANZEIGE_MIN = 1400;  // = 0 LEDs (leer)
constexpr int AKKU_ANZEIGE_MAX = 1950;  // = 8 LEDs (voll)
constexpr int AKKU_ECO_AN      = 1500;  // darunter: Eco-Modus an
constexpr int AKKU_ECO_AUS     = 1600;  // darueber: Eco-Modus wieder aus (Hysterese)
constexpr unsigned long AKKU_VERLAUF_MS = 10000;   // alle 10 s ein Wert fuer das Diagramm
constexpr int AKKU_VERLAUF_PUNKTE      = 360;     // 360 x 10 s = 1 Stunde

// --- Gyroskop (MPU6050) ---
constexpr uint8_t GYRO_ADRESSE        = 0x68;
constexpr int     GYRO_KALIBRIER_MESSUNGEN = 200;   // x 5 ms = 1 Sekunde stillhalten

// --- Fahrwerte ---
constexpr int JOYSTICK_MAX    = 7;    // Dabble liefert -7 ... +7
constexpr int TOTZONE         = 1;    // |Wert| <= 1 gilt als Mitte
constexpr int DPAD_STAERKE     = 7;    // Pfeiltasten wirken wie Joystick-Ausschlag 0..7 (7 = voll)
constexpr int PWM_MAX         = 255;  // technische Obergrenze der Motoren
constexpr int MAX_TEMPO       = 128;  // Fahrgeschwindigkeit: 128 = halb so schnell wie frueher (255 = voll)
constexpr int MAX_LENKUNG     = 75;   // Lenkstaerke waehrend der Fahrt (halbiert, frueher 150)
constexpr int MAX_LENKUNG_STAND = 140; // Lenkstaerke beim Drehen auf der Stelle - braucht viel Kraft,
                                       // weil die Reifen seitlich rutschen muessen (Panzerlenkung).
                                       // Dazwischen wird fliessend uebergeblendet.


// --- Geradeauslauf mit Gyro ---
// Faehrt das Auto ohne Lenkbefehl, haelt das Gyro die Richtung.
constexpr bool  GERADEAUS_AKTIV     = true;
constexpr float GERADEAUS_KP        = 4.0f;   // Korrektur pro Grad Abweichung
constexpr float GERADEAUS_KD        = 0.5f;   // Daempfung pro Grad/s Drehrate
constexpr int   GERADEAUS_MAX_KORR  = 80;     // maximale Korrektur (von 255)

// --- 180-Grad-Wende (X doppelt klicken) ---
constexpr float WENDE_WINKEL      = 180.0f;  // Grad, links herum
constexpr int   WENDE_TEMPO_MAX   = 180;     // Drehtempo am Anfang
constexpr int   WENDE_TEMPO_MIN   = 110;     // langsamstes Drehtempo kurz vor dem Ziel
constexpr float WENDE_BREMS_BEREICH = 45.0f; // ab so vielen Grad vor dem Ziel langsamer werden
constexpr float WENDE_TOLERANZ    = 4.0f;    // Grad: so nah reicht als "fertig"
constexpr float WENDE_VORHALT     = 0.10f;   // s: so lange rollt das Auto nach dem Stopp
                                             // noch nach -> entsprechend frueher abschalten.
                                             // Schiesst es noch ueber: groesser (z.B. 0.15),
                                             // bleibt es zu frueh stehen: kleiner (z.B. 0.05)
constexpr unsigned long WENDE_MAX_ZEIT = 4000; // ms Sicherheits-Abbruch

// --- WLAN / Webseite ---
constexpr const char* WEB_NAME = "roboauto";   // Adresse im Browser: http://roboauto.local
constexpr const char* SETUP_WLAN_NAME = "Roboauto-Setup";   // eigenes WLAN zum Einrichten (ohne Passwort)
constexpr unsigned long WLAN_VERGESSEN_HALTEN_MS = 3000;    // Start + Select so lange halten = WLAN neu einrichten
constexpr unsigned long WEB_TOTMANN_MS = 400;   // ohne neuen Fahrbefehl so lange -> Stopp
constexpr unsigned long WEB_GETRENNT_MS = 6000;  // so lange ohne Lebenszeichen der Seite -> "Webseite getrennt"
constexpr unsigned long WLAN_NEUVERSUCH_MS = 30000; // im Einrichtungsmodus alle 30 s erneut das Heim-WLAN probieren
constexpr unsigned long WLAN_TIMEOUT_MS = 15000;

// --- Auto-Update von GitHub (siehe autoupdate.h) ---
constexpr const char* UPDATE_URL = "https://prod-byte.github.io/Roboauto/";   // GitHub-Pages-Adresse (mit / am Ende)
constexpr unsigned long UPDATE_ERSTE_SUCHE_MS = 60000;       // so lange nach dem WLAN-Verbinden warten, dann nachsehen
constexpr unsigned long UPDATE_INTERVALL_MS = 6UL * 3600000; // danach alle 6 Stunden
constexpr unsigned long UPDATE_RUHE_MS = 120000;             // so lange muss das Auto unbenutzt stehen, bevor es sich aktualisiert
constexpr unsigned long UPDATE_BESTAETIGEN_MS = 60000;       // so lange muss eine neue Version laufen, dann gilt sie als gut
constexpr int UPDATE_PROBE_STARTS = 3;                       // so viele Starts ohne Bestaetigung -> zurueck zur alten Version
constexpr int UPDATE_MIN_AKKU = 30;                          // Akku-Prozent, ab dem automatisch aktualisiert wird (USB immer) // so lange auf das WLAN warten, dann Fehlermeldung

// --- Figuren fahren (Quadrat, Acht, Slalom) ---
// Klein eingestellt: alle Figuren sollen in ca. 50 x 50 cm passen.
constexpr int   FIGUR_TEMPO        = 90;     // Fahrtempo auf Geraden (Quadrat)
constexpr int   FIGUR_BOGEN_TEMPO  = 75;     // Grundtempo in Boegen (Acht, Slalom)
constexpr int   FIGUR_BOGEN_LENK   = 70;     // Unterschied innen/aussen. Fast so gross wie das Bogentempo
                                             // -> inneres Rad steht fast -> sehr enge Kurve
constexpr unsigned long FIGUR_GERADE_MS = 400;   // Laenge einer Quadrat-Seite (Fahrzeit) - groesser = groesseres Quadrat
constexpr float FIGUR_VORHALT      = 0.10f;  // s Nachlauf, wie bei der Wende
constexpr unsigned long FIGUR_SEGMENT_MAX_MS = 6000;  // Sicherheits-Abbruch je Abschnitt

// --- Fahrweg aufnehmen / abspielen ---
constexpr int           AUFNAHME_MAX_EINTRAEGE = 500;     // so viele Lenk-/Gas-Aenderungen passen hinein
constexpr unsigned long AUFNAHME_MAX_MS        = 120000;  // maximal 2 Minuten

// --- Balancieren (auf zwei Raedern, Schnauze hoch) ---
// Einschalten: Auto am "schwerelosen" Punkt halten, dann Start halten + Dreieck (Dabble)
// oder Knopf "Balancieren" auf der Webseite. Die aktuelle Neigung wird zum Balancierpunkt.
constexpr int   BALANCE_RICHTUNG   = 1;      // auf -1 stellen, falls das Auto in die Kipprichtung WEGfaehrt
constexpr float BALANCE_KP         = 18.0f;  // PWM pro Grad Neigung (Hauptwert)
constexpr float BALANCE_KD         = 0.8f;   // PWM pro Grad/s Kippgeschwindigkeit (Daempfung)
constexpr float BALANCE_KI         = 0.0f;   // gegen langsames Wegdriften (erst spaeter erhoehen)
constexpr int   BALANCE_MIN_PWM    = 30;     // Motoren-Totzone ueberwinden
constexpr int   BALANCE_MAX_PWM    = 180;    // Obergrenze (weniger Stromspitzen)
constexpr int   BALANCE_MAX_SPRUNG = 20;     // max. PWM-Aenderung pro 5 ms (sanfte Richtungswechsel)
constexpr float BALANCE_UMFALL_WINKEL = 35.0f;  // mehr Abweichung = umgefallen -> Motoren aus
constexpr float BALANCE_FAHR_WINKEL   = 3.0f;   // Joystick vor/zurueck verschiebt den Sollwinkel max. so weit
constexpr int   BALANCE_LENKUNG       = 60;     // Joystick links/rechts beim Balancieren

// --- Crash- und Umkipp-Erkennung ---
constexpr bool  CRASH_AKTIV          = true;
constexpr float CRASH_STOSS_G        = 2.5f;   // waagerechter Stoss in g = Aufprall (kleiner = empfindlicher)
constexpr int   CRASH_MESSUNGEN      = 3;      // so viele Messungen hintereinander (je 5 ms) ueber der Grenze
constexpr unsigned long CRASH_ANFAHR_MS = 400; // nach Anfahren/Richtungswechsel so lange keinen Aufprall pruefen
constexpr unsigned long CRASH_SPERRE_MS = 1500; // nach einem Aufprall so lange nicht fahren
constexpr float UMGEKIPPT_AUFRECHT   = 0.4f;   // darunter (ca. 65 Grad Schraeglage) gilt als umgekippt
constexpr unsigned long UMGEKIPPT_MS = 600;    // so lange schraeg = wirklich umgekippt (kein Hopser)

constexpr const char* BLE_NAME = "RoboautoV3";
