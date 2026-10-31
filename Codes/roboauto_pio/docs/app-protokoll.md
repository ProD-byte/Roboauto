# 📱 Roboauto-App – Protokoll

Die Roboauto-App (Android) spricht mit dem Auto **über Bluetooth (BLE)** oder **über WLAN**.
In beiden Fällen sind es **dieselben Befehle wie bei der Weboberfläche** – die Firmware
behandelt sie an einer einzigen Stelle (`steuerBefehl()` in [`src/web.cpp`](../src/web.cpp)).

## WLAN

Ganz normale HTTP-Aufrufe an `http://roboauto.local` (oder die IP-Adresse), z. B.
`GET /fahr?x=3&y=7&t=0`. Siehe Liste unten.

## Bluetooth (BLE)

| | UUID | |
|---|---|---|
| Dienst | `8d3a1000-5b7e-4f2a-9c61-2e4b7a9d0c11` | |
| Befehl (App → Auto) | `8d3a1001-5b7e-4f2a-9c61-2e4b7a9d0c11` | Write / Write without response |
| Antwort (Auto → App) | `8d3a1002-5b7e-4f2a-9c61-2e4b7a9d0c11` | Notify |

Gerätename: **`Roboauto-XXXX`** (XXXX = Chip-Nummer; XIAO: `RoboautoXiao-XXXX`). Die App sollte nach dem Verbinden
eine **MTU von 517** anfordern, sonst werden die Antworten in viele kleine Stücke zerlegt.

### Anfrage

Text (UTF-8), ein Befehl pro Schreibvorgang:

```
<nr>|<pfad>?<parameter>
```

- `nr` = beliebige Zahl, die in der Antwort zurückkommt (damit die App Antworten zuordnen kann).
  **`0` = keine Antwort schicken** – für die schnellen Fahrbefehle.
- `pfad` und `parameter` genau wie bei HTTP, z. B. `0|/fahr?x=3&y=7&t=0` oder `12|/status`.

### Antwort

Als Notify auf der Antwort-Characteristic:

```
<nr>|<inhalt>\n
```

Lange Antworten kommen in mehreren Stücken (je MTU − 3 Byte). Die App hängt die Stücke
aneinander, bis ein `\n` kommt. Der Inhalt selbst enthält nie ein `\n`.

## Befehle

| Pfad | Parameter | Antwort | Wozu |
|---|---|---|---|
| `/fahr` | `x`, `y` (−7…7), `t` (Tasten-Bits: 1 Blinker links, 2 Blinker rechts, 4 Hupe, 8 Tankanzeige) | `ok` | Fahren, ca. alle 80 ms schicken |
| `/aktion` | `a` (Nummer, siehe [`include/web.h`](../include/web.h)) | `ok` | Licht, Modus, Figur, Musik … |
| `/status` | – | JSON | Live-Werte, ca. alle 250 ms |
| `/einstellungen` | keine = lesen; `name=wert` = ändern; `standard`, `motorStandard` | JSON | Einstellungen |
| `/verlauf` | – | JSON | Akkuverlauf |
| `/update` | `a=suchen` / `a=installieren` | `ok` | Auto-Update (nur mit WLAN) |
| `/info` | – | JSON | Name, IP-Adresse, WLAN da?, Firmware-Nummer, `udp` = UDP-Port |

Unbekannter Pfad → Antwort `?`.

## WLAN

Alles geht per HTTP wie im Browser (`http://<ip>/status` …). Nur die **Fahrbefehle** schickt die App
als **UDP-Paket** an den Port aus `/info` (`udp`, 4211) – gleiches Textformat wie bei Bluetooth,
z. B. `0|/fahr?x=3&y=7&t=0`. Das spart den Verbindungsaufbau von HTTP, das Auto reagiert schneller.
Den Stopp (`x=0&y=0&t=0`) schickt die App dreimal. Meldet `/info` keinen `udp`-Port (ältere Firmware),
fährt die App per HTTP.

### Ohne Router: WLAN des Autos

Das Auto öffnet immer sein eigenes, offenes WLAN **„Roboauto-XXXX“** (neben dem Heim-WLAN, gleicher Funkkanal
wie der Router). Im Einrichtungsmodus heißt es „Roboauto-Setup“. Das Auto ist dort unter **192.168.4.1**
erreichbar – gleiche HTTP- und UDP-Befehle wie oben. Ab Android 10 wechselt die App selbst in dieses WLAN
(nur für die App, das Handy behält sein Internet über Mobilfunk); bei Android 8/9 wählt man es in den
WLAN-Einstellungen.

## Sicherheit

- Kommt **400 ms lang kein `/fahr`**, bleibt das Auto stehen (Totmann).
- Wird die **Bluetooth-Verbindung getrennt**, stoppt das Auto sofort.
- WLAN-Einrichtung und Firmware-Upload bleiben nur über WLAN/Browser möglich.
