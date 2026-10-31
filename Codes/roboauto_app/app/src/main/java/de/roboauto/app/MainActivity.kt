package de.roboauto.app

import android.Manifest
import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Color
import android.location.LocationManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.provider.Settings
import android.view.WindowManager
import android.webkit.JavascriptInterface
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
import androidx.core.view.WindowInsetsCompat
import androidx.webkit.WebViewAssetLoader
import org.json.JSONObject

/**
 * Die App ist eine Huelle um die Weboberflaeche des Autos (assets/index.html, kommt beim
 * Bauen aus include/webseite.h). Die Webseite fragt wie im Browser fetch('/status') usw. -
 * app.js leitet das hierher um, und die App schickt es per Bluetooth oder WLAN ans Auto.
 */
class MainActivity : ComponentActivity(), BleVerbindung.Ereignisse, WlanVerbindung.Ereignisse {

    companion object {
        private const val SEITEN = "https://appassets.androidplatform.net/assets/"
        private const val NR_INFO = -1                 // eigene Anfrage nach /info (IP-Adresse merken)
    }

    private lateinit var web: WebView
    private lateinit var ble: BleVerbindung
    private lateinit var wlan: WlanVerbindung
    private val haupt = Handler(Looper.getMainLooper())
    private val speicher by lazy { getSharedPreferences("roboauto", MODE_PRIVATE) }

    private var art = ""                 // "ble", "wlan" oder "" (Verbinden-Bildschirm)
    private var autoVerbinden = true     // bekanntes Auto beim Finden gleich verbinden (nicht nach "Trennen")

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
        web.webViewClient = object : WebViewClient() {
            override fun shouldInterceptRequest(view: WebView, request: WebResourceRequest): WebResourceResponse? =
                lader.shouldInterceptRequest(request.url)

            override fun shouldOverrideUrlLoading(view: WebView, request: WebResourceRequest): Boolean {
                val url = request.url
                if (url.toString().startsWith(SEITEN)) return false
                if (url.host == "appassets.androidplatform.net" && url.path == "/wlan") { wlanEinrichten(); return true }
                startActivity(Intent(Intent.ACTION_VIEW, url))   // fremde Links im Browser oeffnen
                return true
            }
        }
        web.addJavascriptInterface(Bruecke(), "Roboauto")

        onBackPressedDispatcher.addCallback(this, object : OnBackPressedCallback(true) {
            override fun handleOnBackPressed() {
                if (art.isNotEmpty()) zumVerbinden(manuell = true) else finish()
            }
        })

        web.loadUrl(SEITEN + "verbinden.html")
    }

    override fun onDestroy() {
        ble.trennen()
        ble.suchStopp()
        wlan.beenden()
        web.destroy()
        super.onDestroy()
    }

    // ---------------- Bruecke fuer JavaScript (window.Roboauto) ----------------
    // Wird auf einem eigenen Thread aufgerufen -> alles auf den Haupt-Thread schieben
    inner class Bruecke {
        @JavascriptInterface fun suchen() { haupt.post { this@MainActivity.suchen() } }
        @JavascriptInterface fun verbindeBle(adresse: String) { haupt.post { bleVerbinden(adresse) } }
        @JavascriptInterface fun verbindeWlan(host: String) { haupt.post { wlanVerbinden(host.trim()) } }
        @JavascriptInterface fun trennen() { haupt.post { zumVerbinden(manuell = true) } }
        @JavascriptInterface fun art(): String = art
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
            speicher.edit().putString("wlan", host).apply()
            art = "wlan"
            web.loadUrl(SEITEN + "index.html")
        }
    }

    private fun zumVerbinden(manuell: Boolean) {
        if (manuell) autoVerbinden = false
        art = ""
        ble.trennen()
        wlan.host = null
        web.loadUrl(SEITEN + "verbinden.html")
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
        ble.anfrage(NR_INFO, "/info")                              // IP-Adresse fuer WLAN merken
        if (art != "ble") {
            art = "ble"
            web.loadUrl(SEITEN + "index.html")
        }
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
}
