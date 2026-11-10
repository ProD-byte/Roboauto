# 🚗 Roboauto

Ein kleines vierrädriges Roboterauto (gelenkt über unterschiedliches Tempo links und rechts) mit dem **Seeed Studio XIAO ESP32S3 Sense** – mit Kamera, Mikrofon und SD-Karte – gesteuert per eigener **Roboauto-App** (Android, Bluetooth oder WLAN) oder im **Browser über WLAN**.
Geschrieben mit **VS Code + PlatformIO**, Schritt für Schritt aufgebaut.

## ✨ Funktionen

| | |
|---|---|
| 🎮 **Steuerung** | [Roboauto-App](../roboauto_app/README.md) für Android (Bluetooth, Heim-WLAN oder direkt ins WLAN „Roboauto-XXXX“ des Autos, auch Lenken durch Kippen des Handys) oder Browser mit Joystick und Tastatur (W A S D) |
| 🌐 **Weboberfläche** | modernes Dashboard: Live-Ansicht des Autos, Kompass, Neigung, Motorleistung, Akkuverlauf · Handy quer = Gamepad-Ansicht · Deutsch/Englisch |
| 🧭 **Gyro (MPU6050)** | Geradeauslauf, 180°-Wende auf Knopfdruck, Kompass, Crash- und Umkipp-Erkennung |
| 💡 **Licht** | Front-/Rücklicht, Fernlicht, Blinker, Warnblinker, Bremslicht, LED-Ring mit Animationen |
| 🚨 **Modi** | Normal, Polizei (Blaulicht + Sirene), Team Blau/Grün/Gelb/Rot |
| ✨ **Programme** | Figuren (Quadrat, Acht, Slalom), Fahrweg aufnehmen und abspielen, Balancieren (Experiment) |
| 🎵 **Jukebox** | Chiptune-Musik über den Buzzer mit Lichtshow auf dem LED-Ring |
| 📷 **Kamera** | Live-Video in App und Browser, Fahren mit Video im Querformat, Fotos und Videos mit Ton auf die SD-Karte, Autofokus (OV5640) |
| 🎤 **Mikrofon** | Live-Ton, Lautstärkeanzeige, 2× klatschen = Licht an/aus |
| 📶 **WLAN** | Einrichtung per Handy (Netz „Roboauto-Setup“), merkt sich bis zu 8 Netzwerke, Updates über WLAN (OTA) |
| 🔧 **Einstellungen** | Tempo, Lenkung, Helligkeit, Lautstärke, Töne, Motor-Richtung mit Motortest-Assistent |

## 🔌 Hardware

- Seeed Studio XIAO ESP32S3 Sense (8 MB Flash, 8 MB PSRAM) mit Kamera OV5640, Mikrofon und microSD-Steckplatz
- 4 × N20-Getriebemotor 3 V, 500 U/min, Motortreiber DRV8833
- MPU6050 (Gyro + Beschleunigung) und Akkuwächter MAX17048/MAX17043 am I²C-Bus
- WS2812-LED-Ring (8 LEDs) + 2 Front- und 2 Rücklicht-LEDs, alle an einem Pin
- Passiver Buzzer
- 1S-LiPo 3,7 V mit Ein/Aus-Schalter, geladen über die USB-C-Buchse des XIAO

Alle Pins stehen in [`include/config.h`](include/config.h), die ausführliche Anleitung in [`docs/xiao-sense.md`](docs/xiao-sense.md).

| XIAO | GPIO | Anschluss an | Funktion |
|---|---|---|---|
| D0 | 1 | Motortreiber IN1 | Motoren links vorwärts (PWM) |
| D1 | 2 | Motortreiber IN2 | Motoren links rückwärts (PWM) |
| D2 | 3 | Motortreiber IN3 | Motoren rechts vorwärts (PWM) |
| D3 | 4 | Motortreiber IN4 | Motoren rechts rückwärts (PWM) |
| D4 | 5 | MPU6050 SDA + Akkuwächter SDA | I²C |
| D5 | 6 | MPU6050 SCL + Akkuwächter SCL | I²C |
| D6 | 43 | Buzzer + | Ton und Lautstärke per PWM |
| D7 | 44 | LED-Ring DIN | alle 12 LEDs: Ring → Front → Heck |
| D8–D10 | 7–9 | – | belegt von der SD-Karte (nichts anschließen) |
| BAT+ / BAT− | | Akku über den Schalter, LED-Kette +, Motortreiber VCC | Versorgung, Laden über USB-C |
| 3V3 | | MPU6050 VCC | |

Die Motoren einer Seite hängen parallel am selben Ausgang. Die LEDs bekommen ihren Strom aus dem
Akku (BAT+ hinter dem Schalter) – nicht vom 5V-Pin, der nur mit USB Spannung hat. Alle Massen sind
verbunden. Sind die Motoren falsch herum angeschlossen, hilft der Motortest-Assistent in der Weboberfläche.

## ⚡ Firmware aufspielen

### Ohne Programmieren – Online-Flasher
Auf der **GitHub-Pages-Seite** dieses Projekts auf „Update installieren“ bzw. „Löschen & neu installieren“ klicken (Chrome/Edge am PC, Auto per USB-C anschließen).

### Mit VS Code + PlatformIO
1. Repository klonen und den Ordner in VS Code öffnen (PlatformIO-Erweiterung installiert).
2. Vorlagen kopieren:
   - `geheim.beispiel.ini` → **`geheim.ini`** (Passwort für WLAN-Updates eintragen oder leer lassen)
   - `include/wlan_zugang.beispiel.h` → **`include/wlan_zugang.h`** (kann leer bleiben)
3. Umgebung **`env:roboauto_xiao`** wählen → Upload (→). Findet PlatformIO den Anschluss nicht, in `platformio.ini` `upload_port` eintragen.
4. Spätere Updates ohne Kabel: Umgebung **`env:roboauto_xiao_wlan`** → Upload.

> 🔒 `geheim.ini` und `include/wlan_zugang.h` werden **nicht** hochgeladen (siehe `.gitignore`).

## 📱 Erste Schritte

1. Auto einschalten → der LED-Ring leuchtet orange.
2. Mit dem Handy ins WLAN **„Roboauto-Setup“** → die Einrichtungsseite öffnet sich von selbst.
3. Eigenes WLAN wählen, Passwort eingeben, speichern.
4. Im normalen WLAN **http://roboauto.local** öffnen – fertig.

Oder mit der **[Roboauto-App](../roboauto_app/README.md)** (Android): App öffnen – sie findet das Auto
von selbst per Bluetooth und im WLAN. Ohne Router geht es direkt über das WLAN **„Roboauto-XXXX“** des Autos.

## 🤖 Automatischer Build (GitHub Actions)

Bei jedem Hochladen baut GitHub die Firmware automatisch:

- **Actions** → Lauf anklicken → **Artifacts**: `roboauto-xiao-komplett.bin` (Neuinstallation) und `roboauto-xiao-update.bin` (Update), dazu die App `roboauto.apk`
- Ein **Tag** wie `v1.0.0` erstellt ein **Release** mit beiden Dateien.
- Der **Online-Flasher** (GitHub Pages) bekommt automatisch die neueste Version.

## 📚 Verwendete Bibliotheken

- [Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel)
- [OV5640 Auto Focus for ESP32 Camera](https://github.com/0015/ESP32-OV5640-AF) für den Autofokus
- [ESP Web Tools](https://esphome.github.io/esp-web-tools/) für den Online-Flasher

Die Musikstücke der Jukebox sind gemeinfrei (Volkslied/Klassik) oder eigene Kompositionen.
Zusätzlich enthalten (Lizenz **CC0**, aus MIDI umgerechnet):
- „15 Melodic RPG Chiptunes“ von **Aureolus Omicron** – [OpenGameArt](https://opengameart.org/content/15-melodic-rpg-chiptunes) (Title Screen, Airship, Battle 2, Town)
- „Theme Song [8-bit]“ und „Leap 8-bit“ von **nene** – [OpenGameArt](https://opengameart.org/content/theme-song-8-bit), [OpenGameArt](https://opengameart.org/content/leap-8bit)
- „Long Away Home [8bit]“, „Boss Battle #2“ und „Boss Battle #5“ von **nene** (CC0) – [OpenGameArt](https://opengameart.org/content/long-away-home-8bit), [OpenGameArt](https://opengameart.org/content/boss-battle-2-8-bit), [OpenGameArt](https://opengameart.org/content/boss-battle-5-8-bit)
- „JRPG Collection“ (Main Theme, Battle) von **Yubatake** – Lizenz [CC-BY 4.0](https://creativecommons.org/licenses/by/4.0/), [OpenGameArt](https://opengameart.org/content/jrpg-collection); für den Buzzer umgerechnet und gekürzt
- „4 Music Box Tracks“ (Sad Tune, Spooky Waltz) und „Bittersweet Story“ von **Aureolus Omicron** (CC0) – [OpenGameArt](https://opengameart.org/content/4-music-box-tracks)
- „Princess“ und „Lullaby“ von **Yubatake** – [CC-BY 4.0](https://creativecommons.org/licenses/by/4.0/), [OpenGameArt](https://opengameart.org/content/lullaby-1)
- „Destiny Formation“ von **Bobjt** (CC0) – [OpenGameArt](https://opengameart.org/content/destiny-formation)
- „Weiße Taube“ – © ProDMusicKI (eigener Song)
