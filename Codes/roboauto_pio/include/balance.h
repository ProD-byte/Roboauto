#pragma once

// Balancier-Modus ein/aus. Beim Einschalten wird die aktuelle Neigung
// als Balancierpunkt uebernommen. Rueckgabe: neuer Zustand
bool balanceUmschalten();

bool balanceAktiv();

// In jedem loop-Durchlauf aufrufen, solange balanceAktiv() true ist.
// x/y = Joystick (-7..7): vor/zurueck verschiebt den Sollwinkel, links/rechts lenkt.
void balanceUpdate(int x, int y);

// Sofort aus (z.B. bei Verbindungsabbruch)
void balanceAus();
