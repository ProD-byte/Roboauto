#pragma once

// Selbststaendig Figuren fahren - mit dem Gyro fuer genaue Winkel.
enum Figur { FIGUR_QUADRAT = 0, FIGUR_ACHT = 1, FIGUR_SLALOM = 2, FIGUR_ANZAHL = 3 };
constexpr const char* FIGUR_NAME[FIGUR_ANZAHL] = { "Quadrat", "Acht", "Slalom" };

// Figur starten. false = nicht moeglich (kein Gyro)
bool figurStarten(int figur);

// Laufende Figur abbrechen (z.B. Joystick bewegt)
void figurAbbrechen();

bool figurAktiv();
int  figurNummer();          // aktuelle Figur, -1 = keine

// In jedem loop-Durchlauf aufrufen, solange figurAktiv() true ist
void figurUpdate();
