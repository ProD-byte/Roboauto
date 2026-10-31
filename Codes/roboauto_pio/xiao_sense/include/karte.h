#pragma once
#include <Arduino.h>

// SD-Karte des Sense-Boards starten. false = keine Karte (alles andere laeuft trotzdem)
bool karteStarten();

bool karteDa();
uint32_t karteGesamtMB();
uint32_t karteBelegtMB();

// Naechste freie Nummer fuer "/ordner/name_0001..." (fuer Fahrten und Fotos)
int karteNaechsteNummer(const char* ordner, const char* name);

// Kurzer Ueberblick als JSON fuer die Webseite: {"da":1,"gesamt":..,"belegt":..,"fahrten":..,"fotos":..}
String karteJson();

// Seiten /dateien und /datei liefert der Medien-Server, siehe medien.h
