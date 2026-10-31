#pragma once

// Bluetooth-Verbindung zur Roboauto-App (ersetzt Dabble).
// Die App schickt dieselben Befehle wie die Webseite, z. B. "0|/fahr?x=3&y=7&t=0".
// Protokoll und UUIDs: docs/app-protokoll.md

// Einmal in setup() aufrufen
void appBleStarten();

// In jedem loop-Durchlauf: empfangene Befehle ausfuehren, Antworten zurueckschicken
void appBleUpdate();
