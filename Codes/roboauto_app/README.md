# 📱 Roboauto-App (Android)

Eigene App statt Dabble. Sie zeigt **genau die Weboberfläche des Autos** und verbindet sich
**per Bluetooth** (ohne WLAN, auch draußen) oder **per WLAN**.

- Android 8 und neuer
- Die Oberfläche kommt beim Bauen direkt aus der Firmware ([`include/webseite.h`](../roboauto_pio/include/webseite.h)):
  ändert sich die Webseite, ändert sich die App automatisch mit.
- Protokoll zwischen App und Auto: [`docs/app-protokoll.md`](../roboauto_pio/docs/app-protokoll.md)

> ⚠️ Zum Testen braucht das Auto die Firmware **`env:roboauto_bletest`** (ohne Dabble).
> Mit der normalen Dabble-Firmware findet die App das Auto nur über WLAN.

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

GitHub baut die App bei jedem Hochladen: **Actions → „App bauen“ → Lauf anklicken → Artifacts → `roboauto-app`**.
Darin liegt `roboauto.apk` – aufs Handy kopieren und installieren („Unbekannte Quellen“ erlauben).
Bei einem Versions-Tag (`v1.2.0`) kommt die APK zusätzlich ins Release.

## Signierschlüssel (einmalig einrichten)

Android installiert ein Update nur, wenn es mit **demselben Schlüssel** signiert ist wie die installierte App.
Der Schlüssel liegt lokal in `schluessel/` (wird **nicht** hochgeladen, siehe `.gitignore`).
Auf GitHub unter **Settings → Secrets and variables → Actions → New repository secret** anlegen:

| Name | Inhalt |
|---|---|
| `ROBOAUTO_KEYSTORE_BASE64` | Inhalt von `schluessel/ROBOAUTO_KEYSTORE_BASE64.txt` |
| `ROBOAUTO_KEYSTORE_PASSWORT` | Inhalt von `schluessel/ROBOAUTO_KEYSTORE_PASSWORT.txt` |

🔒 Den Ordner `schluessel/` gut aufheben (z. B. Passwort-Manager). Geht er verloren, muss die App
einmal deinstalliert und neu installiert werden.

## Am PC bauen (optional)

Ordner `Codes/roboauto_app` in **Android Studio** öffnen → Run. Android Studio richtet Gradle selbst ein.
