#pragma once

// Warum ist das Auto zuletzt gestartet? Abstuerze (Fehler, Watchdog, Spannungseinbruch) werden
// dauerhaft mitgezaehlt - so sieht man auf der Webseite auch spaeter noch, dass etwas war.

void neustartMerken();            // einmal ganz am Anfang von setup()

int  neustartGrund();             // esp_reset_reason() dieses Starts (1 = Einschalten, 4 = Absturz ...)
bool neustartWarAbsturz();        // dieser Start kam nach einem Absturz
int  neustartAbstuerze();         // so viele Abstuerze insgesamt (seit dem Zuruecksetzen)
int  neustartLetzterAbsturz();    // Grund des letzten Absturzes, 0 = noch keiner
void neustartZaehlerLoeschen();   // Zaehler auf 0 (Knopf auf der Webseite)
