package de.roboauto.app

import android.Manifest
import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.content.Intent
import android.content.pm.ActivityInfo
import android.content.pm.PackageManager
import android.graphics.Color
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import android.location.LocationManager
import android.net.ConnectivityManager
import android.net.Network
import android.net.NetworkCapabilities
import android.net.NetworkRequest
import android.net.Uri
import android.net.wifi.WifiNetworkSpecifier
import android.os.PatternMatcher
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.provider.Settings
import android.view.Surface
import android.view.WindowManager
import android.webkit.JavascriptInterface
import android.webkit.WebChromeClient
import android.webkit.WebSettings
import android.webkit.WebResourceRequest
import android.webkit.WebResourceResponse
import android.webkit.WebView
import android.webkit.WebViewClient
import android.widget.FrameLayout
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.OnBackPressedCallback
import androidx.activity.SystemBarStyle
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import androidx.webkit.WebViewAssetLoader
import org.json.JSONObject

/**
 * Die App ist eine Huelle um die Weboberflaeche des Autos (assets/index.html, kommt beim
 * Bauen aus include/webseite.h). Die Webseite fragt wie im Browser fetch('/status') usw. -
 * app.js leitet das hierher um, und die App schickt es per Bluetooth oder WLAN ans Auto.
 */
class MainActivity : ComponentActivity(), BleVerbindung.Ereignisse, WlanVerbindung.Ereignisse, Aktualisierung.Ereignisse {

    companion object {
        private const val SEITEN = "https://appassets.androidplatform.net/assets/"
        private const val NR_INFO = -1                 // eigene Anfrage nach /info (IP-Adresse merken)
        private const val AUTO_WLAN = "Roboauto"       // eigenes WLAN des Autos (auch "Roboauto-Setup")
        private const val AUTO_WLAN_IP = "192.168.4.1"
    }

    private lateinit var web: WebView
    private lateinit var ble: BleVerbindung
    private lateinit var wlan: WlanVerbindung
    private lateinit var aktualisierung: Aktualisierung
    @Volatile private var updateInfo = ""   // "1.0.42|1912311" = neuere App-Version gefunden
    private val haupt = Handler(Looper.getMainLooper())
    private val speicher by lazy { getSharedPreferences("roboauto", MODE_PRIVATE) }

    private var art = ""                 // "ble", "wlan" oder "" (Verbinden-Bildschirm)
    private var cockpitAn = false        // "Fahren mit Video": quer, ohne Status- und Navigationsleiste
    private var autoVerbinden = true     // bekanntes Auto beim Finden gleich verbinden (nicht nach "Trennen")
    private var autoWlanNetz: ConnectivityManager.NetworkCallback? = null   // direkt im WLAN des Autos (Android 10+)
    private var autoWlanWarten = false   // Android 8/9: Nutzer waehlt "Roboauto" in den WLAN-Einstellungen
    private val verbindung by lazy { getSystemService(CONNECTIVITY_SERVICE) as ConnectivityManager }

    private val erlaubnis = registerForActivityResult(ActivityResultContracts.RequestMultiplePermissions()) { r ->
        if (r.values.all { it }) suchen() else zustand("erlaubnis")
    }

    private fun noetigeErlaubnisse(): Array<String> =
        if (Build.VERSION.SDK_INT >= 31) arrayOf(Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_CONNECT)
        else arrayOf(Manifest.permission.ACCESS_FINE_LOCATION)

    private fun hatErlaubnis() =
        noetigeErlaubnisse().all { checkSelfPermission(it) == PackageManager.PERMISSION_GRANTED }

    @SuppressLint("SetJavaScriptEnabled")
    override fun onCreate(savedInstanceState: Bundle?) {
        enableEdgeToEdge(SystemBarStyle.dark(Color.TRANSPARENT), SystemBarStyle.dark(Color.TRANSPARENT))
        super.onCreate(savedInstanceState)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)   // beim Fahren nicht ausgehen

        ble = BleVerbindung(this, this)
        wlan = WlanVerbindung(this, this)
        aktualisierung = Aktualisierung(this, this)
        if (BuildConfig.SELBST_UPDATE) aktualisierung.pruefen()   // gibt es eine neuere App auf GitHub? (nicht im Play Store)

        // Rahmen haelt Abstand zu Statusleiste/Kamera-Loch, die Webseite selbst bleibt unveraendert
        val rahmen = FrameLayout(this)
        rahmen.setBackgroundColor(0xFF0D0F13.toInt())
        web = WebView(this)
        web.setBackgroundColor(0xFF0D0F13.toInt())
        rahmen.addView(web, FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT))
        setContentView(rahmen)
        ViewCompat.setOnApplyWindowInsetsListener(rahmen) { v, insets ->
            val i = insets.getInsets(WindowInsetsCompat.Type.systemBars() or WindowInsetsCompat.Type.displayCutout())
            v.setPadding(i.left, i.top, i.right, i.bottom)
            WindowInsetsCompat.CONSUMED
        }

        val lader = WebViewAssetLoader.Builder()
            .addPathHandler("/assets/", WebViewAssetLoader.AssetsPathHandler(this))
            .build()
        web.settings.javaScriptEnabled = true
        web.settings.domStorageEnabled = true          // Sprache wird im localStorage gemerkt
        // XIAO: Video, Ton und SD-Dateien holt die App selbst und reicht sie unter .../medien/... durch
        // (ein direkter http-Zugriff der https-Seite ins Heimnetz wird von der WebView blockiert -> Video flackerte nur)
        web.settings.mixedContentMode = WebSettings.MIXED_CONTENT_ALWAYS_ALLOW
        web.webViewClient = object : WebViewClient() {
            override fun shouldInterceptRequest(view: WebView, request: WebResourceRequest): WebResourceResponse? {
                val url = request.url
                val pfad = url.encodedPath ?: ""
                if (url.host == "appassets.androidplatform.net" && pfad.startsWith("/medien/"))
                    return wlan.medien(pfad.removePrefix("/medien") + (url.encodedQuery?.let { "?$it" } ?: ""))
                return lader.shouldInterceptRequest(url)
            }

            override fun shouldOverrideUrlLoading(view: WebView, request: WebResourceRequest): Boolean {
                val url = request.url
                if (url.toString().startsWith(SEITEN)) return false
                if (url.host == "appassets.androidplatform.net" && url.path == "/wlan") { wlanEinrichten(); return true }
                startActivity(Intent(Intent.ACTION_VIEW, url))   // fremde Links im Browser oeffnen
                return true
            }
        }
        // Ohne WebChromeClient zeigt die WebView keine alert()/confirm()-Fenster
        // (z. B. "Einstellungen auf Standard zuruecksetzen?" - confirm() waere sonst immer "Nein")
        web.webChromeClient = WebChromeClient()
        web.addJavascriptInterface(Bruecke(), "Roboauto")

        onBackPressedDispatcher.addCallback(this, object : OnBackPressedCallback(true) {
            override fun handleOnBackPressed() {
                if (cockpitAn) web.evaluateJavascript("window.cockpitAus && cockpitAus()", null)   // erst das Cockpit verlassen
                else if (art.isNotEmpty()) zumVerbinden(manuell = true) else finish()
            }
        })

        web.loadUrl(SEITEN + "verbinden.html")
    }

    override fun onResume() {
        super.onResume()
        aktualisierung.weiterNachErlaubnis()          // zurueck aus "Apps aus dieser Quelle erlauben"
        if (autoWlanWarten) {                         // zurueck aus den WLAN-Einstellungen
            autoWlanWarten = false
            wlanVerbinden(AUTO_WLAN_IP)
        }
        kippSensor?.let { sensoren.registerListener(kippHoerer, it, SensorManager.SENSOR_DELAY_GAME) }
    }

    override fun onPause() {
        sensoren.unregisterListener(kippHoerer)
        super.onPause()
    }

    // ---------------- Kipp-Lenkung ----------------
    // Schwerkraft in Bildschirm-Achsen (x nach rechts, y nach oben, z aus dem Bildschirm),
    // auch im Querformat. Die Seite rechnet daraus die Winkel.
    private val sensoren by lazy { getSystemService(SENSOR_SERVICE) as SensorManager }
    private val kippSensor: Sensor? by lazy {
        sensoren.getDefaultSensor(Sensor.TYPE_GRAVITY) ?: sensoren.getDefaultSensor(Sensor.TYPE_ACCELEROMETER)
    }
    @Volatile private var schwere = floatArrayOf(0f, 0f, 0f)
    private val kippHoerer = object : SensorEventListener {
        override fun onSensorChanged(e: SensorEvent) {
            val (x, y, z) = e.values
            val drehung = if (Build.VERSION.SDK_INT >= 30) display?.rotation ?: 0
                          else @Suppress("DEPRECATION") windowManager.defaultDisplay.rotation
            val (sx, sy) = when (drehung) {
                Surface.ROTATION_90 -> -y to x
                Surface.ROTATION_180 -> -x to -y
                Surface.ROTATION_270 -> y to -x
                else -> x to y
            }
            // Beschleunigungssensor (ohne Schwerkraft-Sensor) wackelt -> glaetten
            val f = if (e.sensor.type == Sensor.TYPE_GRAVITY) 1f else 0.2f
            val a = schwere
            schwere = floatArrayOf(a[0] + (sx - a[0]) * f, a[1] + (sy - a[1]) * f, a[2] + (z - a[2]) * f)
        }
        override fun onAccuracyChanged(s: Sensor, genauigkeit: Int) {}
    }

    override fun onDestroy() {
        ble.trennen()
        ble.suchStopp()
        autoWlanLoslassen()
        wlan.beenden()
        aktualisierung.beenden()
        web.destroy()
        super.onDestroy()
    }

    // ---------------- Bruecke fuer JavaScript (window.Roboauto) ----------------
    // Wird auf einem eigenen Thread aufgerufen -> alles auf den Haupt-Thread schieben
    inner class Bruecke {
        @JavascriptInterface fun suchen() { haupt.post { this@MainActivity.suchen() } }
        @JavascriptInterface fun verbindeBle(adresse: String) { haupt.post { bleVerbinden(adresse) } }
        @JavascriptInterface fun verbindeWlan(host: String) { haupt.post { wlanVerbinden(host.trim()) } }
        @JavascriptInterface fun verbindeAutoWlan() { haupt.post { autoWlanVerbinden() } }
        @JavascriptInterface fun updateInfo(): String = updateInfo
        @JavascriptInterface fun version(): String =
            try { packageManager.getPackageInfo(packageName, 0).versionName ?: "" } catch (_: Exception) { "" }
        @JavascriptInterface fun updateInstallieren() { haupt.post { aktualisierung.installieren() } }
        @JavascriptInterface fun trennen() { haupt.post { zumVerbinden(manuell = true) } }
        @JavascriptInterface fun kippDa(): Boolean = kippSensor != null
        @JavascriptInterface fun cockpit(an: Boolean) { haupt.post { this@MainActivity.cockpit(an) } }
        @JavascriptInterface fun kipp(): String = schwere.let { "${it[0]},${it[1]},${it[2]}" }
        @JavascriptInterface fun art(): String = art
        @JavascriptInterface fun host(): String = wlan.host ?: ""   // Adresse des Autos im WLAN ("" = Bluetooth)
        @JavascriptInterface fun letzteAdresse(): String = speicher.getString("wlan", null) ?: speicher.getString("ip", null) ?: "roboauto.local"
        @JavascriptInterface fun wlanEinrichten() { haupt.post { this@MainActivity.wlanEinrichten() } }
        @JavascriptInterface fun btEinschalten() { haupt.post { oeffne(Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE)) } }
        @JavascriptInterface fun standortEinschalten() { haupt.post { oeffne(Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS)) } }
        @JavascriptInterface fun appEinstellungen() {
            haupt.post { oeffne(Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS, Uri.parse("package:$packageName"))) }
        }

        @JavascriptInterface fun anfrage(nr: Int, pfad: String) {
            haupt.post {
                when (art) {
                    "ble" -> if (!ble.anfrage(nr, pfad)) antwortAnSeite(nr, false, "nicht verbunden")
                    "wlan" -> wlan.anfrage(nr, pfad)
                    else -> antwortAnSeite(nr, false, "nicht verbunden")
                }
            }
        }

        @JavascriptInterface fun senden(pfad: String) {
            haupt.post {
                when (art) {
                    "ble" -> ble.senden(pfad)
                    "wlan" -> wlan.senden(pfad)
                }
            }
        }
    }

    private fun oeffne(i: Intent) {
        try { startActivity(i) } catch (_: Exception) {}
    }

    // ---------------- Verbinden-Bildschirm ----------------
    private fun suchen() {
        if (art.isEmpty()) ble.trennen()                           // laufende Neuversuche abbrechen
        wlan.suchen()
        if (!hatErlaubnis()) { erlaubnis.launch(noetigeErlaubnisse()); return }
        val a = ble.adapter
        if (a == null) { zustand("keinbt"); return }
        if (!a.isEnabled) { zustand("btaus"); return }
        if (Build.VERSION.SDK_INT < 31 && Build.VERSION.SDK_INT >= 28 &&
            !(getSystemService(LOCATION_SERVICE) as LocationManager).isLocationEnabled) {
            zustand("standort"); return                            // bis Android 11 findet die Suche sonst nichts
        }
        zustand("sucht")
        ble.suchen()
    }

    private fun bleVerbinden(adresse: String) {
        if (!hatErlaubnis()) { suchen(); return }
        wlan.host = null
        zustand("verbinde", adresse)
        ble.verbinden(adresse)
    }

    private fun wlanVerbinden(host: String) {
        if (host.isEmpty()) return
        zustand("verbinde_wlan", host)
        wlan.pruefen(host) { info ->
            if (info == null) { zustand("wlan_fehler", host); return@pruefen }
            ble.trennen()
            ble.suchStopp()
            wlan.suchStopp()
            wlan.host = host
            wlan.udpPort = try { JSONObject(info).optInt("udp") } catch (_: Exception) { 0 }
            if (host != AUTO_WLAN_IP) speicher.edit().putString("wlan", host).apply()
            art = "wlan"
            steuerungZeigen(info)
        }
    }

    // "Fahren mit Video": Handy fest quer (beide Richtungen), Status- und Navigationsleiste weg
    // (Wischen vom Rand holt sie kurz zurueck). Aus = wieder wie vorher, das Handy dreht frei.
    private fun cockpit(an: Boolean) {
        cockpitAn = an
        requestedOrientation = if (an) ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE else ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED
        val leisten = WindowCompat.getInsetsController(window, window.decorView)
        if (an) {
            leisten.systemBarsBehavior = WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
            leisten.hide(WindowInsetsCompat.Type.systemBars())
        } else {
            leisten.show(WindowInsetsCompat.Type.systemBars())
        }
    }

    private fun zumVerbinden(manuell: Boolean) {
        if (cockpitAn) cockpit(false)
        if (manuell) autoVerbinden = false
        art = ""
        ble.trennen()
        wlan.host = null
        autoWlanLoslassen()
        web.loadUrl(SEITEN + "verbinden.html")
    }

    // Direkt ins WLAN des Autos (ohne Router). Ab Android 10 fragt das Handy selbst nach,
    // vorher waehlt man "Roboauto" in den WLAN-Einstellungen und kommt zurueck.
    private fun autoWlanVerbinden() {
        if (Build.VERSION.SDK_INT < 29) {
            autoWlanWarten = true
            zustand("auto_wlan_hand")
            oeffne(Intent(Settings.ACTION_WIFI_SETTINGS))
            return
        }
        autoWlanLoslassen()
        zustand("verbinde_wlan", AUTO_WLAN)
        val anfrage = NetworkRequest.Builder()
            .addTransportType(NetworkCapabilities.TRANSPORT_WIFI)
            .removeCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
            .setNetworkSpecifier(WifiNetworkSpecifier.Builder()
                .setSsidPattern(PatternMatcher(AUTO_WLAN, PatternMatcher.PATTERN_PREFIX)).build())
            .build()
        val r = object : ConnectivityManager.NetworkCallback() {
            override fun onAvailable(netz: Network) {
                verbindung.bindProcessToNetwork(netz)       // alle Anfragen der App gehen jetzt ueber das Auto-WLAN
                haupt.post { if (autoWlanNetz === this) wlanVerbinden(AUTO_WLAN_IP) }
            }
            override fun onUnavailable() {
                haupt.post { if (autoWlanNetz === this) { autoWlanNetz = null; zustand("auto_wlan_fehler") } }
            }
        }
        autoWlanNetz = r
        try { verbindung.requestNetwork(anfrage, r) } catch (_: Exception) { autoWlanNetz = null; zustand("auto_wlan_fehler") }
    }

    private fun autoWlanLoslassen() {
        val r = autoWlanNetz ?: return
        autoWlanNetz = null
        verbindung.bindProcessToNetwork(null)
        try { verbindung.unregisterNetworkCallback(r) } catch (_: Exception) {}
    }

    private fun wlanEinrichten() {
        val h = wlan.host ?: speicher.getString("ip", null)
        if (h != null) {
            oeffne(Intent(Intent.ACTION_VIEW, Uri.parse("http://$h/wlan")))
        } else {
            Toast.makeText(this, getString(R.string.wlan_einrichten_hinweis), Toast.LENGTH_LONG).show()
        }
    }

    // ---------------- Meldungen an die Seite ----------------
    private fun js(code: String) = web.evaluateJavascript(code, null)

    private fun zustand(code: String, extra: String = "") =
        js("window.robo&&robo.zustand(${JSONObject.quote(code)},${JSONObject.quote(extra)})")

    private fun antwortAnSeite(nr: Int, ok: Boolean, text: String) =
        js("window.__roboAntwort&&__roboAntwort($nr,$ok,${JSONObject.quote(text)})")

    // ---------------- Bluetooth-Ereignisse ----------------
    override fun bleGefunden(name: String, adresse: String, signal: Int) {
        if (art.isNotEmpty()) return
        val g = JSONObject().put("art", "ble").put("name", name).put("adresse", adresse).put("signal", signal)
        js("window.robo&&robo.gefunden($g)")
        if (autoVerbinden && adresse == speicher.getString("ble", null)) {
            autoVerbinden = false
            bleVerbinden(adresse)
        }
    }

    override fun bleBereit() {
        speicher.edit().putString("ble", ble.adresse).apply()      // naechstes Mal automatisch verbinden
        ble.anfrage(NR_INFO, "/info")                              // IP-Adresse merken + welches Board
        if (art != "ble") {
            art = "ble"
            seiteGeladen = false
            // Die Seite kommt mit der /info-Antwort. Bleibt sie aus, nach 2 s trotzdem.
            haupt.postDelayed({ if (art == "ble" && !seiteGeladen) steuerungZeigen("") }, 2000)
        }
    }

    // Steuerseite (assets/index.html, kommt beim Bauen aus include/webseite.h der Firmware)
    private var seiteGeladen = false
    @Suppress("UNUSED_PARAMETER")
    private fun steuerungZeigen(info: String) {
        seiteGeladen = true
        web.loadUrl(SEITEN + "index.html")
    }

    override fun bleGetrennt() {
        // Steuerung offen: die Seite zeigt "keine Verbindung", BleVerbindung versucht es von selbst neu
        if (art.isEmpty()) zustand("fehler")
    }

    override fun bleAntwort(nr: Int, text: String) {
        if (nr == NR_INFO) {
            try {
                val ip = JSONObject(text).optString("ip")
                if (ip.isNotEmpty()) speicher.edit().putString("ip", ip).apply()
            } catch (_: Exception) {}
            if (art == "ble" && !seiteGeladen) steuerungZeigen(text)
            return
        }
        antwortAnSeite(nr, true, text)
    }

    // ---------------- WLAN-Ereignisse ----------------
    override fun wlanGefunden(name: String, host: String) {
        if (art.isNotEmpty()) return
        val g = JSONObject().put("art", "wlan").put("name", name).put("adresse", host).put("signal", 0)
        js("window.robo&&robo.gefunden($g)")
    }

    override fun wlanAntwort(nr: Int, ok: Boolean, text: String) = antwortAnSeite(nr, ok, text)

    // ---------------- App-Update ----------------
    // Der Hinweis steht auf dem Verbinden-Bildschirm (die Seite fragt beim Laden auch selbst nach)
    override fun updateVerfuegbar(version: String, groesse: Long) {
        updateInfo = "$version|$groesse"
        js("window.robo&&robo.update(${JSONObject.quote(version)},$groesse)")
    }

    override fun updateFortschritt(prozent: Int) = js("window.robo&&robo.updateFortschritt($prozent)")

    override fun updateFehler(code: String) = js("window.robo&&robo.updateFehler(${JSONObject.quote(code)})")
}
