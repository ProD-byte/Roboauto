package de.roboauto.app

import android.app.Activity
import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.pm.PackageInstaller
import android.net.Uri
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.provider.Settings
import androidx.core.content.ContextCompat
import org.json.JSONObject
import java.io.File
import java.net.HttpURLConnection
import java.net.URL
import java.util.concurrent.Executors

/**
 * Die App aktualisiert sich selbst: Beim Start schaut sie in app.json auf GitHub Pages nach,
 * ob es eine neuere Version gibt (Versionsnummer = GitHub-Build-Nummer). Auf Wunsch laedt sie
 * die APK und installiert sie ueber den PackageInstaller von Android.
 *
 * Das erste Mal fragt Android "Apps aus dieser Quelle erlauben?" und "Aktualisieren?".
 * Ab Android 12 startet MainActivity das Update von selbst, solange kein Auto verbunden ist.
 * Ab Android 12 geht es danach ohne Nachfrage, weil unsere App dann selbst als Installierer gilt.
 * Klappt nur, wenn die neue APK mit demselben Schluessel signiert ist (GitHub-Secrets).
 */
class Aktualisierung(private val activity: Activity, private val ereignis: Ereignisse) {

    interface Ereignisse {
        fun updateVerfuegbar(version: String, groesse: Long)
        fun updateFortschritt(prozent: Int)
        fun updateFehler(code: String)
    }

    companion object {
        private const val SEITE = "https://prod-byte.github.io/Roboauto/"
        private const val AKTION = "de.roboauto.app.UPDATE_ERGEBNIS"
    }

    private val haupt = Handler(Looper.getMainLooper())
    private val faden = Executors.newSingleThreadExecutor()
    private var neueVersion: String? = null
    @Volatile private var laeuft = false
    private var wartetAufErlaubnis = false

    private fun eigeneNummer(): Long {
        val info = activity.packageManager.getPackageInfo(activity.packageName, 0)
        @Suppress("DEPRECATION")
        return if (Build.VERSION.SDK_INT >= 28) info.longVersionCode else info.versionCode.toLong()
    }

    /** Im Hintergrund nachsehen, ob es eine neuere App gibt (meldet nur, wenn ja) */
    fun pruefen() {
        faden.execute {
            try {
                val v = URL(SEITE + "app.json?t=" + System.currentTimeMillis()).openConnection() as HttpURLConnection
                v.connectTimeout = 5000; v.readTimeout = 5000
                v.useCaches = false
                val j = JSONObject(v.inputStream.bufferedReader().readText())
                v.disconnect()
                val version = j.optString("version")                       // "1.0.<Build>"
                val nummer = version.substringAfterLast('.').toLongOrNull() ?: return@execute
                if (nummer > eigeneNummer()) {
                    neueVersion = version
                    haupt.post { ereignis.updateVerfuegbar(version, j.optLong("groesse")) }
                }
            } catch (_: Exception) {}                                        // kein Internet -> einfach nichts melden
        }
    }

    /** Neue Version herunterladen und installieren */
    fun installieren() {
        if (laeuft || neueVersion == null) return
        // Android 8+: "Apps aus dieser Quelle erlauben" muss fuer unsere App an sein
        if (!activity.packageManager.canRequestPackageInstalls()) {
            wartetAufErlaubnis = true
            ereignis.updateFehler("erlauben")
            try {
                activity.startActivity(Intent(Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES,
                    Uri.parse("package:" + activity.packageName)))
            } catch (_: Exception) {}
            return
        }
        laeuft = true
        ereignis.updateFortschritt(0)
        faden.execute {
            try {
                val datei = laden()
                haupt.post { uebergeben(datei) }
            } catch (_: Exception) {
                laeuft = false
                haupt.post { ereignis.updateFehler("laden") }
            }
        }
    }

    /** Zurueck aus den Einstellungen: war die Erlaubnis der Grund, gleich weitermachen */
    fun weiterNachErlaubnis() {
        if (!wartetAufErlaubnis) return
        wartetAufErlaubnis = false
        if (activity.packageManager.canRequestPackageInstalls()) installieren()
    }

    private fun laden(): File {
        val v = URL(SEITE + "roboauto.apk?t=" + System.currentTimeMillis()).openConnection() as HttpURLConnection
        v.connectTimeout = 8000; v.readTimeout = 15000
        v.useCaches = false
        val gesamt = v.contentLengthLong
        val datei = File(activity.cacheDir, "update.apk")
        v.inputStream.use { ein ->
            datei.outputStream().use { aus ->
                val puffer = ByteArray(32 * 1024)
                var geladen = 0L
                var letzte = -1
                while (true) {
                    val n = ein.read(puffer)
                    if (n < 0) break
                    aus.write(puffer, 0, n)
                    geladen += n
                    val p = if (gesamt > 0) (geladen * 100 / gesamt).toInt() else 0
                    if (p != letzte) { letzte = p; haupt.post { ereignis.updateFortschritt(p) } }
                }
            }
        }
        v.disconnect()
        return datei
    }

    // APK an den PackageInstaller geben. Das Ergebnis kommt als Broadcast zurueck.
    private fun uebergeben(datei: File) {
        try {
            val installer = activity.packageManager.packageInstaller
            val parameter = PackageInstaller.SessionParams(PackageInstaller.SessionParams.MODE_FULL_INSTALL)
            parameter.setAppPackageName(activity.packageName)
            if (Build.VERSION.SDK_INT >= 31) parameter.setRequireUserAction(PackageInstaller.SessionParams.USER_ACTION_NOT_REQUIRED)
            val id = installer.createSession(parameter)
            installer.openSession(id).use { sitzung ->
                datei.inputStream().use { ein ->
                    sitzung.openWrite("roboauto.apk", 0, datei.length()).use { aus ->
                        ein.copyTo(aus)
                        sitzung.fsync(aus)
                    }
                }
                try { activity.unregisterReceiver(empfaenger) } catch (_: Exception) {}   // nicht doppelt anmelden
                ContextCompat.registerReceiver(activity, empfaenger, IntentFilter(AKTION), ContextCompat.RECEIVER_NOT_EXPORTED)
                val absicht = Intent(AKTION).setPackage(activity.packageName)
                val pi = PendingIntent.getBroadcast(activity, id, absicht,
                    PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_MUTABLE)
                sitzung.commit(pi.intentSender)
            }
        } catch (_: Exception) {
            laeuft = false
            ereignis.updateFehler("installieren")
        }
    }

    private val empfaenger = object : BroadcastReceiver() {
        override fun onReceive(ctx: Context, intent: Intent) {
            when (intent.getIntExtra(PackageInstaller.EXTRA_STATUS, PackageInstaller.STATUS_FAILURE)) {
                PackageInstaller.STATUS_PENDING_USER_ACTION -> {
                    // Android fragt nach ("Aktualisieren?") - den Dialog zeigen
                    @Suppress("DEPRECATION")
                    val bestaetigen = intent.getParcelableExtra<Intent>(Intent.EXTRA_INTENT)
                    if (bestaetigen != null) {
                        bestaetigen.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                        try { activity.startActivity(bestaetigen) } catch (_: Exception) {}
                    }
                }
                PackageInstaller.STATUS_SUCCESS -> { /* die App wird gleich neu gestartet */ }
                else -> {
                    laeuft = false
                    try { activity.unregisterReceiver(this) } catch (_: Exception) {}
                    val status = intent.getIntExtra(PackageInstaller.EXTRA_STATUS, 0)
                    // anderer Schluessel -> Android verweigert das Update
                    val code = if (status == PackageInstaller.STATUS_FAILURE_CONFLICT ||
                                   status == PackageInstaller.STATUS_FAILURE_INCOMPATIBLE) "schluessel"
                               else if (status == PackageInstaller.STATUS_FAILURE_ABORTED) "abgebrochen"
                               else "installieren"
                    ereignis.updateFehler(code)
                }
            }
        }
    }

    fun beenden() {
        try { activity.unregisterReceiver(empfaenger) } catch (_: Exception) {}
        faden.shutdownNow()
    }
}
