#pragma once
#include <Arduino.h>

// ============================================================
//  Alle WS2812-LEDs haengen an EINEM Pin (LED_KETTE_PIN):
//  ESP -> Ring (DIN) -> Ring DOUT -> Frontlicht -> Ruecklicht.
//  Ring und Lichter schreiben ihre Farben hier hinein, ausgegeben wird gemeinsam.
// ============================================================

void ketteStarten();

// Farbe einer LED setzen. nr = Platz in der Kette (siehe LED_KETTE_* in config.h),
// hell = Helligkeit dieses Bereichs 0..255 (Ring und Lichter haben getrennte Regler)
void ketteSetze(int nr, uint8_t r, uint8_t g, uint8_t b, uint8_t hell);

// Geaenderte Farben ausgeben (hoechstens alle paar ms). sofort = ohne Wartezeit
// (fuer Fortschrittsbalken, wenn der loop gerade nicht laeuft)
void ketteZeigen(bool sofort = false);
