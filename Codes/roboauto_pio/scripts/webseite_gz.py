# PlatformIO-Vorab-Skript: packt die Weboberflaeche (WEBSEITE in include/webseite.h)
# mit gzip und schreibt sie als Byte-Feld nach .pio/build/<umgebung>/erzeugt/webseite_gz.h.
# Der Browser entpackt sie selbst - spart rund 70 KB Flash.
# Laeuft bei jedem Build automatisch (auch auf GitHub), siehe extra_scripts in platformio.ini.
#
# Eine Webseite fuer beide Autos: Teile nur fuer den XIAO Sense (Kamera, Mikrofon, SD-Karte)
# stehen zwischen <!--XIAO--> ... <!--/XIAO--> (HTML) bzw. /*XIAO*/ ... /*/XIAO*/ (CSS/JS).
# Beim Super Mini fliegen sie raus, beim XIAO nur die Markierungen.
# Die App macht beim Bauen dasselbe (roboauto_app/app/build.gradle.kts).
import gzip, os, re

try:
    Import("env")  # noqa: F821  (von PlatformIO bereitgestellt)
    basis = env.subst("$PROJECT_DIR")  # noqa: F821
    xiao = "ROBOAUTO_XIAO" in env.subst("$BUILD_FLAGS")  # noqa: F821
    zielordner = os.path.join(env.subst("$BUILD_DIR"), "erzeugt")  # noqa: F821
    env.Append(CPPPATH=[zielordner])  # noqa: F821
except NameError:
    basis = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    xiao = False
    zielordner = os.path.join(basis, ".pio", "erzeugt")


def fuer_board(html, xiao):
    if xiao:
        return re.sub(r"<!--/?XIAO-->|/\*/?XIAO\*/", "", html)
    return re.sub(r"<!--XIAO-->.*?<!--/XIAO-->|/\*XIAO\*/.*?/\*/XIAO\*/", "", html, flags=re.S)


quelle = os.path.join(basis, "include", "webseite.h")
ziel = os.path.join(zielordner, "webseite_gz.h")
alt_ziel = os.path.join(basis, "include", "webseite_gz.h")   # frueherer Ort - wuerde die neue Datei verdecken
if os.path.exists(alt_ziel):
    os.remove(alt_ziel)

text = open(quelle, encoding="utf-8").read()
m = re.search(r'const char WEBSEITE\[\] PROGMEM = R"==\((.*?)\)==";', text, re.S)
if not m:
    raise SystemExit("webseite_gz.py: WEBSEITE nicht gefunden")
roh = fuer_board(m.group(1), xiao).encode("utf-8")
gz = gzip.compress(roh, compresslevel=9, mtime=0)

zeilen = []
for i in range(0, len(gz), 24):
    zeilen.append("  " + ",".join("0x%02x" % b for b in gz[i:i + 24]) + ",")
inhalt = (
    "// AUTOMATISCH ERZEUGT von scripts/webseite_gz.py - nicht von Hand aendern!\n"
    "// Quelle: include/webseite.h (WEBSEITE, %s), %d Bytes -> gzip %d Bytes\n"
    "#pragma once\n#include <Arduino.h>\n"
    "const size_t WEBSEITE_GZ_LAENGE = %d;\n"
    "const uint8_t WEBSEITE_GZ[] PROGMEM = {\n%s\n};\n"
    % ("XIAO Sense" if xiao else "Super Mini", len(roh), len(gz), len(gz), "\n".join(zeilen))
)
os.makedirs(zielordner, exist_ok=True)
alt = open(ziel, encoding="utf-8").read() if os.path.exists(ziel) else ""
if alt != inhalt:
    open(ziel, "w", encoding="utf-8").write(inhalt)
print("webseite_gz.py: Weboberflaeche (%s) %d -> %d Bytes (gzip)" % ("XIAO" if xiao else "Mini", len(roh), len(gz)))
