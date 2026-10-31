package de.roboauto.app

import android.content.Context
import android.net.nsd.NsdManager
import android.net.nsd.NsdServiceInfo
import android.net.wifi.WifiManager
import android.os.Build
import android.os.Handler
import android.os.Looper
import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.HttpURLConnection
import java.net.InetAddress
import java.net.URL
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.atomic.AtomicReference

/**
 * WLAN-Verbindung zum Auto: dieselben HTTP-Aufrufe wie die Webseite im Browser.
 * Die App macht sie selbst (nicht die Webseite), dann gibt es keine Probleme mit
 * Browser-Sicherheitsregeln. Das Auto wird per mDNS gefunden (es meldet sich als "roboauto").
 */
class WlanVerbindung(ctx: Context, private val ereignis: Ereignisse) {

    interface Ereignisse {
        fun wlanGefunden(name: String, host: String)
        fun wlanAntwort(nr: Int, ok: Boolean, text: String)
    }

    private val haupt = Handler(Looper.getMainLooper())
    private val pool = Executors.newFixedThreadPool(3)
    private val nsd = ctx.getSystemService(Context.NSD_SERVICE) as NsdManager

    /** Adresse des Autos (IP oder Name), null = nicht ueber WLAN verbunden */
    @Volatile var host: String? = null
        set(h) {
            field = h
            if (h == null) udpPort = 0
            wlanSchnell(h != null)
        }

    /** UDP-Port fuer Fahrbefehle (aus /info), 0 = aeltere Firmware -> Fahrbefehle per HTTP */
    @Volatile var udpPort = 0

    // Handy-WLAN nicht in den Stromsparmodus gehen lassen, solange wir fahren (sonst bis zu 100 ms Verzoegerung)
    @Suppress("DEPRECATION")
    private val wlanSperre = (ctx.applicationContext.getSystemService(Context.WIFI_SERVICE) as WifiManager).createWifiLock(
        if (Build.VERSION.SDK_INT >= 29) WifiManager.WIFI_MODE_FULL_LOW_LATENCY else WifiManager.WIFI_MODE_FULL_HIGH_PERF,
        "roboauto").apply { setReferenceCounted(false) }

    private fun wlanSchnell(an: Boolean) {
        try { if (an) wlanSperre.acquire() else if (wlanSperre.isHeld) wlanSperre.release() } catch (_: Exception) {}
    }

    private var udp: DatagramSocket? = null
    private val udpFaden = Executors.newSingleThreadExecutor()   // ein Faden: Pakete gehen in der richtigen Reihenfolge raus

    private fun holen(h: String, pfad: String): String {
        val v = URL("http://$h$pfad").openConnection() as HttpURLConnection
        v.connectTimeout = 2500
        v.readTimeout = 3000
        try {
            if (v.responseCode != 200) throw Exception("HTTP ${v.responseCode}")
            return v.inputStream.bufferedReader().readText()
        } finally {
            v.disconnect()
        }
    }

    /** Ist unter dieser Adresse ein Roboauto? Antwort von /info oder null. */
    fun pruefen(h: String, fertig: (String?) -> Unit) {
        pool.execute {
            val info = try { holen(h, "/info").takeIf { it.startsWith("{") } } catch (_: Exception) { null }
            haupt.post { fertig(info) }
        }
    }

    fun anfrage(nr: Int, pfad: String) {
        val h = host ?: return ereignis.wlanAntwort(nr, false, "nicht verbunden")
        pool.execute {
            try {
                val text = holen(h, pfad)
                haupt.post { ereignis.wlanAntwort(nr, true, text) }
            } catch (e: Exception) {
                haupt.post { ereignis.wlanAntwort(nr, false, e.message ?: "Fehler") }
            }
        }
    }

    // Fahrbefehle: nie mehrere gleichzeitig unterwegs, immer nur der neueste wird geschickt
    private val naechsteFahrt = AtomicReference<String?>(null)
    private val faehrt = AtomicBoolean(false)

    fun senden(pfad: String) {
        val h = host ?: return
        if (udpPort > 0) return udpSenden(h, udpPort, pfad)
        naechsteFahrt.set(pfad)
        if (!faehrt.compareAndSet(false, true)) return
        pool.execute {
            try {
                while (true) {
                    val p = naechsteFahrt.getAndSet(null) ?: break
                    try { holen(h, p) } catch (_: Exception) {}
                }
            } finally {
                faehrt.set(false)
            }
        }
    }

    // Ein Paket "0|/fahr?..." - ohne Verbindungsaufbau, ohne Antwort. Geht eins verloren, kommt 80 ms
    // spaeter das naechste. Nur der Stopp wird dreimal geschickt, damit das Auto sicher anhaelt.
    private fun udpSenden(h: String, port: Int, pfad: String) {
        val stopp = pfad.contains("x=0&y=0&t=0")
        udpFaden.execute {
            try {
                val s = udp ?: DatagramSocket().also { udp = it }
                val daten = "0|$pfad".toByteArray()
                val paket = DatagramPacket(daten, daten.size, InetAddress.getByName(h), port)
                repeat(if (stopp) 3 else 1) { s.send(paket) }
            } catch (_: Exception) {}
        }
    }

    // ---------------- Suchen (mDNS) ----------------
    private var suche: NsdManager.DiscoveryListener? = null

    fun suchen() {
        if (suche != null) return
        val l = object : NsdManager.DiscoveryListener {
            override fun onServiceFound(info: NsdServiceInfo) {
                if (info.serviceName.startsWith("roboauto", ignoreCase = true)) aufloesen(info)
            }
            override fun onServiceLost(info: NsdServiceInfo) {}
            override fun onDiscoveryStarted(typ: String) {}
            override fun onDiscoveryStopped(typ: String) {}
            override fun onStartDiscoveryFailed(typ: String, fehler: Int) { haupt.post { suche = null } }
            override fun onStopDiscoveryFailed(typ: String, fehler: Int) {}
        }
        suche = l
        try { nsd.discoverServices("_http._tcp.", NsdManager.PROTOCOL_DNS_SD, l) } catch (_: Exception) { suche = null }
    }

    @Suppress("DEPRECATION")
    private fun aufloesen(info: NsdServiceInfo) {
        try {
            nsd.resolveService(info, object : NsdManager.ResolveListener {
                override fun onResolveFailed(i: NsdServiceInfo, fehler: Int) {}
                override fun onServiceResolved(i: NsdServiceInfo) {
                    val ip = i.host?.hostAddress ?: return
                    if (ip.contains(':')) return                     // nur IPv4
                    haupt.post { ereignis.wlanGefunden(i.serviceName, ip) }
                }
            })
        } catch (_: Exception) {}
    }

    fun suchStopp() {
        val l = suche ?: return
        suche = null
        try { nsd.stopServiceDiscovery(l) } catch (_: Exception) {}
    }

    fun beenden() {
        suchStopp()
        wlanSchnell(false)
        udpFaden.shutdownNow()
        try { udp?.close() } catch (_: Exception) {}
        pool.shutdownNow()
    }
}
