# Erzeugt die Zeichnungen des Roboauto V7 fuer die Weboberflaeche (include/webseite.h)
# und setzt sie dort ein: Logo, Live-Ansicht von oben, Seite (Neigung vorn/hinten), Heck (Neigung seitlich).
# Masse abgelesen aus den SketchUp-Ansichten (Parallelprojektion).
# Aufruf (einmalig, nach Aenderungen am Design):  python scripts/auto_v7_svg.py
import math, os, re

GELB, DECKEL, WANNE, REIFEN, FELGE, LICHT = '#ffe03a', '#f2f2f2', '#404040', '#2a2a2a', '#383838', '#c8cde0'
LINIE = 'stroke="#111" stroke-width=".6"'


def f(x):
    return ('%.1f' % x).rstrip('0').rstrip('.')


def rad_seite(cx, cy, r):
    """Profilrad von der Seite: Stollen, Reifen, Felge mit 6 Speichen."""
    s = ''
    for k in range(20):                                   # Stollen rundherum
        s += '<rect x="%s" y="%s" width="%s" height="%s" rx="1" fill="#1a1a1a" transform="rotate(%d %s %s)"/>' % (
            f(cx - r * .11), f(cy - r - 1.2), f(r * .22), f(r * .12 + 1.2), k * 18, f(cx), f(cy))
    s += '<circle cx="%s" cy="%s" r="%s" fill="%s" %s/>' % (f(cx), f(cy), f(r * .96), WANNE, LINIE)
    s += '<circle cx="%s" cy="%s" r="%s" fill="%s" stroke="#1e1e1e" stroke-width="1"/>' % (f(cx), f(cy), f(r * .66), FELGE)
    s += '<circle cx="%s" cy="%s" r="%s" fill="none" stroke="#2a2a2a" stroke-width=".8"/>' % (f(cx), f(cy), f(r * .55))
    for k in range(6):
        w = math.radians(k * 60 + 30)
        s += '<line x1="%s" y1="%s" x2="%s" y2="%s" stroke="#222" stroke-width="1.3"/>' % (
            f(cx + math.cos(w) * r * .15), f(cy + math.sin(w) * r * .15),
            f(cx + math.cos(w) * r * .55), f(cy + math.sin(w) * r * .55))
    s += '<circle cx="%s" cy="%s" r="%s" fill="#2e2e2e" stroke="#1a1a1a" stroke-width=".8"/>' % (f(cx), f(cy), f(r * .17))
    s += '<circle cx="%s" cy="%s" r="%s" fill="%s"/>' % (f(cx), f(cy), f(r * .07), LICHT)
    return s


def seite():
    """Seitenansicht, Boden bei y=38, Mitte x=0 (wie die alte Zeichnung - dreht sich um 0,38)."""
    r, cy = 31.4, 38 - 31.4
    s = '<rect x="-50" y="-22.3" width="100" height="3.2" fill="%s" %s/>' % (DECKEL, LINIE)        # Deckel
    s += '<rect x="-6" y="-24.2" width="12" height="2" rx=".8" fill="#9a9a9a" %s/>' % LINIE           # Knopf
    s += '<rect x="-50" y="-19.1" width="100" height="19" fill="%s" %s/>' % (GELB, LINIE)          # Mittelteil
    s += '<rect x="-50" y="-.1" width="100" height="15.9" fill="%s" %s/>' % (WANNE, LINIE)         # Wanne
    return s + rad_seite(-32, cy, r) + rad_seite(32, cy, r)


def heck():
    """Heckansicht (Neigung seitlich), Boden y=38. rv2 = Ruecklicht links, rv3 = rechts."""
    s = '<defs><pattern id="profilH" width="48" height="9" patternUnits="userSpaceOnUse">' \
        '<path d="M-4 5 q13 -6 26 0 t26 0" stroke="#5a5a5a" stroke-width="2.4" fill="none"/></pattern></defs>'
    for x in (-65, 35.2):                                                                            # Raeder
        s += '<rect x="%s" y="-13" width="29.8" height="51" rx="8" fill="%s" %s/>' % (f(x), REIFEN, LINIE)
        s += '<rect x="%s" y="-13" width="29.8" height="51" rx="8" fill="url(#profilH)"/>' % f(x)
    s += '<rect x="-38.8" y="10.5" width="3.6" height="3" fill="%s"/><rect x="35.2" y="10.5" width="3.6" height="3" fill="%s"/>' % (LICHT, LICHT)
    s += '<rect x="-38.1" y="-12.8" width="76.2" height="3" fill="%s" %s/>' % (DECKEL, LINIE)
    s += '<rect x="-4.7" y="-16.6" width="9.4" height="3.8" rx="1.2" fill="#9a9a9a" %s/>' % LINIE
    s += '<rect x="-38.1" y="-9.8" width="76.2" height="17.4" fill="%s" %s/>' % (GELB, LINIE)
    s += '<rect x="-38.1" y="7.6" width="76.2" height="14.9" fill="%s" %s/>' % (WANNE, LINIE)
    for x in (-31.7, 31.7):                                                                          # Schrauben
        s += '<circle cx="%s" cy="-6.1" r="2.6" fill="#b0b0b0" %s/>' % (f(x), LINIE)
    for i, x in ((2, -26.5), (3, 18.3)):                                                             # Ruecklichter
        s += '<rect x="%s" y="-8.4" width="8.2" height="8.1" fill="#b8b02a" %s/>' % (f(x), LINIE)
        s += '<rect class="led" id="rv%d" x="%s" y="-7.2" width="5.8" height="5.7" rx=".8"/>' % (i, f(x + 1.2))
    s += '<rect x="-7" y="-6.3" width="14" height="3" rx=".5" fill="#111"/>'                         # SD-Karte
    s += '<rect x="-5.7" y="-1.4" width="11.4" height="4.8" rx="2.2" fill="#111" stroke="#666" stroke-width=".5"/>'  # USB-C
    return s


def oben():
    """Live-Ansicht von oben, vorne = oben. Ring-Mitte (110,117) - die Ring-LEDs setzt das Skript."""
    s = '<defs><pattern id="profilV7" width="48" height="9" patternUnits="userSpaceOnUse">' \
        '<path d="M-4 5 q13 -6 26 0 t26 0" stroke="#4a4a4a" stroke-width="2.4" fill="none"/></pattern>' \
        '<filter id="durchschein" x="-50%" y="-50%" width="200%" height="200%"><feGaussianBlur stdDeviation="4.5"/></filter>' \
        '<radialGradient id="deckelglanz" cx="35%" cy="30%" r="80%"><stop offset="0" stop-color="#ffffff"/>' \
        '<stop offset="1" stop-color="#dcdcdc"/></radialGradient></defs>'
    s += '<text x="110" y="9" text-anchor="middle" font-size="9" fill="#9aa0a6">vorne</text>'
    for seite_, x, glow in (('L', 13, 'gl'), ('R', 171.5, 'gr')):
        for y in (37, 119.5):
            s += '<rect x="%s" y="%s" width="35.5" height="77.5" rx="12" fill="#151515" stroke="#000" stroke-width="1"/>' % (f(x), f(y))
            s += '<rect x="%s" y="%s" width="35.5" height="77.5" rx="12" fill="url(#profilV7)"/>' % (f(x), f(y))
            s += '<rect class="radglow %s" x="%s" y="%s" width="35.5" height="77.5" rx="12"/>' % (glow, f(x), f(y))
    for y in (74, 156.5):                                                                            # Achsen
        s += '<rect x="46" y="%s" width="4" height="7" fill="%s"/><rect x="170" y="%s" width="4" height="7" fill="%s"/>' % (f(y), LICHT, f(y), LICHT)
    s += '<rect x="48.3" y="55.3" width="123.4" height="123.4" rx="2" fill="url(#deckelglanz)" stroke="#222" stroke-width="1.2"/>'
    s += '<g id="ringleds" filter="url(#durchschein)"></g>'                                          # leuchtet durch den Deckel
    s += '<circle cx="110" cy="117" r="13.5" fill="#9a9a9a" stroke="#222" stroke-width="1"/>'      # Knopf in der Mitte
    s += '<circle cx="110" cy="117" r="10" fill="#a8a8a8" stroke="#555" stroke-width="1"/>'
    s += '<rect x="103" y="51.5" width="14" height="4" rx="1.5" fill="#111"/>'                       # Kamera vorne
    s += '<rect x="104" y="178.7" width="12" height="3.2" rx="1" fill="#111"/>'                      # SD/USB hinten
    for i, x, y in ((0, 67, 51.3), (1, 139.6, 51.3), (2, 67, 178.7), (3, 139.6, 178.7)):          # Lichter vorne/hinten
        s += '<rect class="led" id="lv%d" x="%s" y="%s" width="13.4" height="4" rx="1.5"/>' % (i, f(x), f(y))
    for pid, x in (('pfeilL', 30.75), ('pfeilR', 189.25)):                                          # Motortest-Pfeile
        s += '<g class="pfeil" id="%s"><path d="M%s 89 l15 18 h-8 v38 h-14 v-38 h-8 z" fill="#3ccf6e" stroke="#0b2a16" stroke-width="1.5"/></g>' % (pid, f(x))
    return '<svg id="liveauto" viewBox="0 0 220 228">' + s + '</svg>'


def logo(attr):
    return '<svg viewBox="-68 -27 136 67"%s>%s</svg>' % (attr, seite())


if __name__ == '__main__':
    pfad = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'include', 'webseite.h')
    t = open(pfad, encoding='utf-8').read()
    n0 = len(t)
    t, a = re.subn(r'<svg viewBox="-66 -18 132 60"([^>]*)>.*?</svg>', lambda m: logo(m.group(1)), t, flags=re.S)
    t, b = re.subn(r'<g id="g-nicken">.*?</g>', lambda m: '<g id="g-nicken">' + seite() + '</g>', t, flags=re.S)
    t, c = re.subn(r'<g id="g-rollen">.*?</g>', lambda m: '<g id="g-rollen">' + heck() + '</g>', t, flags=re.S)
    t, d = re.subn(r'<svg id="liveauto"[^>]*>.*?</svg>', lambda m: oben(), t, flags=re.S)
    print('ersetzt: Logo %d, Seite %d, Heck %d, oben %d  (%d -> %d Zeichen)' % (a, b, c, d, n0, len(t)))
    if (a, b, c, d) != (2, 1, 1, 1):
        raise SystemExit('Unerwartete Trefferzahl - nichts gespeichert')
    open(pfad, 'w', encoding='utf-8', newline='').write(t)
