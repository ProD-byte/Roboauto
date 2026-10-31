package de.roboauto.app

import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothGattDescriptor
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothProfile
import android.bluetooth.BluetoothStatusCodes
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanFilter
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.os.ParcelUuid
import java.io.ByteArrayOutputStream
import java.util.UUID

/**
 * Bluetooth-Verbindung zum Auto (eigener Dienst statt Dabble).
 * Protokoll: Codes/roboauto_pio/docs/app-protokoll.md
 *
 * Alles laeuft auf dem Haupt-Thread: Android meldet Bluetooth-Ereignisse auf eigenen
 * Threads, die werden hier sofort auf den Haupt-Thread umgeleitet.
 * Die Erlaubnisse prueft MainActivity, bevor sie hier etwas aufruft.
 */
@SuppressLint("MissingPermission")
class BleVerbindung(private val ctx: Context, private val ereignis: Ereignisse) {

    interface Ereignisse {
        fun bleGefunden(name: String, adresse: String, signal: Int)
        fun bleBereit()
        fun bleGetrennt()
        fun bleAntwort(nr: Int, text: String)
    }

    companion object {
        val DIENST: UUID = UUID.fromString("8d3a1000-5b7e-4f2a-9c61-2e4b7a9d0c11")
        val BEFEHL: UUID = UUID.fromString("8d3a1001-5b7e-4f2a-9c61-2e4b7a9d0c11")
        val ANTWORT: UUID = UUID.fromString("8d3a1002-5b7e-4f2a-9c61-2e4b7a9d0c11")
        val CCCD: UUID = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")
        private const val SUCHDAUER_MS = 15_000L
        private const val NEUVERSUCH_MS = 1_500L
    }

    private val haupt = Handler(Looper.getMainLooper())
    val adapter: BluetoothAdapter?
        get() = (ctx.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager?)?.adapter

    private var gatt: BluetoothGatt? = null
    private var befehl: BluetoothGattCharacteristic? = null
    private var gewollt: String? = null          // mit diesem Auto verbunden bleiben (Neuversuch nach Abbruch)
    var verbunden = false
        private set
    val adresse: String? get() = gewollt

    // Android kann immer nur einen Schreibvorgang gleichzeitig -> Warteschlange
    private class Paket(val daten: ByteArray, val wichtig: Boolean)
    private val warteschlange = ArrayDeque<Paket>()
    private var schreibtGerade = false
    private val puffer = ByteArrayOutputStream()

    // ---------------- Suchen ----------------
    private var sucht = false
    private val suchEnde = Runnable { suchStopp() }

    private val scanCb = object : ScanCallback() {
        override fun onScanResult(typ: Int, r: ScanResult) {
            val name = r.scanRecord?.deviceName ?: r.device.name ?: "Roboauto"
            ereignis.bleGefunden(name, r.device.address, r.rssi)
        }
        override fun onScanFailed(code: Int) { sucht = false }
    }

    fun suchen() {
        val scanner = adapter?.bluetoothLeScanner ?: return
        if (!sucht) {
            val filter = listOf(ScanFilter.Builder().setServiceUuid(ParcelUuid(DIENST)).build())
            val einst = ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build()
            scanner.startScan(filter, einst, scanCb)
            sucht = true
        }
        haupt.removeCallbacks(suchEnde)
        haupt.postDelayed(suchEnde, SUCHDAUER_MS)
    }

    fun suchStopp() {
        haupt.removeCallbacks(suchEnde)
        if (!sucht) return
        sucht = false
        try { adapter?.bluetoothLeScanner?.stopScan(scanCb) } catch (_: Exception) {}
    }

    // ---------------- Verbinden ----------------
    fun verbinden(adresse: String) {
        suchStopp()
        trennen()
        gewollt = adresse
        oeffnen(adresse)
    }

    private fun oeffnen(adresse: String) {
        val geraet = adapter?.getRemoteDevice(adresse) ?: return
        puffer.reset()
        gatt = geraet.connectGatt(ctx, false, gattCb, BluetoothDevice.TRANSPORT_LE)
    }

    fun trennen() {
        gewollt = null
        verbunden = false
        warteschlange.clear()
        schreibtGerade = false
        gatt?.let { it.disconnect(); it.close() }
        gatt = null
        befehl = null
    }

    // Verbindung weg (oder Aufbau gescheitert): aufraeumen und - falls gewollt - neu versuchen
    private fun verloren(g: BluetoothGatt) {
        if (g != gatt) return
        g.close()
        gatt = null
        befehl = null
        warteschlange.clear()
        schreibtGerade = false
        verbunden = false
        ereignis.bleGetrennt()
        val adresse = gewollt ?: return
        haupt.postDelayed({ if (gewollt == adresse && gatt == null) oeffnen(adresse) }, NEUVERSUCH_MS)
    }

    private val gattCb = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(g: BluetoothGatt, status: Int, neu: Int) {
            haupt.post {
                if (g != gatt) return@post
                if (neu == BluetoothProfile.STATE_CONNECTED && status == BluetoothGatt.GATT_SUCCESS) {
                    g.requestConnectionPriority(BluetoothGatt.CONNECTION_PRIORITY_HIGH)   // kurze Reaktionszeit beim Fahren
                    if (!g.requestMtu(517)) g.discoverServices()                          // grosse Pakete fuer den Status
                } else {
                    verloren(g)
                }
            }
        }

        override fun onMtuChanged(g: BluetoothGatt, mtu: Int, status: Int) {
            haupt.post { if (g == gatt) g.discoverServices() }
        }

        override fun onServicesDiscovered(g: BluetoothGatt, status: Int) {
            haupt.post {
                if (g != gatt) return@post
                val dienst = g.getService(DIENST)
                val b = dienst?.getCharacteristic(BEFEHL)
                val a = dienst?.getCharacteristic(ANTWORT)
                val d = a?.getDescriptor(CCCD)
                if (b == null || a == null || d == null) { g.disconnect(); return@post }
                befehl = b
                g.setCharacteristicNotification(a, true)
                val an = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                if (Build.VERSION.SDK_INT >= 33) {
                    g.writeDescriptor(d, an)
                } else {
                    @Suppress("DEPRECATION")
                    d.value = an
                    @Suppress("DEPRECATION")
                    g.writeDescriptor(d)
                }
            }
        }

        override fun onDescriptorWrite(g: BluetoothGatt, d: BluetoothGattDescriptor, status: Int) {
            haupt.post {
                if (g != gatt) return@post
                verbunden = true
                ereignis.bleBereit()
                schreibeNaechstes()
            }
        }

        override fun onCharacteristicWrite(g: BluetoothGatt, c: BluetoothGattCharacteristic, status: Int) {
            haupt.post {
                if (g != gatt) return@post
                schreibtGerade = false
                schreibeNaechstes()
            }
        }

        // Android 13+
        override fun onCharacteristicChanged(g: BluetoothGatt, c: BluetoothGattCharacteristic, wert: ByteArray) {
            haupt.post { if (g == gatt) empfangen(wert) }
        }

        // bis Android 12
        @Deprecated("Deprecated in Java")
        override fun onCharacteristicChanged(g: BluetoothGatt, c: BluetoothGattCharacteristic) {
            if (Build.VERSION.SDK_INT >= 33) return
            @Suppress("DEPRECATION")
            val wert = c.value?.copyOf() ?: return
            haupt.post { if (g == gatt) empfangen(wert) }
        }
    }

    // ---------------- Senden / Empfangen ----------------

    /** Befehl mit Antwort, z. B. anfrage(12, "/status"). false = nicht verbunden. */
    fun anfrage(nr: Int, pfad: String): Boolean = einreihen("$nr|$pfad", true)

    /** Fahrbefehl ohne Antwort. Nur der neueste zaehlt - aeltere, noch nicht gesendete fliegen raus. */
    fun senden(pfad: String): Boolean = einreihen("0|$pfad", false)

    private fun einreihen(text: String, wichtig: Boolean): Boolean {
        if (!verbunden) return false
        if (!wichtig) warteschlange.removeAll { !it.wichtig }
        if (warteschlange.size > 30) warteschlange.removeFirst()
        warteschlange.addLast(Paket(text.toByteArray(Charsets.UTF_8), wichtig))
        schreibeNaechstes()
        return true
    }

    private fun schreibeNaechstes() {
        if (schreibtGerade || !verbunden) return
        val g = gatt ?: return
        val c = befehl ?: return
        val p = warteschlange.removeFirstOrNull() ?: return
        val typ = if (p.wichtig) BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
                  else BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
        val ok = if (Build.VERSION.SDK_INT >= 33) {
            g.writeCharacteristic(c, p.daten, typ) == BluetoothStatusCodes.SUCCESS
        } else {
            c.writeType = typ
            @Suppress("DEPRECATION")
            c.value = p.daten
            @Suppress("DEPRECATION")
            g.writeCharacteristic(c)
        }
        if (ok) {
            schreibtGerade = true
        } else {
            warteschlange.addFirst(p)                          // Stapel gerade beschaeftigt -> gleich nochmal
            haupt.postDelayed({ schreibeNaechstes() }, 15)
        }
    }

    // Antworten kommen in Stuecken: "<nr>|<inhalt>\n"
    private fun empfangen(daten: ByteArray) {
        for (b in daten) {
            if (b == '\n'.code.toByte()) {
                val text = puffer.toString("UTF-8")
                puffer.reset()
                val strich = text.indexOf('|')
                val nr = if (strich > 0) text.substring(0, strich).toIntOrNull() else null
                if (nr != null) ereignis.bleAntwort(nr, text.substring(strich + 1))
            } else {
                puffer.write(b.toInt())
            }
        }
    }
}
