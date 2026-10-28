# Erzeugt den Schaltplan "Roboauto mit Seeed Studio XIAO ESP32-C6" als SVG (A3 quer).
# Aufruf: python schaltplan_xiao.py  ->  Schaltplan_XIAO_ESP32-C6.svg
import html
import os

W, H = 1680, 1188  # A3 quer, 4 px pro mm
out = []

C_WIRE = '#1b7a3a'
C_BODY = '#8a1c1c'
C_FILL = '#fffbea'
C_TXT = '#1d1d1d'
C_REF = '#0b4f8a'
C_VAL = '#555555'
C_NET = '#6a1b9a'
C_PWR = '#b3261e'
C_GRID = '#3a3a3a'
FONT = 'Arial, Helvetica, sans-serif'


def esc(s):
    return html.escape(s, quote=True)


def add(s):
    out.append(s)


def poly(pts, color=C_WIRE, w=2.0, dash=None, fill='none'):
    d = ' '.join(f'{x},{y}' for x, y in pts)
    extra = f' stroke-dasharray="{dash}"' if dash else ''
    add(f'<polyline points="{d}" fill="{fill}" stroke="{color}" stroke-width="{w}" '
        f'stroke-linejoin="round" stroke-linecap="round"{extra}/>')


def wire(*pts):
    poly(pts, C_WIRE, 2.2)


def text(x, y, s, size=12, color=C_TXT, anchor='start', weight='normal', style='normal'):
    add(f'<text x="{x}" y="{y}" font-size="{size}" fill="{color}" text-anchor="{anchor}" '
        f'font-weight="{weight}" font-style="{style}">{esc(s)}</text>')


def rect(x, y, w, h, stroke=C_BODY, fill=C_FILL, sw=2.0, rx=0, dash=None):
    extra = f' stroke-dasharray="{dash}"' if dash else ''
    add(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{rx}" fill="{fill}" '
        f'stroke="{stroke}" stroke-width="{sw}"{extra}/>')


def junction(x, y):
    add(f'<circle cx="{x}" cy="{y}" r="4" fill="{C_WIRE}"/>')


def noconn(x, y):
    poly([(x - 6, y - 6), (x + 6, y + 6)], C_BODY, 2)
    poly([(x - 6, y + 6), (x + 6, y - 6)], C_BODY, 2)


def gnd(x, y):
    wire((x, y), (x, y + 12))
    poly([(x - 13, y + 12), (x + 13, y + 12)], C_BODY, 2.5)
    poly([(x - 8, y + 17), (x + 8, y + 17)], C_BODY, 2.5)
    poly([(x - 3, y + 22), (x + 3, y + 22)], C_BODY, 2.5)


def pwr(x, y, name):
    wire((x, y), (x, y - 14))
    poly([(x - 12, y - 14), (x + 12, y - 14)], C_PWR, 3)
    text(x, y - 21, name, 12, C_PWR, 'middle', 'bold')


def glabel(x, y, name, side):
    """Netzname als Fahne. (x, y) = Anschlusspunkt; side = Richtung, in die die Fahne zeigt."""
    w = len(name) * 7.0 + 14
    if side == 'left':
        pts = [(x, y), (x - 8, y - 9), (x - 8 - w, y - 9), (x - 8 - w, y + 9), (x - 8, y + 9), (x, y)]
        tx = x - 8 - w / 2
    else:
        pts = [(x, y), (x + 8, y - 9), (x + 8 + w, y - 9), (x + 8 + w, y + 9), (x + 8, y + 9), (x, y)]
        tx = x + 8 + w / 2
    poly(pts, C_NET, 1.6, fill='#ffffff')
    text(tx, y + 4, name, 11, C_NET, 'middle', 'bold')


def res_v(x, y1, y2, ref, val, side='right'):
    m = (y1 + y2) / 2
    wire((x, y1), (x, m - 20))
    rect(x - 7, m - 20, 14, 40)
    wire((x, m + 20), (x, y2))
    if side == 'right':
        text(x + 14, m - 3, ref, 12, C_REF, 'start', 'bold')
        text(x + 14, m + 13, val, 11, C_VAL)
    elif side == 'left':
        text(x - 14, m - 3, ref, 12, C_REF, 'end', 'bold')
        text(x - 14, m + 13, val, 11, C_VAL, 'end')


def res_h(x1, x2, y, ref, val):
    m = (x1 + x2) / 2
    wire((x1, y), (m - 20, y))
    rect(m - 20, y - 7, 40, 14)
    wire((m + 20, y), (x2, y))
    text(m, y - 13, f'{ref}  {val}', 11, C_REF, 'middle', 'bold')


def cap_v(x, y1, y2, ref=None, val=None, pol=False, side='right'):
    m = (y1 + y2) / 2
    wire((x, y1), (x, m - 5))
    poly([(x - 14, m - 5), (x + 14, m - 5)], C_BODY, 3)
    if pol:
        add(f'<path d="M{x - 14},{m + 9} Q{x},{m + 1} {x + 14},{m + 9}" fill="none" stroke="{C_BODY}" stroke-width="3"/>')
        text(x - 20, m - 9, '+', 13, C_BODY, 'middle', 'bold')
        wire((x, m + 5), (x, y2))
    else:
        poly([(x - 14, m + 5), (x + 14, m + 5)], C_BODY, 3)
        wire((x, m + 5), (x, y2))
    if ref and side == 'right':
        text(x + 20, m - 3, ref, 12, C_REF, 'start', 'bold')
        text(x + 20, m + 13, val, 11, C_VAL)


def battery(x, y1, y2):
    m = (y1 + y2) / 2
    wire((x, y1), (x, m - 6))
    poly([(x - 18, m - 6), (x + 18, m - 6)], C_BODY, 2.5)
    poly([(x - 10, m + 6), (x + 10, m + 6)], C_BODY, 5)
    wire((x, m + 6), (x, y2))
    text(x - 24, m - 10, '+', 14, C_BODY, 'middle', 'bold')


def switch(x1, x2, y):
    add(f'<circle cx="{x1}" cy="{y}" r="4" fill="#fff" stroke="{C_BODY}" stroke-width="2"/>')
    add(f'<circle cx="{x2}" cy="{y}" r="4" fill="#fff" stroke="{C_BODY}" stroke-width="2"/>')
    poly([(x1 + 3, y - 2), (x2 - 4, y - 20)], C_BODY, 2.5)


def motor(x, y1, y2, ref):
    m = (y1 + y2) / 2
    wire((x, y1), (x, m - 19))
    add(f'<circle cx="{x}" cy="{m}" r="19" fill="{C_FILL}" stroke="{C_BODY}" stroke-width="2"/>')
    text(x, m + 4, ref, 11.5, C_BODY, 'middle', 'bold')
    wire((x, m + 19), (x, y2))


def diode_v(x, y1, y2):
    """Kathode oben (y1)."""
    m = (y1 + y2) / 2
    wire((x, y1), (x, m - 10))
    poly([(x - 11, m - 10), (x + 11, m - 10)], C_BODY, 2.5)
    add(f'<polygon points="{x},{m - 10} {x - 11},{m + 10} {x + 11},{m + 10}" fill="{C_FILL}" '
        f'stroke="{C_BODY}" stroke-width="2" stroke-linejoin="round"/>')
    wire((x, m + 10), (x, y2))


def buzzer_v(x, y1, y2):
    m = (y1 + y2) / 2
    wire((x, y1), (x, m - 16))
    rect(x - 16, m - 16, 32, 32)
    add(f'<path d="M{x - 8},{m + 8} A9,9 0 0 1 {x + 8},{m + 8}" fill="none" stroke="{C_BODY}" stroke-width="2"/>')
    poly([(x - 10, m + 8), (x + 10, m + 8)], C_BODY, 2)
    text(x - 10, m - 4, '+', 11, C_BODY, 'middle', 'bold')
    wire((x, m + 16), (x, y2))


def npn(cx, cy):
    """Gibt Basis-, Kollektor- und Emitter-Anschlusspunkt zurueck."""
    add(f'<circle cx="{cx}" cy="{cy}" r="24" fill="{C_FILL}" stroke="{C_BODY}" stroke-width="2"/>')
    poly([(cx - 8, cy - 14), (cx - 8, cy + 14)], C_BODY, 3.5)
    wire((cx - 34, cy), (cx - 8, cy))
    poly([(cx - 8, cy - 6), (cx + 10, cy - 18), (cx + 10, cy - 40)], C_BODY, 2)
    poly([(cx - 8, cy + 6), (cx + 10, cy + 18), (cx + 10, cy + 40)], C_BODY, 2)
    add(f'<polygon points="{cx + 10},{cy + 18} {cx - 1},{cy + 16} {cx + 5},{cy + 9}" fill="{C_BODY}"/>')
    return (cx - 34, cy), (cx + 10, cy - 40), (cx + 10, cy + 40)


def ic(x, y, w, h, pins):
    """pins: (seite, position, nummer, name). Gibt {name: (x, y)} der Pin-Enden zurueck."""
    rect(x, y, w, h, sw=2.4)
    ends = {}
    for side, pos, num, name in pins:
        if side == 'L':
            px, py, ex, ey = x, y + pos, x - 30, y + pos
            text(x + 8, py + 4, name, 11.5, C_TXT)
            text(x - 15, py - 4, str(num), 9.5, C_VAL, 'middle')
        elif side == 'R':
            px, py, ex, ey = x + w, y + pos, x + w + 30, y + pos
            text(x + w - 8, py + 4, name, 11.5, C_TXT, 'end')
            text(x + w + 15, py - 4, str(num), 9.5, C_VAL, 'middle')
        elif side == 'T':
            px, py, ex, ey = x + pos, y, x + pos, y - 30
            text(px, y + 16, name, 11.5, C_TXT, 'middle')
            text(px + 5, y - 15, str(num), 9.5, C_VAL)
        else:
            px, py, ex, ey = x + pos, y + h, x + pos, y + h + 30
            text(px, y + h - 8, name, 11.5, C_TXT, 'middle')
            text(px + 5, y + h + 18, str(num), 9.5, C_VAL)
        poly([(px, py), (ex, ey)], C_BODY, 2)
        ends[name] = (ex, ey)
    return ends


def section(x1, y1, x2, y2, title):
    rect(x1, y1, x2 - x1, y2 - y1, stroke='#9a9a9a', fill='none', sw=1.2, rx=6, dash='6 4')
    add(f'<rect x="{x1 + 10}" y="{y1 - 9}" width="{len(title) * 7.4 + 14}" height="18" fill="#ffffff"/>')
    text(x1 + 17, y1 + 5, title, 12.5, '#4a4a4a', 'start', 'bold')


# ------------------------------------------------------------------ Rahmen
add(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}" font-family="{FONT}">')
add(f'<rect width="{W}" height="{H}" fill="#ffffff"/>')
rect(20, 20, W - 40, H - 40, stroke=C_GRID, fill='none', sw=1.5)
rect(40, 40, W - 80, H - 80, stroke=C_GRID, fill='none', sw=2.2)
for i in range(8):
    x = 40 + i * 200
    if i:
        poly([(x, 20), (x, 40)], C_GRID, 1.2)
        poly([(x, H - 40), (x, H - 20)], C_GRID, 1.2)
    text(x + 100, 35, str(i + 1), 12, C_GRID, 'middle', 'bold')
    text(x + 100, H - 25, str(i + 1), 12, C_GRID, 'middle', 'bold')
rh = (H - 80) / 6
for i, z in enumerate('ABCDEF'):
    y = 40 + i * rh
    if i:
        poly([(20, y), (40, y)], C_GRID, 1.2)
        poly([(W - 40, y), (W - 20, y)], C_GRID, 1.2)
    text(30, y + rh / 2 + 4, z, 12, C_GRID, 'middle', 'bold')
    text(W - 30, y + rh / 2 + 4, z, 12, C_GRID, 'middle', 'bold')

# ------------------------------------------------------------------ 1 Stromversorgung
section(60, 60, 560, 430, '1 · Stromversorgung & Akkumessung')
battery(130, 150, 215)
gnd(130, 215)
text(160, 170, 'BT1', 12, C_REF, 'start', 'bold')
text(160, 186, 'LiPo 1S 3,7 V', 11, C_VAL)
text(160, 201, '500–1000 mAh', 11, C_VAL)
wire((130, 150), (130, 120), (200, 120))
switch(200, 260, 120)
text(230, 146, 'SW1', 12, C_REF, 'middle', 'bold')
text(230, 161, 'Ein / Aus', 11, C_VAL, 'middle')
wire((260, 120), (520, 120))
pwr(520, 120, 'VBAT')
junction(420, 120)
res_v(420, 120, 240, 'R5', '200 kΩ')
junction(420, 240)
res_v(420, 240, 330, 'R6', '200 kΩ', side='left')
gnd(420, 330)
wire((420, 240), (500, 240))
cap_v(500, 240, 330, 'C3', '100 nF')
gnd(500, 330)
wire((420, 240), (360, 240))
glabel(360, 240, 'AKKU_MESS', 'left')
text(75, 380, 'Teiler 1:2 an D0 (ADC): 4,2 V Akku → 2,1 V am Pin', 11, C_TXT)
text(75, 397, 'Laden über USB-C am XIAO (Ladechip auf dem Modul) – nur bei SW1 = Ein', 11, C_TXT)
text(75, 414, 'VBAT versorgt XIAO (BAT-Pads), Motortreiber, LEDs und Buzzer', 11, C_TXT)

# ------------------------------------------------------------------ 2 Mikrocontroller
section(580, 60, 1120, 830, '2 · Mikrocontroller')
text(860, 104, 'U1', 13, C_REF, 'middle', 'bold')
text(860, 121, 'Seeed Studio XIAO ESP32-C6', 12, C_VAL, 'middle')
pins = [
    ('L', 40, 1, 'D0 · GPIO0 · A0'), ('L', 80, 2, 'D1 · GPIO1'), ('L', 120, 3, 'D2 · GPIO2'),
    ('L', 160, 4, 'D3 · GPIO21'), ('L', 200, 5, 'D4 · GPIO22 · SDA'), ('L', 240, 6, 'D5 · GPIO23 · SCL'),
    ('L', 280, 7, 'D6 · GPIO16'),
    ('R', 40, 14, '5V'), ('R', 80, 13, 'GND'), ('R', 120, 12, '3V3'), ('R', 160, 11, 'GPIO18 · D10'),
    ('R', 200, 10, 'GPIO20 · D9'), ('R', 240, 9, 'GPIO19 · D8'), ('R', 280, 8, 'GPIO17 · D7'),
    ('B', 60, '', 'BAT+'), ('B', 140, '', 'BAT−'),
]
u1 = ic(760, 140, 200, 330, pins)
rect(838, 140, 44, 12, stroke=C_BODY, fill='#e0e0e0', sw=1.4)
text(860, 166, 'USB-C', 9.5, C_VAL, 'middle')
for name, net in [('D0 · GPIO0 · A0', 'AKKU_MESS'), ('D1 · GPIO1', 'MOT_L_VOR'), ('D2 · GPIO2', 'MOT_L_RUECK'),
                  ('D3 · GPIO21', 'MOT_R_VOR'), ('D4 · GPIO22 · SDA', 'I2C_SDA'), ('D5 · GPIO23 · SCL', 'I2C_SCL'),
                  ('D6 · GPIO16', 'LED_HECK')]:
    ex, ey = u1[name]
    wire((ex, ey), (ex - 20, ey))
    glabel(ex - 20, ey, net, 'left')
for name, net in [('GPIO18 · D10', 'MOT_R_RUECK'), ('GPIO20 · D9', 'BUZZER'), ('GPIO19 · D8', 'LED_RING'),
                  ('GPIO17 · D7', 'LED_FRONT')]:
    ex, ey = u1[name]
    wire((ex, ey), (ex + 20, ey))
    glabel(ex + 20, ey, net, 'right')
ex, ey = u1['5V']
wire((ex, ey), (ex + 18, ey))
noconn(ex + 18, ey)
text(ex + 30, ey + 4, 'nur bei USB', 10, C_VAL)
ex, ey = u1['GND']
wire((ex, ey), (ex + 70, ey))
gnd(ex + 70, ey)
ex, ey = u1['3V3']
wire((ex, ey), (ex + 20, ey))
pwr(ex + 20, ey, '+3V3')
ex, ey = u1['BAT+']
wire((ex, ey), (ex, ey + 30), (ex - 60, ey + 30))
pwr(ex - 60, ey + 30, 'VBAT')
ex, ey = u1['BAT−']
wire((ex, ey), (ex, ey + 20))
gnd(ex, ey + 20)
text(860, 565, 'BAT+ / BAT− = Lötpads auf der Unterseite des XIAO', 10.5, C_VAL, 'middle', style='italic')

# Pin-Tabelle
tx0, ty0 = 610, 590
cols = [(tx0 + 8, 'XIAO'), (tx0 + 62, 'GPIO'), (tx0 + 128, 'Signal'), (tx0 + 252, 'Anschluss')]
rect(tx0, ty0, 490, 20, stroke='#9a9a9a', fill='#efe9d8', sw=1)
for cx, title in cols:
    text(cx, ty0 + 14, title, 11, C_TXT, 'start', 'bold')
rows = [
    ('D0', 'GPIO0', 'AKKU_MESS', 'R5/R6 Spannungsteiler (ADC)'),
    ('D1', 'GPIO1', 'MOT_L_VOR', 'U2 IN1 · links vorwärts'),
    ('D2', 'GPIO2', 'MOT_L_RUECK', 'U2 IN2 · links rückwärts'),
    ('D3', 'GPIO21', 'MOT_R_VOR', 'U2 IN3 · rechts vorwärts'),
    ('D10', 'GPIO18', 'MOT_R_RUECK', 'U2 IN4 · rechts rückwärts'),
    ('D4', 'GPIO22', 'I2C_SDA', 'U3 SDA (MPU-6050)'),
    ('D5', 'GPIO23', 'I2C_SCL', 'U3 SCL (MPU-6050)'),
    ('D8', 'GPIO19', 'LED_RING', 'LED1 DIN · 8× WS2812B'),
    ('D7', 'GPIO17', 'LED_FRONT', 'LED2 DIN · Frontlicht'),
    ('D6', 'GPIO16', 'LED_HECK', 'LED3 DIN · Rücklicht'),
    ('D9', 'GPIO20', 'BUZZER', 'Q1 Basis (über R10)'),
]
for i, row in enumerate(rows):
    y = ty0 + 20 + i * 18
    rect(tx0, y, 490, 18, stroke='#d6d6d6', fill='#ffffff' if i % 2 else '#faf7ee', sw=0.8)
    for (cx, _), val in zip(cols, row):
        text(cx, y + 13, val, 10.5, C_NET if val == row[2] else C_TXT, 'start', 'bold' if val == row[2] else 'normal')

# ------------------------------------------------------------------ 3 Motortreiber
section(60, 450, 560, 1140, '3 · Motortreiber & Antrieb')
u2 = ic(250, 600, 140, 410, [
    ('L', 40, 1, 'IN1'), ('L', 150, 2, 'IN2'), ('L', 260, 3, 'IN3'), ('L', 370, 4, 'IN4'),
    ('R', 40, 5, 'OUT1'), ('R', 150, 6, 'OUT2'), ('R', 260, 7, 'OUT3'), ('R', 370, 8, 'OUT4'),
    ('T', 70, '', 'VCC'), ('B', 70, '', 'GND'),
])
text(320, 790, 'U2', 13, C_REF, 'middle', 'bold')
text(320, 807, 'MX1508', 12, C_VAL, 'middle', 'bold')
text(320, 823, '2× H-Brücke', 10.5, C_VAL, 'middle')
ex, ey = u2['VCC']
wire((ex, ey), (ex, 530))
pwr(ex, 530, 'VBAT')
junction(ex, 548)
wire((ex, 548), (500, 548))
cap_v(430, 548, 598, pol=True)
gnd(430, 598)
cap_v(500, 548, 598)
gnd(500, 598)
text(430, 512, 'C1', 12, C_REF, 'middle', 'bold')
text(430, 527, '470 µF / 10 V', 10.5, C_VAL, 'middle')
text(505, 512, 'C2', 12, C_REF, 'middle', 'bold')
text(505, 527, '100 nF', 10.5, C_VAL, 'middle')
ex, ey = u2['GND']
gnd(ex, ey)
for i, (pin, net) in enumerate([('IN1', 'MOT_L_VOR'), ('IN2', 'MOT_L_RUECK'), ('IN3', 'MOT_R_VOR'), ('IN4', 'MOT_R_RUECK')]):
    ex, ey = u2[pin]
    wire((ex, ey), (175, ey))
    glabel(175, ey, net, 'left')
    junction(200, ey)
    res_v(200, ey, ey + 70, f'R{i + 1}', '10 kΩ', side='left')
    gnd(200, ey + 70)
for a, b, m1, m2, caption in [('OUT1', 'OUT2', 'M1', 'M2', 'N20 links, parallel'),
                              ('OUT3', 'OUT4', 'M3', 'M4', 'N20 rechts, parallel')]:
    (ax, ay), (bx, by) = u2[a], u2[b]
    wire((ax, ay), (520, ay))
    wire((bx, by), (520, by))
    motor(460, ay, by, m1)
    motor(520, ay, by, m2)
    junction(460, ay)
    junction(460, by)
    text(490, by + 30, caption, 11, C_TXT, 'middle', 'bold')
text(75, 1085, 'R1–R4 halten die Motoren beim Einschalten und Reset aus.', 11, C_TXT)
text(75, 1101, 'C1/C2 direkt an VCC/GND des MX1508 löten.', 11, C_TXT)
text(75, 1117, 'Falsche Drehrichtung: Motorkabel tauschen oder in der Firmware invertieren.', 11, C_TXT)

# ------------------------------------------------------------------ 4 Beleuchtung
section(1140, 60, 1620, 580, '4 · Beleuchtung (WS2812B)')
for y0, ref, net, name, sub in [(100, 'LED1', 'LED_RING', '8× WS2812B', 'Status-Ring'),
                                (240, 'LED2', 'LED_FRONT', '2× WS2812B', 'Frontlicht'),
                                (380, 'LED3', 'LED_HECK', '2× WS2812B', 'Rücklicht')]:
    e = ic(1380, y0, 160, 80, [('L', 40, 1, 'DIN'), ('R', 20, 2, 'VDD'), ('R', 60, 3, 'GND')])
    text(1452, y0 + 30, ref, 12.5, C_REF, 'middle', 'bold')
    text(1452, y0 + 47, name, 11, C_TXT, 'middle', 'bold')
    text(1452, y0 + 63, sub, 10, C_VAL, 'middle')
    ex, ey = e['DIN']
    rnum = {'LED1': 7, 'LED2': 8, 'LED3': 9}[ref]
    res_h(1270, ex, ey, f'R{rnum}', '330 Ω')
    glabel(1270, ey, net, 'left')
    ex, ey = e['VDD']
    pwr(ex, ey, 'VBAT')
    ex, ey = e['GND']
    gnd(ex, ey)
pwr(1200, 500, 'VBAT')
cap_v(1200, 500, 545, pol=True)
gnd(1200, 545)
text(1225, 515, 'C4', 12, C_REF, 'start', 'bold')
text(1253, 515, '100 µF / 10 V – Puffer für die LED-Versorgung,', 11, C_VAL)
text(1225, 531, 'direkt an LED1 (größter Verbraucher)', 11, C_VAL)

# ------------------------------------------------------------------ 5 Lagesensor
section(1140, 600, 1620, 810, '5 · Lagesensor')
u3 = ic(1380, 640, 160, 140, [('L', 25, '', 'SDA'), ('L', 55, '', 'SCL'), ('L', 85, '', 'AD0'),
                              ('L', 115, '', 'INT'), ('R', 25, '', 'VCC'), ('R', 115, '', 'GND')])
text(1472, 700, 'U3', 12.5, C_REF, 'middle', 'bold')
text(1472, 717, 'GY-521 (MPU-6050)', 11, C_TXT, 'middle', 'bold')
text(1472, 733, 'I²C 0x68', 10, C_VAL, 'middle')
glabel(u3['SDA'][0], u3['SDA'][1], 'I2C_SDA', 'left')
glabel(u3['SCL'][0], u3['SCL'][1], 'I2C_SCL', 'left')
for p, note in [('AD0', 'offen → 0x68'), ('INT', 'nicht benutzt')]:
    ex, ey = u3[p]
    noconn(ex, ey)
    text(ex - 12, ey + 4, note, 10, C_VAL, 'end')
pwr(u3['VCC'][0], u3['VCC'][1], '+3V3')
gnd(u3['GND'][0], u3['GND'][1])
text(1155, 800, 'Pull-ups 4,7 kΩ für SDA/SCL sind auf dem GY-521-Modul.', 10.5, C_VAL, style='italic')

# ------------------------------------------------------------------ 6 Signalgeber
section(580, 850, 1120, 1140, '6 · Signalgeber (Hupe)')
pwr(900, 895, 'VBAT')
wire((900, 895), (900, 910))
junction(900, 910)
buzzer_v(900, 910, 990)
text(878, 946, 'BZ1', 12, C_REF, 'end', 'bold')
text(878, 962, 'TMB12A05', 11, C_VAL, 'end')
wire((900, 910), (980, 910))
diode_v(980, 910, 990)
text(996, 946, 'D1', 12, C_REF, 'start', 'bold')
text(996, 962, '1N4148', 11, C_VAL)
wire((980, 990), (900, 990))
junction(900, 990)
b, c, e = npn(890, 1040)
wire((900, 990), c)
gnd(e[0], e[1])
text(930, 1036, 'Q1', 12, C_REF, 'start', 'bold')
text(930, 1052, 'S8050 / BC817 (NPN)', 11, C_VAL)
res_h(740, 820, 1040, 'R10', '1 kΩ')
wire((820, 1040), b)
junction(835, 1040)
res_v(835, 1040, 1100, 'R11', '100 kΩ', side='left')
gnd(835, 1100)
glabel(740, 1040, 'BUZZER', 'left')
text(600, 905, 'Der Magnetbuzzer braucht ca. 30 mA –', 11, C_TXT)
text(600, 921, 'mehr als ein GPIO liefern sollte.', 11, C_TXT)
text(600, 937, 'Q1 schaltet, D1 fängt die Spulen-', 11, C_TXT)
text(600, 953, 'spannung ab, R11 hält Q1 beim Start aus.', 11, C_TXT)

# ------------------------------------------------------------------ 7 Hinweise
section(1140, 830, 1620, 995, '7 · Hinweise')
for i, line in enumerate([
    '1. Der 5V-Pin des XIAO führt im Akkubetrieb keine Spannung –',
    '    LEDs, Motortreiber und Buzzer hängen deshalb an VBAT.',
    '2. D6 ist UART-TX: beim Einschalten kann das Rücklicht kurz',
    '    flackern (Boot-Ausgabe), danach läuft es normal.',
    '3. WS2812B an VBAT (3,0–4,2 V): 3,3-V-Datensignal genügt.',
    '4. Firmware: Pins in include/config.h auf diese Belegung',
    '    umstellen, Akku-Grenzwerte neu kalibrieren.',
]):
    text(1155, 862 + i * 17.5, line, 11, C_TXT)

# ------------------------------------------------------------------ Schriftfeld
tbx, tby, tbw, tbh = 1140, 1010, 500, 138
rect(tbx, tby, tbw, tbh, stroke=C_GRID, fill='#ffffff', sw=2)
poly([(1290, tby), (1290, tby + tbh)], C_GRID, 1.5)
text(1215, 1052, 'DAVESLAB', 18, '#111111', 'middle', 'bold')
text(1215, 1070, 'Ideen werden spielbar.', 10, C_VAL, 'middle', style='italic')
text(1215, 1128, 'github.com/ProD-byte/Roboauto', 9.5, C_VAL, 'middle')
text(1302, 1036, 'Roboauto – Schaltplan', 19, '#111111', 'start', 'bold')
text(1302, 1056, 'Seeed Studio XIAO ESP32-C6 · MX1508 · MPU-6050 · WS2812B', 11, C_VAL)
poly([(1290, 1068), (tbx + tbw, 1068)], C_GRID, 1.2)
poly([(1290, 1108), (tbx + tbw, 1108)], C_GRID, 1.2)
for x1, x2, y, label, val in [(1290, 1400, 1068, 'Blatt', '1 / 1'), (1400, 1510, 1068, 'Revision', 'A'),
                              (1510, 1640, 1068, 'Format', 'A3 quer'), (1290, 1460, 1108, 'Datum', '28.10.2026'),
                              (1460, 1640, 1108, 'Status', 'Entwurf – ungeprüft')]:
    if x1 > 1290:
        poly([(x1, y), (x1, y + 40)], C_GRID, 1.2)
    text(x1 + 8, y + 13, label, 9.5, C_VAL)
    text(x1 + 8, y + 31, val, 13, '#111111', 'start', 'bold')

add('</svg>')

ziel = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'Schaltplan_XIAO_ESP32-C6.svg')
with open(ziel, 'w', encoding='utf-8') as f:
    f.write('\n'.join(out))
print('geschrieben:', ziel)
