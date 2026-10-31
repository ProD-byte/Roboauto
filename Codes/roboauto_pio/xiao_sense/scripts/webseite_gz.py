# PlatformIO-Vorab-Skript: packt die Weboberflaeche (WEBSEITE in include/webseite.h)
# mit gzip und schreibt sie als Byte-Feld nach include/webseite_gz.h.
# Der Browser entpackt sie selbst - spart rund 70 KB Flash.
# Laeuft bei jedem Build automatisch (auch auf GitHub), siehe extra_scripts in platformio.ini.
import gzip, os, re

try:
    Import("env")  # noqa: F821  (von PlatformIO bereitgestellt)
    basis = env.subst("$PROJECT_DIR")  # noqa: F821
except NameError:
    basis = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

quelle = os.path.join(basis, "include", "webseite.h")
ziel = os.path.join(basis, "include", "webseite_gz.h")

text = open(quelle, encoding="utf-8").read()
m = re.search(r'const char WEBSEITE\[\] PROGMEM = R"==\((.*?)\)==";', text, re.S)
if not m:
    raise SystemExit("webseite_gz.py: WEBSEITE nicht gefunden")
roh = m.group(1).encode("utf-8")
gz = gzip.compress(roh, compresslevel=9, mtime=0)

zeilen = []
for i in range(0, len(gz), 24):
    zeilen.append("  " + ",".join("0x%02x" % b for b in gz[i:i + 24]) + ",")
inhalt = (
    "// AUTOMATISCH ERZEUGT von scripts/webseite_gz.py - nicht von Hand aendern!\n"
    "// Quelle: include/webseite.h (WEBSEITE), %d Bytes -> gzip %d Bytes\n"
    "#pragma once\n#include <Arduino.h>\n"
    "const size_t WEBSEITE_GZ_LAENGE = %d;\n"
    "const uint8_t WEBSEITE_GZ[] PROGMEM = {\n%s\n};\n" % (len(roh), len(gz), len(gz), "\n".join(zeilen))
)
alt = open(ziel, encoding="utf-8").read() if os.path.exists(ziel) else ""
if alt != inhalt:
    open(ziel, "w", encoding="utf-8").write(inhalt)
print("webseite_gz.py: Weboberflaeche %d -> %d Bytes (gzip)" % (len(roh), len(gz)))
