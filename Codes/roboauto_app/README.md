# 📱 Roboauto-App (Android)

Die App zum Roboauto (ersetzt die frühere Dabble-App). Sie zeigt **genau die Weboberfläche des Autos** und verbindet sich
**per Bluetooth** (ohne WLAN, auch draußen) oder **per WLAN**.

- Android 8 und neuer
- Die Oberfläche kommt beim Bauen direkt aus der Firmware ([`include/webseite.h`](../roboauto_pio/include/webseite.h)):
  ändert sich die Webseite, ändert sich die App automatisch mit.
- Protokoll zwischen App und Auto: [`docs/app-protokoll.md`](../roboauto_pio/docs/app-protokoll.md)

- Drei Wege zum Auto: **Bluetooth**, **Heim-WLAN** oder direkt das WLAN **„Roboauto-XXXX“** des Autos (ohne Router)
- **📱 Kippen:** Knopf unter dem Joystick. Dann den Finger auf dem Feld halten und das Handy kippen
  (nach vorn = Gas, zur Seite = lenken). Wie das Handy beim Antippen gehalten wird, ist der Nullpunkt.
  Loslassen = Stopp. Gibt es nur in der App, nicht im Browser.

> Ältere Autos mit Dabble-Firmware bekommen die App-Firmware per Auto-Update (oder einmal per Online-Flasher).

## So funktioniert's

| Datei | Wozu |
|---|---|
| `app/web/verbinden.html` | Startbildschirm: sucht Autos per Bluetooth und WLAN, verbindet |
| `app/web/app.js` | leitet `fetch('/status')` usw. der Weboberfläche an die App weiter |
| `MainActivity.kt` | WebView-Hülle, Erlaubnisse, Brücke zwischen Seite und Verbindung |
| `BleVerbindung.kt` | Bluetooth: suchen, verbinden, Befehle schicken, Antworten zusammensetzen |
| `WlanVerbindung.kt` | WLAN: HTTP-Aufrufe ans Auto, Auto im Netz finden (mDNS) |

Das zuletzt benutzte Auto verbindet sich beim nächsten Start automatisch. Die Pille
**Bluetooth ⇄** bzw. **WLAN ⇄** oben in der Kopfleiste (oder die Zurück-Taste) führt zurück zur Auswahl.

## APK bekommen

Am einfachsten: auf der **GitHub-Pages-Seite** des Projekts (dort, wo auch der Online-Flasher ist)
auf **„App herunterladen“** tippen – am PC gibt es dort einen QR-Code zum Abscannen.

GitHub baut die App zusammen mit der Firmware (Workflow „Firmware bauen“, Job `app`) und legt sie
als `roboauto.apk` auf die Seite. Bei einem Versions-Tag (`v1.2.0`) kommt sie zusätzlich ins Release.

**Updates:** Die App schaut beim Start in `app.json` nach einer neueren Version und zeigt dann eine
grüne Update-Karte (bzw. eine Pille in der Steuerung). Antippen lädt und installiert die neue Version
(`Aktualisierung.kt`). Klappt nur, wenn alle Versionen mit demselben Schlüssel signiert sind (Secrets unten).
Ab Android 12 läuft es nach dem ersten Update ohne Nachfrage durch.

## Signierschlüssel (einmalig einrichten)

Android installiert ein Update nur, wenn es mit **demselben Schlüssel** signiert ist wie die installierte App.
Der Schlüssel liegt lokal in `schluessel/` (wird **nicht** hochgeladen, siehe `.gitignore`).
Auf GitHub unter **Settings → Secrets and variables → Actions → New repository secret** anlegen:

| Name | Inhalt |
|---|---|
| `ROBOAUTO_KEYSTORE_BASE64` | Inhalt von `schluessel/ROBOAUTO_KEYSTORE_BASE64.txt` |
| `ROBOAUTO_KEYSTORE_PASSWORT` | Inhalt von `schluessel/ROBOAUTO_KEYSTORE_PASSWORT.txt` |

✅ Eingerichtet seit Build 1.0.39 – ab dieser Version klappen die App-Updates ohne Neuinstallation.

🔒 Den Ordner `schluessel/` gut aufheben (z. B. Passwort-Manager). Geht er verloren, muss die App
einmal deinstalliert und neu installiert werden.

## Am PC bauen (optional)

Ordner `Codes/roboauto_app` in **Android Studio** öffnen → Run. Android Studio richtet Gradle selbst ein.
