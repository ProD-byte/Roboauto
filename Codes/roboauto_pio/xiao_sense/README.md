# 🚗📷 Roboauto – Seeed Studio XIAO ESP32S3 Sense

Upgrade des Roboautos vom ESP32-S3 Super Mini auf den **XIAO ESP32S3 Sense**.
Alle bisherigen Funktionen bleiben erhalten (Dabble, Weboberfläche, Gyro, Lichter, Modi, Figuren,
Fahrweg, Balancieren, Jukebox, Auto-Update). Neu dazu kommen:

| | |
|---|---|
| 💾 **Doppelt Speicher** | 8 MB Flash (2 × 3,2 MB Programmplatz) + 8 MB PSRAM |
| 📷 **Live-Video** | Kamera (OV2640/OV3660) im Browser, 320×240 bis 800×600, Fotos auf die SD-Karte |
| 🎤 **Mikrofon** | Live-Ton im Browser, Lautstärkeanzeige, **2× klatschen = Licht an/aus** |
| 🧠 **Lernen** | schreibt beim Fahren Steuerung, Motoren, Gyro, Akku, Lautstärke **und Kamerabilder** auf die SD-Karte |
| 🎬 **Lange Fahrwege** | mit SD-Karte bis 10 Minuten statt 2 |
| 💡 **Ein LED-Pin** | Ring, Front- und Rücklicht hängen hintereinander an einem Pin |
| 🔋 **Akkuwächter** | MAX17048 misst den Ladestand in % (genauer als der alte Spannungsteiler) |

## 🔌 Pinbelegung

| XIAO | GPIO | Anschluss | Funktion |
|---|---|---|---|
| D0 | 1 | Motortreiber IN1 | Motoren links vorwärts (PWM) |
| D1 | 2 | Motortreiber IN2 | Motoren links rückwärts (PWM) |
| D2 | 3 | Motortreiber IN3 | Motoren rechts vorwärts (PWM) |
| D3 | 4 | Motortreiber IN4 | Motoren rechts rückwärts (PWM) |
| D4 | 5 | MPU6050 SDA **+** MAX17048 SDA | I²C |
| D5 | 6 | MPU6050 SCL **+** MAX17048 SCL | I²C |
| D6 | 43 | Buzzer + | passiver Buzzer |
| D7 | 44 | LED-Ring **DIN** | LED-Kette (12 × WS2812B) |
| D8–D10 | 7–9 | – | **belegt von der SD-Karte** (nichts anschließen!) |
| intern | | Kamera, Mikrofon, SD-Karte | auf dem Sense-Aufsteckboard |
| BAT+/BAT− (Unterseite) | | Akku (nach Schalter), MAX17048 CELL | der XIAO lädt den Akku über USB-C selbst |
| 5V | | LED-Kette (+) | Versorgung der LEDs |
| 3V3 | | MPU6050 VCC, MAX17048 VDD (falls das Board es braucht) | |

Alle Pins stehen in [`include/config.h`](include/config.h).

### LED-Kette (ein Pin für alle LEDs)

```
XIAO D7 ──► Ring DIN   Ring DOUT ──► Front links DIN ─► Front rechts ─► Heck links ─► Heck rechts
            (LED 0–7)                (LED 8)            (LED 9)         (LED 10)       (LED 11)
```

Reihenfolge in der Kette: **0–7 Ring, 8–9 Frontlicht, 10–11 Rücklicht**. Ist die Reihenfolge links/rechts
vertauscht, `FRONT_LED_LINKS` bzw. `REAR_LED_LINKS` in `config.h` auf `1` stellen. Ring und Lichter
haben trotzdem weiter getrennte Helligkeitsregler.

> Tipp: einen 330-Ω-Widerstand in die Datenleitung vor den Ring und einen 470–1000-µF-Elko an 5V/GND der LEDs.

### Warum ein Akkuwächter?
Mit SD-Karte bleiben am XIAO nur 8 freie Pins (D0–D7). Gebraucht werden 4 × Motor, LED, Buzzer, SDA, SCL – und
die Akkumessung. Der **MAX17048** hängt einfach mit am I²C-Bus vom Gyro und braucht keinen eigenen Pin.

## 🌐 Weboberfläche

Wie gewohnt **http://roboauto.local**. Neu ist der Bereich **Kamera** mit Live-Video, Ton, Foto,
Klatschsteuerung, Lernen und SD-Karten-Übersicht.

Video, Ton und Dateien laufen über einen eigenen Medien-Server auf **Port 81**. So bremst ein Live-Video die
Steuerung nicht aus:

| Adresse | |
|---|---|
| `http://roboauto.local:81/stream` | Live-Video (MJPEG) |
| `http://roboauto.local:81/bild` | ein einzelnes Foto |
| `http://roboauto.local:81/ton` | Live-Ton (WAV, immer nur ein Zuhörer) |
| `http://roboauto.local:81/dateien?pfad=/lernen` | Inhalt der SD-Karte (JSON) |
| `http://roboauto.local:81/datei?pfad=/fotos/foto_0001.jpg` | Datei herunterladen (`&weg=1` = löschen) |

## 🧠 Lernen – Daten zum Trainieren sammeln

Knopf **🧠 Lernen** auf der Webseite → das Auto schreibt jede Fahrt in einen eigenen Ordner:

```
/lernen/fahrt_0001/daten.csv        10× pro Sekunde
/lernen/fahrt_0001/bild_00001.jpg   4× pro Sekunde
```

`daten.csv` hat die Spalten
`zeit_ms, bild, x, y, links, rechts, drehrate, winkel, nicken, rollen, akku, pegel, modus, licht`.
Die Spalte `bild` sagt, welches Kamerabild zu dieser Zeile gehört. So kann am PC ein Modell lernen:
„Bei diesem Bild hat der Fahrer so gelenkt.“ (Behavior Cloning). Die SD-Karte muss **FAT32** formatiert sein.

## ⚡ Firmware aufspielen

1. In VS Code den Ordner **`xiao_sense`** öffnen (es ist ein eigenes PlatformIO-Projekt).
2. `geheim.beispiel.ini` → `geheim.ini`, `include/wlan_zugang.beispiel.h` → `include/wlan_zugang.h`.
3. Umgebung **`env:roboauto_xiao`** → Upload (→). Klappt der Upload nicht: **BOOT** gedrückt halten, USB einstecken, loslassen.
4. Später ohne Kabel: **`env:roboauto_xiao_wlan`**.

### Ohne Programmieren – Online-Flasher
Auf der GitHub-Pages-Seite gibt es eine eigene Karte **„Roboauto mit XIAO ESP32S3 Sense“** (Update oder Neuinstallation).

### Automatischer Build und Auto-Update
GitHub Actions baut bei jedem Hochladen beide Autos in **zwei unabhängigen Jobs** (gleichzeitig, mit derselben
Build-Nummer). Scheitert einer, wird das andere Auto trotzdem veröffentlicht. Für das gescheiterte bleiben die
bisherigen Dateien online. Die XIAO-Dateien landen
auf GitHub Pages im Unterordner **`/xiao/`** (`version.json`, `roboauto-xiao-update.bin`, `roboauto-xiao-komplett.bin`).
Der XIAO schaut nur dort nach Updates, der Super Mini nur im Hauptordner. So lädt keines der beiden Autos
die falsche Firmware. Bei einem Versions-Tag (`v1.2.0`) hängen die XIAO-Dateien zusätzlich am Release.

Der Bluetooth-Name in Dabble ist jetzt **„RoboautoXiao“**.
