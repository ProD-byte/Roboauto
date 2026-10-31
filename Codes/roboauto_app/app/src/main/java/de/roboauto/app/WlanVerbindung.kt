package de.roboauto.app

import android.content.Context
import android.net.nsd.NsdManager
import android.net.nsd.NsdServiceInfo
import android.os.Handler
import android.os.Looper
import java.net.HttpURLConnection
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
        pool.shutdownNow()
    }
}
