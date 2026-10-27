#pragma once

// MPU6050 starten. Rueckgabe false = Sensor nicht gefunden (Rest laeuft trotzdem)
bool gyroStarten();

// Nullpunkt messen - das Auto muss dabei ca. 1 Sekunde STILL stehen
void gyroKalibrieren();

// In jedem loop-Durchlauf aufrufen (liest den Sensor und rechnet den Winkel hoch)
void gyroUpdate();

bool  gyroBereit();
float gyroDrehrate();      // aktuelle Drehung um die Hochachse in Grad/Sekunde
float gyroWinkel();        // aufsummierter Drehwinkel in Grad seit dem letzten Reset
void  gyroWinkelReset();

// Kippwinkel (fuer das Balancieren) um die X- bzw. Y-Achse des Sensors in Grad.
// Welche davon "nach vorne kippen" ist, haengt davon ab, wie der Sensor eingebaut ist.
float gyroNeigungX();
float gyroNeigungY();
float gyroRateX();         // Drehrate um X in Grad/s
float gyroRateY();         // Drehrate um Y in Grad/s

// Zaehlt jede neue Messung hoch (200x pro Sekunde). Wer nur bei neuen Daten rechnen will,
// merkt sich die letzte Nummer und vergleicht.
uint32_t gyroMessNummer();

// Gesamtbeschleunigung in g (Stillstand ca. 1,0; Aufprall deutlich mehr)
float gyroBeschleunigung();

// Wie aufrecht steht das Auto? 1 = normal auf den Raedern, 0 = auf der Seite, -1 = auf dem Dach
float gyroAufrecht();

// Waagerechter Stoss in g (vor/zurueck/seitlich) - Bodenwellen (senkrecht) zaehlen nicht
float gyroStossWaagerecht();

// Neu kalibrieren, waehrend alles weiterlaeuft (ca. 1 s, Auto muss still stehen).
// Bewegt sich das Auto dabei, wird abgebrochen und die alten Werte bleiben.
void  gyroKalibrierungStarten();
bool  gyroKalibriertGerade();
float gyroKalibrierFortschritt();     // 0..1
int   gyroKalibrierErgebnis();        // einmalig: 1 = fertig, 2 = abgebrochen, sonst 0
