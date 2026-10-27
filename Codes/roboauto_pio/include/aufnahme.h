#pragma once

// Fahrweg aufnehmen und abspielen.
// Aufgenommen werden die Steuerbefehle (Joystick x/y) mit ihrer Dauer.
// Die Aufnahme wird im Speicher des ESP32 gesichert und bleibt nach dem Ausschalten erhalten.

void aufnahmeStarten();              // laedt eine gespeicherte Aufnahme (einmal in setup)

void aufnahmeUmschalten();           // Aufnahme an/aus
void abspielenUmschalten();          // Abspielen an/aus

bool aufnahmeLaeuft();
bool abspielenLaeuft();
bool aufnahmeVorhanden();
float aufnahmeSekunden();            // Laenge der gespeicherten Aufnahme
float abspielenSekunden();           // wie weit das Abspielen schon ist

// In jedem loop-Durchlauf aufrufen, mit den aktuellen Steuerwerten des Fahrers.
// Beim Abspielen werden x/y durch die aufgenommenen Werte ersetzt.
// Bewegt der Fahrer beim Abspielen den Joystick, wird das Abspielen abgebrochen.
void aufnahmeUpdate(int& x, int& y);
