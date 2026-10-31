#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "gyro.h"

// MPU6050-Register
constexpr uint8_t REG_KONFIG      = 0x1A;   // digitaler Tiefpass
constexpr uint8_t REG_GYRO_KONFIG = 0x1B;   // Gyro-Messbereich
constexpr uint8_t REG_ACC_KONFIG  = 0x1C;   // Beschleunigungs-Messbereich
constexpr uint8_t REG_DATEN       = 0x3B;   // ab hier 14 Bytes: Acc XYZ, Temp, Gyro XYZ
constexpr uint8_t REG_POWER       = 0x6B;   // Aufwecken

// Gyro +-1000 Grad/s (bei +-250 war der Sensor beim schnellen Drehen am Anschlag)
constexpr uint8_t GYRO_BEREICH = 0x10;
constexpr float   LSB_PRO_GRAD = 32.8f;
// Beschleunigung +-2 g (Standard) -> 16384 pro g; fuer Winkel zaehlt nur das Verhaeltnis

// Komplementaerfilter: 98 % Gyro (schnell, driftet) + 2 % Beschleunigung (traege, driftfrei)
constexpr float FILTER_GYRO_ANTEIL = 0.98f;

static bool  bereit = false;
static float offset[3] = {0, 0, 0};          // Gyro-Nullpunkt X, Y, Z in Grad/s
static float rate[3]   = {0, 0, 0};          // aktuelle Drehraten X, Y, Z in Grad/s
static float winkelZ   = 0;                  // Drehwinkel um die Hochachse (Gieren)
static float neigungX  = 0;                  // Kippwinkel um die X-Achse des Sensors
static float neigungY  = 0;                  // Kippwinkel um die Y-Achse des Sensors
static float accG[3]   = {0, 0, 1};          // Beschleunigung X, Y, Z in g (1 g = Erdanziehung)
constexpr float LSB_PRO_G = 4096.0f;         // bei +-8 g
static unsigned long letzteMessung = 0;      // in Mikrosekunden
static uint32_t messNummer = 0;              // zaehlt jede Messung hoch

// --- Kalibrierung im Hintergrund (waehrend der loop weiterlaeuft) ---
static bool  kalAktiv = false;
static int   kalAnzahl = 0;
static long  kalSumme[3];
static int16_t kalMin[3], kalMax[3];
static int   kalErgebnis = 0;                // 0 = nichts Neues, 1 = fertig, 2 = abgebrochen (bewegt)
constexpr int KAL_BEWEGUNG = 100;            // Rohwert-Schwankung (ca. 3 Grad/s) = Auto wurde bewegt

static bool schreibeRegister(uint8_t reg, uint8_t wert) {
  Wire.beginTransmission(GYRO_ADRESSE);
  Wire.write(reg);
  Wire.write(wert);
  return Wire.endTransmission() == 0;
}

// Alle Rohwerte in einem Rutsch lesen
static bool leseAlles(int16_t acc[3], int16_t gyr[3]) {
  Wire.beginTransmission(GYRO_ADRESSE);
  Wire.write(REG_DATEN);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(GYRO_ADRESSE, (uint8_t)14) != 14) return false;
  uint8_t b[14];
  for (int i = 0; i < 14; i++) b[i] = Wire.read();
  for (int i = 0; i < 3; i++) {
    acc[i] = (int16_t)((b[i * 2] << 8) | b[i * 2 + 1]);
    gyr[i] = (int16_t)((b[8 + i * 2] << 8) | b[8 + i * 2 + 1]);
  }
  return true;
}

// Neigung allein aus der Schwerkraft (Beschleunigungssensor)
static float accNeigungX(const int16_t a[3]) { return atan2f(a[1], a[2]) * 180.0f / PI; }
// Y = Kippachse beim Balancieren. atan2(-ax, az) deckt den vollen Bereich +-180 Grad ab,
// damit der Wert auch bei fast senkrecht stehendem Auto (um -90 Grad) nicht "zurueckklappt".
static float accNeigungY(const int16_t a[3]) { return atan2f(-a[0], a[2]) * 180.0f / PI; }

bool gyroStarten() {
  Wire.begin(GYRO_SDA_PIN, GYRO_SCL_PIN);
  Wire.setClock(400000);

  bereit = schreibeRegister(REG_POWER, 0x00)          // aufwecken
        && schreibeRegister(REG_GYRO_KONFIG, GYRO_BEREICH)
        && schreibeRegister(REG_ACC_KONFIG, 0x10)     // +-8 g (bei +-2 g waere ein Aufprall am Anschlag)
        && schreibeRegister(REG_KONFIG, 0x03);        // Tiefpass ~44 Hz gegen Motor-Vibrationen

  Serial.println(bereit ? ">> Gyro (MPU6050) gefunden" : ">> FEHLER: Gyro (MPU6050) nicht gefunden!");
  return bereit;
}

void gyroKalibrieren() {
  if (!bereit) return;
  Serial.println(">> Gyro-Kalibrierung... Auto bitte NICHT bewegen");

  long summe[3] = {0, 0, 0};
  int gueltig = 0;
  int16_t acc[3], gyr[3];
  for (int i = 0; i < GYRO_KALIBRIER_MESSUNGEN; i++) {
    if (leseAlles(acc, gyr)) {
      for (int k = 0; k < 3; k++) summe[k] += gyr[k];
      gueltig++;
    }
    delay(5);
  }
  for (int k = 0; k < 3; k++) offset[k] = gueltig > 0 ? (float)summe[k] / gueltig / LSB_PRO_GRAD : 0;

  // Startwerte der Neigung direkt aus der Schwerkraft
  if (leseAlles(acc, gyr)) {
    neigungX = accNeigungX(acc);
    neigungY = accNeigungY(acc);
  }
  winkelZ = 0;
  letzteMessung = micros();
  Serial.printf(">> Gyro kalibriert, Nullpunkt X=%.2f Y=%.2f Z=%.2f Grad/s\n", offset[0], offset[1], offset[2]);
}

void gyroUpdate() {
  if (!bereit) return;
  unsigned long jetzt = micros();
  if (jetzt - letzteMessung < 5000) return;      // alle 5 ms messen (200x pro Sekunde)

  int16_t acc[3], gyr[3];
  if (!leseAlles(acc, gyr)) return;

  float dt = (jetzt - letzteMessung) / 1000000.0f;
  letzteMessung = jetzt;

  // Kalibrierung laeuft: Messwerte sammeln und pruefen, ob das Auto wirklich still steht
  if (kalAktiv) {
    for (int k = 0; k < 3; k++) {
      kalSumme[k] += gyr[k];
      if (kalAnzahl == 0 || gyr[k] < kalMin[k]) kalMin[k] = gyr[k];
      if (kalAnzahl == 0 || gyr[k] > kalMax[k]) kalMax[k] = gyr[k];
      if (kalMax[k] - kalMin[k] > KAL_BEWEGUNG) {
        kalAktiv = false;
        kalErgebnis = 2;
        Serial.println(">> Kalibrierung ABGEBROCHEN - das Auto wurde bewegt");
        return;
      }
    }
    if (++kalAnzahl >= GYRO_KALIBRIER_MESSUNGEN) {
      for (int k = 0; k < 3; k++) offset[k] = (float)kalSumme[k] / kalAnzahl / LSB_PRO_GRAD;
      neigungX = accNeigungX(acc);
      neigungY = accNeigungY(acc);
      winkelZ = 0;                              // Kompass: aktuelle Richtung = 0 Grad
      kalAktiv = false;
      kalErgebnis = 1;
      Serial.printf(">> Gyro neu kalibriert, Nullpunkt X=%.2f Y=%.2f Z=%.2f Grad/s\n", offset[0], offset[1], offset[2]);
    }
    return;
  }

  for (int k = 0; k < 3; k++) rate[k] = gyr[k] / LSB_PRO_GRAD - offset[k];
  for (int k = 0; k < 3; k++) accG[k] = acc[k] / LSB_PRO_G;

  // Gieren (Hochachse) - wie bisher, kleines Rauschen im Stillstand ignorieren
  float rz = fabs(rate[2]) < 0.3f ? 0 : rate[2];
  winkelZ += rz * dt;

  // Neigung: Gyro hochrechnen und mit der Schwerkraft langsam nachziehen
  neigungX = FILTER_GYRO_ANTEIL * (neigungX + rate[0] * dt) + (1 - FILTER_GYRO_ANTEIL) * accNeigungX(acc);
  neigungY = FILTER_GYRO_ANTEIL * (neigungY + rate[1] * dt) + (1 - FILTER_GYRO_ANTEIL) * accNeigungY(acc);
  messNummer++;
}

bool  gyroBereit()      { return bereit; }
float gyroDrehrate()    { return fabs(rate[2]) < 0.3f ? 0 : rate[2]; }
float gyroWinkel()      { return winkelZ; }
void  gyroWinkelReset() { winkelZ = 0; }
float gyroNeigungX()    { return neigungX; }
float gyroNeigungY()    { return neigungY; }
float gyroRateX()       { return rate[0]; }
float gyroRateY()       { return rate[1]; }
uint32_t gyroMessNummer() { return messNummer; }
float gyroBeschleunigung() { return sqrtf(accG[0] * accG[0] + accG[1] * accG[1] + accG[2] * accG[2]); }
float gyroAufrecht()       { float g = gyroBeschleunigung(); return g > 0.2f ? accG[2] / g : 1; }
float gyroStossWaagerecht() { return sqrtf(accG[0] * accG[0] + accG[1] * accG[1]); }

void gyroKalibrierungStarten() {
  if (!bereit || kalAktiv) return;
  kalAktiv = true;
  kalAnzahl = 0;
  kalErgebnis = 0;
  for (int k = 0; k < 3; k++) kalSumme[k] = 0;
  Serial.println(">> Gyro-Kalibrierung gestartet - Auto 1 Sekunde nicht bewegen");
}
bool  gyroKalibriertGerade()     { return kalAktiv; }
float gyroKalibrierFortschritt() { return kalAnzahl / (float)GYRO_KALIBRIER_MESSUNGEN; }
int   gyroKalibrierErgebnis()    { int e = kalErgebnis; kalErgebnis = 0; return e; }
