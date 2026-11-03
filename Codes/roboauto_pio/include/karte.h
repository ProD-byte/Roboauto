#pragma once
#include <Arduino.h>

// SD-Karte des Sense-Boards starten. false = keine Karte (alles andere laeuft trotzdem)
bool karteStarten();

bool karteDa();
uint32_t karteGesamtMB();
uint32_t karteBelegtMB();

// true = es steckt eine Karte, aber ohne FAT-Dateisystem (z.B. exFAT ab Werk bei 64 GB+)
bool karteUnlesbar();

// Karte als FAT32 neu anlegen (loescht ALLES). Laeuft im Hintergrund, Zustand mit karteFormatZustand().
// false = geht gerade nicht (keine Karte, Lernen laeuft, Formatieren laeuft schon)
bool karteFormatieren();
constexpr int KARTE_FMT_RUHE = 0, KARTE_FMT_LAEUFT = 1, KARTE_FMT_FERTIG = 2, KARTE_FMT_FEHLER = 3;
int  karteFormatZustand();

// Naechste freie Nummer fuer "/ordner/name_0001..." (fuer Fahrten und Fotos)
int karteNaechsteNummer(const char* ordner, const char* name);

// Kurzer Ueberblick als JSON fuer die Webseite:
// {"da":1,"gesamt":..,"belegt":..,"fahrten":..,"fotos":..,"unlesbar":0,"fmt":0}
String karteJson();

// Seiten /dateien und /datei liefert der Medien-Server, siehe medien.h
