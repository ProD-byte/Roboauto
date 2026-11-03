#include <Arduino.h>
#include <time.h>
#include <sys/time.h>
#include "zeit.h"

static const char* ZONE = "CET-1CEST,M3.5.0,M10.5.0/3";   // Deutschland, Sommerzeit Ende Maerz bis Ende Oktober

void zeitStarten() {
  setenv("TZ", ZONE, 1);
  tzset();
}

void zeitInternet() {
  configTzTime(ZONE, "pool.ntp.org", "time.google.com");   // laeuft im Hintergrund weiter
}

void zeitSetzen(uint32_t sekunden) {
  if (sekunden < 1700000000UL) return;                     // offensichtlich falsch (vor Ende 2023)
  time_t jetzt = time(nullptr);
  if (zeitBekannt() && labs((long)(jetzt - (time_t)sekunden)) < 5) return;   // stimmt schon
  timeval tv = { (time_t)sekunden, 0 };
  settimeofday(&tv, nullptr);
  Serial.printf(">> Uhrzeit gestellt (vom Browser/App)\n");
}

bool zeitBekannt() { return time(nullptr) > 1700000000; }

static void formatiere(char* puffer, size_t groesse, const char* format) {
  time_t jetzt = time(nullptr);
  tm t;
  localtime_r(&jetzt, &t);
  strftime(puffer, groesse, format, &t);
}

void zeitTag(char* puffer, size_t groesse) {
  if (zeitBekannt()) formatiere(puffer, groesse, "%Y-%m-%d");
  else strlcpy(puffer, "ohne-datum", groesse);
}

void zeitUhr(char* puffer, size_t groesse) {
  if (zeitBekannt()) formatiere(puffer, groesse, "%H-%M-%S");
  else snprintf(puffer, groesse, "nach-%lus", millis() / 1000);   // ohne Uhrzeit: Sekunden seit dem Start
}
