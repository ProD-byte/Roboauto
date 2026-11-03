# Roboauto – vollständige Projektübersicht

> Diese Datei erklärt das ganze Projekt so, dass jemand ohne Vorwissen (z. B. eine KI wie ChatGPT)
> versteht, was das Roboauto ist, wie es aufgebaut ist, wie die Software zusammenspielt und wo was liegt.
> Stand: 31.10.2026 · Firmware „Schritt 40“ · App-Version 1.0.3x

---

## 1. Was ist das Roboauto?

Das Roboauto ist ein kleines, selbst konstruiertes Roboterauto zum Basteln, Fahren und Lernen.

- **Fahrwerk:** 4 Räder mit Panzerlenkung (Skid-Steer). Es gibt keine Lenkachse: Gelenkt wird, indem die
  linke und die rechte Seite unterschiedlich schnell drehen. Auf der Stelle drehen geht auch.
- **Gehäuse:** komplett selbst in SketchUp konstruiert und 3D-gedruckt (aktuell Version 6, Version 7 in Arbeit).
- **Elektronik:** ESP32-S3 (Mikrocontroller mit WLAN + Bluetooth), Motortreiber, Gyro, LEDs, Buzzer, LiPo-Akku.
- **Software:** alles selbst geschrieben – Firmware fürs Auto, Weboberfläche, eigene Android-App, automatischer
  Build und Online-Installer. Es wird **keine** fremde Steuer-App mehr benutzt (früher „Dabble“, jetzt entfernt).
- **Gesteuert wird** per eigener **Roboauto-App** (Android) oder im **Browser** – mit derselben Oberfläche.

Es gibt zwei Varianten:

| Variante | Board | Ordner | Besonderheiten |
|---|---|---|---|
| **Hauptauto** | ESP32-S3 Super Mini (4 MB Flash) | `Codes/roboauto_pio` | Standardauto |
| **XIAO Sense** | Seeed Studio XIAO ESP32S3 Sense (8 MB Flash, 8 MB PSRAM) | `Codes/roboauto_pio` (env `roboauto_xiao`) | zusätzlich Kamera, Mikrofon, SD-Karte, alle LEDs an einem Pin, Akku-Chip MAX17048 |

---

## 2. Was kann das Auto?

| Bereich | Funktionen |
|---|---|
| 🎮 Steuerung | Joystick in App/Browser (Werte −7…+7), Tastatur am PC (W A S D / Pfeiltasten), Totmann-Schalter: kommt 400 ms kein Fahrbefehl, bleibt das Auto stehen |
| 🧭 Gyro (MPU6050) | Geradeauslauf (Gyro hält die Richtung), 180°-Wende auf Knopfdruck, Kompass, Neigungsanzeige, Kalibrieren |
| 💡 Licht | Front-/Rücklicht, Fernlicht, Blinker links/rechts, Warnblinker, Bremslicht, Rückfahrlicht |
| 🔵 LED-Ring (8 × WS2812) | zeigt Zustand und Verbindungsart (siehe Abschnitt 6), Tankanzeige, Musik-Lichtshow, Alarm |
| 🚨 Modi | Normal, Polizei (Blaulicht + Sirene), Team Blau/Grün/Gelb/Rot (Farbspiel + eigener Hupenton) |
| ✨ Programme | Figuren fahren (Quadrat, Acht, Slalom), Fahrweg aufnehmen und abspielen (max. 2 min), Balancieren auf zwei Rädern (Experiment) |
| 🎵 Jukebox | 18 Chiptune-Lieder über den Buzzer, Disco-Lichter im Takt |
| 🛡️ Sicherheit | Crash-Erkennung (Stoß > 2,5 g) und Umkipp-Erkennung → Motoren aus, Alarm, Warnblinker |
| 🔋 Akku | Akkustand, Eco-Modus bei schwachem Akku, Akkuverlauf der letzten Stunde |
| 📶 WLAN | merkt sich bis zu 8 WLANs, Einrichtung per Handy, eigenes WLAN „Roboauto-XXXX“ zum Fahren ohne Router |
| 🔄 Updates | Auto-Update von GitHub (nur wenn das Auto 2 min steht und der Akku reicht, mit automatischem Zurückfallen auf die alte Version, falls die neue nicht sauber startet); Updates per WLAN aus VS Code (OTA) |
| 🔧 Einstellungen | Tempo, Lenkung, Drehen auf der Stelle, Figurengröße, Helligkeiten, Lautstärke, Töne, Motor-Richtungen mit Motortest-Assistent – alles im Auto gespeichert |
| 📷 nur XIAO | Live-Video im Browser, Fotos auf SD-Karte, Live-Ton, Lautstärkepegel, zweimal Klatschen = Licht an/aus, „Lernen“ (schreibt Fahrdaten + Bilder auf die SD-Karte) |

---

## 3. Hardware (Hauptauto)

- ESP32-S3 Super Mini (4 MB Flash)
- 4 × N20-Getriebemotor 1000 U/min, Motortreiber MX1508 / DRV8833 (Motoren einer Seite parallel)
- MPU6050 (Gyro + Beschleunigung) an I²C
- WS2812-LED-Ring (8 LEDs), 2 Front-LEDs, 2 Rück-LEDs (auch WS2812)
- passiver Buzzer (Ton und Lautstärke per PWM)
- 1S-LiPo 3,7 V über Schalter, Spannungsteiler zur Akkumessung

| ESP32-S3 Pin | Anschluss | Funktion |
|---|---|---|
| GPIO1 / GPIO4 | Motortreiber IN1 / IN2 | Motoren links vor / zurück (PWM) |
| GPIO13 / GPIO2 | Motortreiber IN3 / IN4 | Motoren rechts vor / zurück (PWM) |
| GPIO5 | LED-Ring DIN | 8 × WS2812B |
| GPIO7 | Frontlicht DIN | 2 × WS2812B |
| GPIO10 | Rücklicht DIN | 2 × WS2812B (LED 1 = Rückfahrlicht) |
| GPIO6 | Buzzer | passiver Buzzer |
| GPIO8 / GPIO9 | MPU6050 SDA / SCL | I²C, Adresse 0x68 |
| GPIO3 | Spannungsteiler | Akkuspannung (ADC) |

Alle Pins und Einstellwerte stehen in `include/config.h`.

---

## 4. Software-Architektur – wie alles zusammenspielt

```
                ┌───────────────────────── Handy (Android) ─────────────────────────┐
                │  Roboauto-App (Kotlin)                                              │
                │   └─ WebView zeigt dieselbe Weboberfläche wie der Browser            │
                │      app.js leitet fetch('/status'), fetch('/fahr?...') an die App  │
                │   ├─ Bluetooth (BLE, eigener GATT-Dienst)                            │
                │   ├─ Heim-WLAN (HTTP + UDP, Auto per mDNS „roboauto“ finden)         │
                │   └─ Auto-WLAN „Roboauto-XXXX“ direkt (192.168.4.1, ohne Router)          │
                └───────────────┬───────────────────────────┬───────────────────────┘
                                │ BLE                         │ WLAN (HTTP / UDP 4211)
                ┌───────────────▼───────────────────────────▼───────────────────────┐
                │  Firmware im Auto (C++/Arduino, PlatformIO)                         │
                │   app_ble.cpp ─┐                                                     │
                │   UDP 4211 ────┼─► steuerBefehl()  (ein Verteiler für alle Wege)    │
                │   HTTP :80 ────┘        │                                            │
                │                         ▼                                            │
                │   main.cpp loop(): Fahren, Lichter, Ring, Buzzer, Gyro, Sicherheit … │
                └────────────────────────────────────────────────────────────────────┘
                PC-Browser ──HTTP──► http://roboauto.local  (gleiche Oberfläche)
```

**Kernidee:** Es gibt **eine** Weboberfläche (`include/webseite.h`) und **einen** Befehlsverteiler
(`steuerBefehl()` in `src/web.cpp`). Browser, App über WLAN und App über Bluetooth schicken genau dieselben
Befehle. Dadurch funktioniert jede neue Funktion sofort überall.

### 4.1 Firmware (`Codes/roboauto_pio`)

- Sprache C++ mit Arduino-Framework, gebaut mit **PlatformIO** (VS Code), Plattform espressif32 8.0.0, Arduino-Core 3.x.
- `main.cpp`: `setup()` startet alle Teile, `loop()` liest Befehle, fährt, steuert Lichter/Ring/Buzzer, prüft Sicherheit.
- Die Weboberfläche wird beim Bauen mit gzip gepackt (`scripts/webseite_gz.py` → `include/webseite_gz.h`), spart Flash.

| Datei (src/ + include/) | Aufgabe |
|---|---|
| `main.cpp` | Hauptprogramm, verbindet alles |
| `config.h` | alle Pins, Grenzwerte, Namen, Ports |
| `web.cpp / web.h` | WLAN (Heim-WLAN, Einrichtung, eigenes WLAN „Roboauto-XXXX“), Webserver, Befehlsverteiler, UDP, OTA |
| `webseite.h` | komplette Weboberfläche (HTML/CSS/JS in einer Datei), Deutsch/Englisch/Spanisch/Französisch |
| `app_ble.cpp / .h` | Bluetooth-Dienst für die App |
| `antrieb.cpp` | Motoren, Panzerlenkung, Geradeauslauf, Wende, Motortest |
| `gyro.cpp` | MPU6050, Kalibrierung, Winkel, Neigung |
| `lichter.cpp` | Front-/Rücklicht, Blinker, Warnblinker, Bremslicht, Disco |
| `ledring.cpp` | alle Ring-Animationen |
| `buzzer.cpp`, `musik.cpp` | Töne, Hupe, Sirene, Melodien, Jukebox |
| `akku.cpp` | Akkumessung, Eco-Modus, Verlauf |
| `sicherheit.cpp` | Crash- und Umkipp-Erkennung |
| `figuren.cpp`, `aufnahme.cpp`, `balance.cpp` | Figuren, Fahrweg aufnehmen/abspielen, Balancieren |
| `einstellungen.cpp` | Einstellungen im Flash speichern (Preferences) |
| `autoupdate.cpp` | Auto-Update von GitHub Pages mit Rückfall-Schutz |

Zusätzlich nur bei XIAO: `kamera.cpp`, `mikro.cpp`, `karte.cpp` (SD), `lernen.cpp`, `medien.cpp`
(Medien-Server Port 81 für Video/Ton/Dateien), `ledkette.cpp` (alle LEDs an einem Pin).

### 4.2 Die Befehle (gleich für Browser, WLAN-App und Bluetooth-App)

| Pfad | Parameter | Antwort | Zweck |
|---|---|---|---|
| `/fahr` | `x`, `y` (−7…7), `t` (Bits: 1 Blinker links, 2 Blinker rechts, 4 Hupe, 8 Tank) | `ok` | Fahren, ca. alle 80 ms |
| `/aktion` | `a` = Nummer | `ok` | Einmal-Aktionen (siehe unten) |
| `/status` | – | JSON | Live-Werte (Akku, Motoren, Licht, Neigung, Kurs, Ring-Farben …), ca. alle 250 ms |
| `/einstellungen` | ohne = lesen; `name=wert` = ändern; `standard`, `motorStandard` | JSON | Einstellungen |
| `/verlauf` | – | JSON | Akkuverlauf |
| `/update` | `a=suchen` / `a=installieren` | `ok` | Auto-Update |
| `/info` | – | JSON | Name, Bluetooth-Name, IP, WLAN da?, Firmware-Build, UDP-Port |
| `/kamera`, `/karte` | nur XIAO | | Kamera-Auflösung, SD-Karten-Übersicht |

Aktionsnummern: 1 Licht · 2 Fernlicht · 3 Warnblinker · 4 Wende · 5 nächster Modus · 10–15 Modus direkt ·
20–22 Figur (Quadrat, Acht, Slalom) · 30 Aufnahme · 31 Abspielen · 32 Balancieren · 33 Gyro kalibrieren ·
34/35 Motortest links/rechts · 40 Musik stopp · 41+n Lied n.

### 4.3 Übertragungswege

- **HTTP** (Browser und App im WLAN): normale GET-Anfragen, z. B. `http://192.168.178.42/fahr?x=3&y=7&t=0`.
- **UDP** (nur Fahrbefehle der App im WLAN, Port 4211): ein Paket `0|/fahr?x=3&y=7&t=0`, kein Verbindungsaufbau
  → etwa 4× schneller als HTTP. Den Stopp schickt die App dreimal.
- **Bluetooth LE** (App): eigener GATT-Dienst
  - Service `8d3a1000-5b7e-4f2a-9c61-2e4b7a9d0c11`
  - Befehl (write) `8d3a1001-…`, Antwort (notify) `8d3a1002-…`
  - Anfrage `"<nr>|<pfad>?<params>"`, Antwort `"<nr>|<text>\n"` (in MTU-Stücken), `nr = 0` = keine Antwort
  - MTU 517, Gerätename „Roboauto-XXXX“ (XXXX = Chip-Nummer, XIAO: „RoboautoXiao-XXXX“) – so sind mehrere Autos unterscheidbar
  - Reißt Bluetooth ab, stoppt das Auto sofort.
- Ausführlich: `docs/app-protokoll.md`.

### 4.4 WLAN-Verhalten

1. Beim Start sucht das Auto bekannte WLANs (bis zu 8 gespeichert) und verbindet sich mit dem stärksten.
2. **Gleichzeitig** öffnet es sein eigenes offenes WLAN **„Roboauto-XXXX“** (Adresse 192.168.4.1) – zum Fahren ohne Router.
3. Findet es kein bekanntes WLAN, wird daraus der **Einrichtungsmodus** „Roboauto-Setup“: Handy verbinden,
   die Einrichtungsseite öffnet sich von selbst, WLAN auswählen, Passwort eingeben.
4. Im Heim-WLAN ist das Auto unter `http://roboauto.local` erreichbar (mDNS).
- Wichtig: Bluetooth und WLAN teilen sich eine Antenne. Solange Bluetooth an ist, „döst“ das WLAN zwischen
  Router-Signalen (Vorgabe von Espressif) – deshalb ist der direkte Weg über das Auto-WLAN oder UDP schneller.

---

## 5. Die Roboauto-App (`Codes/roboauto_app`)

- **Android 8 und neuer**, Kotlin, minSdk 26 / targetSdk 35, AGP 8.7, Gradle 8.11, JDK 17. Kein iPhone.
- Die App ist eine **Hülle um die Weboberfläche**: Beim Bauen wird `webseite.h` aus der Firmware ausgelesen
  und als Asset eingebaut (Gradle-Task `WebseiteAusziehen`). Ändert sich die Weboberfläche, ändert sich die App mit.
- `app.js` läuft vor der Weboberfläche und leitet ihre `fetch()`-Aufrufe an die App weiter
  (Brücke `window.Roboauto`). Die App schickt sie per Bluetooth, HTTP oder UDP ans Auto.
- `verbinden.html`: Startbildschirm im gleichen Design – sucht Autos per Bluetooth und im WLAN (mDNS),
  zeigt Signalstärke, merkt sich das letzte Auto (verbindet automatisch), WLAN-Adresse von Hand,
  Eintrag „WLAN des Autos ‚Roboauto‘“ (ab Android 10 verbindet die App selbst per WifiNetworkSpecifier
  und bindet ihre Anfragen an dieses Netz; das Handy behält Internet über Mobilfunk).
- `MainActivity.kt` (WebView, Erlaubnisse, Brücke), `BleVerbindung.kt` (Suchen, Verbinden, Schreib-Warteschlange,
  nur der neueste Fahrbefehl zählt, automatisches Neuverbinden), `WlanVerbindung.kt` (HTTP, UDP, mDNS-Suche,
  Low-Latency-WLAN-Sperre).
- Bedienung: Pille „Bluetooth ⇄ / WLAN ⇄“ oben oder Zurück-Taste = Verbindung wechseln. Bildschirm bleibt an.
- **Build:** GitHub Actions baut die APK bei jedem Push. Versionsnummer = Build-Nummer (`1.0.<Lauf>`).
  Signiert wird mit einem eigenen Schlüssel aus GitHub-Secrets (`ROBOAUTO_KEYSTORE_BASE64`,
  `ROBOAUTO_KEYSTORE_PASSWORT`); fehlen sie, wird mit einem Wegwerf-Schlüssel signiert.

---

## 6. Weboberfläche (`include/webseite.h`)

Modernes dunkles Dashboard, eine Datei, keine externen Bibliotheken:

- **Steuerung:** großer Joystick, Knöpfe für Licht, Fernlicht, Warnblinker, Blinker (halten), Hupe (halten),
  Wende, Tank (halten), Kalibrieren, Modus-Chips.
- **Live:** Auto von oben (Lichter und Ring leuchten wie am echten Auto), Kompass, Motorleistung,
  Neigung vorn/hinten und seitlich, Akkuverlauf.
- **Programme:** Figuren, Fahrweg, Balancieren. **Jukebox** und **Einstellungen** zum Aufklappen.
- **Handy hochkant:** aufgeräumt, keine Tastatur-Hinweise, Statuszeile mit Symbolen.
- **Handy quer:** Gamepad-Ansicht – Joystick links, Knöpfe rechts, alles ohne Scrollen.
- Scrollen geht auch auf Knöpfen; nur Halte-Knöpfe und Joystick sperren Wischen.
- Sprachen: Deutsch, Englisch, Spanisch, Französisch.

**LED-Ring-Bedeutung:**

| Ring | Bedeutung |
|---|---|
| dunkelblau atmend, Punkt kreist | niemand verbunden |
| Regenbogen dreht sich | App per Bluetooth |
| türkiser Komet | über den Router (Heim-WLAN) |
| violetter Komet | direkt im Auto-WLAN „Roboauto-XXXX“ |
| kurzes Füllen in der Farbe | gerade verbunden / Weg gewechselt |
| orange atmend | WLAN-Einrichtung offen |
| Blaulicht / Teamfarben | Polizei- bzw. Team-Modus |
| roter Punkt oben | Fahrweg wird aufgenommen; schneller grüner Komet = wird abgespielt |
| rot/weiß blitzend | Crash oder umgekippt |

---

## 7. Build, Verteilung, Updates

- **GitHub-Repository** `prod-byte/Roboauto`, Workflow „Firmware bauen“ (`.github/workflows/build.yml`):
  baut Hauptauto (`env:roboauto`), XIAO (`env:roboauto_xiao`) und die App.
- Ergebnis landet auf **GitHub Pages** `https://prod-byte.github.io/Roboauto/`:
  - **Online-Flasher** (ESP Web Tools): Firmware ohne Programmieren per USB im Browser aufspielen
  - `roboauto.apk` + `app.json` (Version, Größe) zum Herunterladen, QR-Code am PC
  - Firmware-Dateien für das Auto-Update
- Ein Tag wie `v1.2.0` erzeugt ein Release mit allen Dateien.
- **Projektseite:** `https://daveslab.de/basteln/` (WordPress) mit Download-Abschnitten für Roboauto,
  Roboauto XIAO, ESPCam und die Roboauto-App (Version wird live von `app.json` geholt).
- Selbst bauen: VS Code + PlatformIO, `geheim.beispiel.ini` → `geheim.ini`,
  `include/wlan_zugang.beispiel.h` → `wlan_zugang.h`, Umgebung `env:roboauto` (USB) oder `env:roboauto_wlan` (OTA).

---

## 8. Ordnerstruktur

```
Roboauto/
├─ README.md
├─ .github/workflows/build.yml        automatischer Build (Firmware, XIAO, App, Pages)
├─ Codes/
│  ├─ roboauto_pio/                    Firmware für beide Autos (ein PlatformIO-Projekt)
│  │  ├─ src/, include/, scripts/
│  │  └─ docs/                         Online-Flasher (index.html), app-protokoll.md, xiao-sense.md, Bilder, Schaltpläne
│  └─ roboauto_app/                    Android-App (Kotlin, Gradle)
│     ├─ app/src/main/java/de/roboauto/app/   MainActivity, BleVerbindung, WlanVerbindung
│     └─ app/web/                      verbinden.html, app.js
├─ 3D-Druck/                           aktuelle Druckteile (.skp SketchUp, .stl), alte Versionen
├─ Modell/                             komplettes Auto als 3D-Modell (V6, V7 in Arbeit)
└─ Platine/                            Platine, Pinbelegung, Schaltplan (XIAO ESP32-C6, geplant)
```

---

## 9. Geschichte (kurz)

Das Auto wurde Schritt für Schritt aufgebaut (die Firmware zählt ihre Schritte, aktuell 40):
Bluetooth-Fahren mit der fremden App Dabble → LED-Ring und Buzzer → Lichter, Blinker → Akku → Gyro,
Geradeauslauf, Wende → WLAN und Weboberfläche → Modi → WLAN-Einrichtung per Handy → Crash-Erkennung →
Figuren, Fahrweg, Balancieren → Jukebox → Auto-Update, Online-Flasher → mehrsprachig →
**eigene Android-App** mit Bluetooth, UDP und eigenem Auto-WLAN → **Dabble komplett entfernt** (Schritt 40).

---

## 10. Offene Punkte / Ideen

- App in den Google Play Store (Material in `Codes/roboauto_app/playstore/`).
- Platine mit XIAO ESP32-C6 (Schaltplan liegt in `Platine/`).

---

## 11. Hinweise für eine KI, die an diesem Projekt hilft

- Code-Kommentare, Variablen und Texte sind **auf Deutsch** (ohne Umlaute im Code: ae/oe/ue/ss).
- Neue Funktionen immer über `steuerBefehl()` in `web.cpp` anbinden – dann gehen sie in Browser, WLAN-App und
  Bluetooth-App gleichzeitig. Neue Knöpfe kommen in `webseite.h`, die App übernimmt sie beim nächsten Build.
- Grenzwerte und Pins gehören nach `config.h`, Einstellungen, die der Nutzer ändern kann, nach `einstellungen.cpp`.
- Flash ist beim Hauptauto knapp (ca. 91 % belegt) – große Bibliotheken vermeiden.
- Beide Autos sind **ein** Projekt: XIAO-Teile stehen in `#ifdef ROBOAUTO_XIAO` (C++) bzw. zwischen
  `<!--XIAO-->…<!--/XIAO-->` und `/*XIAO*/…/*/XIAO*/` (webseite.h). Reine XIAO-Dateien (Kamera, Mikro, SD,
  MAX17048) schließt `build_src_filter` in `platformio.ini` beim Super Mini aus.
- Sicherheit zuerst: Totmann (400 ms), Stopp bei Verbindungsabbruch, Crash-Erkennung nicht aushebeln.
