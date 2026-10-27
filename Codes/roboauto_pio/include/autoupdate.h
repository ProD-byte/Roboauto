#pragma once
// ============================================================
//  Auto-Update: Das Auto holt sich neue Firmware selbst von GitHub.
//
//  - GitHub baut bei jedem Hochladen eine neue Version (Build-Nummer)
//    und legt sie zusammen mit version.json auf GitHub Pages ab.
//  - Das Auto schaut nach dem WLAN-Verbinden und dann alle paar Stunden nach.
//  - Installiert wird nur, wenn das Auto in Ruhe steht (niemand faehrt, Akku ok).
//  - Rueckfall-Schutz: Startet die neue Version nicht sauber (3 Starts ohne
//    Bestaetigung), schaltet das Auto automatisch auf die alte Version zurueck.
//  - Selbst gebaute Versionen (VS Code) haben Build 0 und werden nie automatisch
//    ueberschrieben - nur ueber den Knopf auf der Webseite.
// ============================================================

enum UpdateZustand { UPD_RUHE = 0, UPD_SUCHT = 1, UPD_LAEDT = 2, UPD_AKTUELL = 3, UPD_FEHLER = 4, UPD_VERFUEGBAR = 5 };

void autoUpdateStart();                        // ganz am Anfang von setup (Rueckfall-Pruefung)
void autoUpdateLoop(bool wlanDa, bool ruhig);   // in jedem loop-Durchlauf
void autoUpdateSuchen();                       // Webseite: jetzt nachsehen
void autoUpdateInstallieren();                 // Webseite: verfuegbares Update jetzt installieren
int  autoUpdateBuild();                        // eigene Build-Nummer (0 = selbst gebaut)
int  autoUpdateNeu();                          // neueste Build-Nummer auf GitHub (0 = unbekannt)
int  autoUpdateZustand();                      // siehe UpdateZustand
int  autoUpdateProzent();                      // Download-Fortschritt
bool autoUpdateZurueckgesetzt();               // true, wenn beim Start auf die alte Version zurueckgeschaltet wurde
