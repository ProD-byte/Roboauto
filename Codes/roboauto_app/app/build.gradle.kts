plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

// Versionsnummer: auf GitHub die Build-Nummer, am PC 1
val buildNummer = (System.getenv("GITHUB_RUN_NUMBER") ?: "1").toInt()

// Signierschluessel kommt auf GitHub aus den Secrets (siehe README). Ohne Schluessel: Debug-Schluessel.
val schluessel = System.getenv("ROBOAUTO_KEYSTORE")

android {
    namespace = "de.roboauto.app"
    compileSdk = 35

    defaultConfig {
        applicationId = "de.roboauto.app"
        minSdk = 26          // Android 8
        targetSdk = 35
        versionCode = buildNummer
        versionName = "1.0.$buildNummer"
    }

    signingConfigs {
        create("roboauto") {
            if (schluessel != null) {
                storeFile = file(schluessel)
                storePassword = System.getenv("ROBOAUTO_KEYSTORE_PASSWORT")
                keyAlias = System.getenv("ROBOAUTO_KEY_ALIAS") ?: "roboauto"
                keyPassword = System.getenv("ROBOAUTO_KEYSTORE_PASSWORT")
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            signingConfig = signingConfigs.getByName(if (schluessel != null) "roboauto" else "debug")
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions {
        jvmTarget = "17"
    }
    lint {
        abortOnError = false
        checkReleaseBuilds = false
    }
}

dependencies {
    implementation("androidx.core:core-ktx:1.15.0")
    implementation("androidx.activity:activity-ktx:1.9.3")
    implementation("androidx.webkit:webkit:1.12.1")
}

// ------------------------------------------------------------
//  Weboberflaeche aus der Firmware uebernehmen: WEBSEITE aus include/webseite.h
//  wird zu assets/index.html. Damit sieht die App immer genauso aus wie die Webseite.
//  Dazu kommen die App-eigenen Dateien aus app/web (app.js, verbinden.html).
// ------------------------------------------------------------
abstract class WebseiteAusziehen : DefaultTask() {
    @get:InputFile abstract val quelle: RegularFileProperty
    @get:InputDirectory abstract val webOrdner: DirectoryProperty
    @get:OutputDirectory abstract val ausgabe: DirectoryProperty

    @TaskAction
    fun los() {
        val text = quelle.get().asFile.readText()
        val html = Regex("""const char WEBSEITE\[\] PROGMEM = R"==\((.*?)\)==";""", RegexOption.DOT_MATCHES_ALL)
            .find(text)?.groupValues?.get(1) ?: throw GradleException("WEBSEITE in webseite.h nicht gefunden")
        // Auto-Logo aus der Kopfleiste - der Verbinden-Bildschirm benutzt es auch
        val logo = Regex("""<div class="marke">(<svg.*?</svg>)""", RegexOption.DOT_MATCHES_ALL)
            .find(html)?.groupValues?.get(1) ?: ""

        val ziel = ausgabe.get().asFile
        ziel.deleteRecursively()
        ziel.mkdirs()
        // app.js als erstes Skript: leitet fetch('/status') usw. an die App weiter
        File(ziel, "index.html").writeText(html.replaceFirst("<head>", "<head>\n<script src=\"app.js\"></script>"))
        webOrdner.get().asFile.listFiles()!!.forEach { f ->
            File(ziel, f.name).writeText(f.readText().replace("<!--LOGO-->", logo))
        }
    }
}

val webseite = tasks.register<WebseiteAusziehen>("webseiteAusziehen") {
    quelle.set(file("../../roboauto_pio/include/webseite.h"))
    webOrdner.set(file("web"))
}

androidComponents {
    onVariants { variante ->
        variante.sources.assets?.addGeneratedSourceDirectory(webseite, WebseiteAusziehen::ausgabe)
    }
}
