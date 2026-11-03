#include <Arduino.h>
#include <Preferences.h>
#include <esp_system.h>
#include "neustart.h"

static int grund = 0, abstuerze = 0, letzter = 0;

static bool istAbsturz(int g) {
  return g == ESP_RST_PANIC || g == ESP_RST_INT_WDT || g == ESP_RST_TASK_WDT || g == ESP_RST_WDT ||
         g == ESP_RST_BROWNOUT;
}

void neustartMerken() {
  grund = (int)esp_reset_reason();
  Preferences p;
  p.begin("system", false);
  abstuerze = p.getInt("abstuerze", 0);
  letzter   = p.getInt("absturzGrund", 0);
  if (istAbsturz(grund)) {
    abstuerze++;
    letzter = grund;
    p.putInt("abstuerze", abstuerze);
    p.putInt("absturzGrund", letzter);
  }
  p.end();
}

int  neustartGrund()          { return grund; }
bool neustartWarAbsturz()     { return istAbsturz(grund); }
int  neustartAbstuerze()      { return abstuerze; }
int  neustartLetzterAbsturz() { return letzter; }

void neustartZaehlerLoeschen() {
  abstuerze = 0;
  letzter = 0;
  Preferences p;
  p.begin("system", false);
  p.putInt("abstuerze", 0);
  p.putInt("absturzGrund", 0);
  p.end();
}
