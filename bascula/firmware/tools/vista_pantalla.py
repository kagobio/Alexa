#!/usr/bin/env python3
"""Vista previa de la pantalla simplificada del firmware, dibujada con las FUENTES REALES (.h) y las mismas coordenadas que drawMain().

Sirve para detectar solapes y desalineaciones sin la placa. NO es TFT_eSPI: reproduce a mano la geometria (si cambias drawMain, cambia tambien aqui).
Uso:  python3 -I vista_pantalla.py  [salida.png]
"""
import re, struct, math, sys, os
from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__)); F = os.path.join(HERE, "..", "CoffeeScale") + os.sep

def load(f, n):
    s = open(f).read(); body = s[s.index("{", s.index(n)):]
    return bytes(int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{2})", body))

class VLW:  # fuente suave de TFT_eSPI (cabecera de 24 bytes + 28 por glifo + mapas de 8 bits)
    def __init__(s, f, n):
        b = load(F + f, n); gc, ver, size, mp, asc, desc = struct.unpack(">6I", b[:24]); s.asc, s.desc = asc, desc
        off = 24; ents = []
        for i in range(gc):
            e = list(struct.unpack(">7I", b[off:off + 28])); off += 28
            for k in (4, 5):
                if e[k] >= 1 << 31: e[k] -= 1 << 32
            ents.append(e)
        pos = off; s.g = {}
        for u, h, w, adv, dy, dx, _ in ents: s.g[u] = (h, w, adv, dy, dx, b[pos:pos + h * w]); pos += h * w
    def h(s): return s.asc + s.desc
    def w(s, t): return sum(s.g[ord(c)][2] for c in t)
    def draw(s, img, x, y, t, col, datum="TL"):
        if datum == "TR": x -= s.w(t)
        px = img.load(); cx = x; base = y + s.asc
        for c in t:
            h, w, adv, dy, dx, bm = s.g[ord(c)]
            for r in range(h):
                for q in range(w):
                    a = bm[r * w + q]
                    if a:
                        X, Y = cx + dx + q, base - dy + r
                        if 0 <= X < img.width and 0 <= Y < img.height:
                            bg = px[X, Y]; px[X, Y] = tuple(int(bg[i] + (col[i] - bg[i]) * a / 255) for i in range(3))
            cx += adv

j96 = VLW("Jersey10-Digits96.h", "Jersey10Digits96"); b22 = VLW("TrackerSans-Bold22.h", "TrackerSansBold22"); b36 = VLW("TrackerSans-Bold36.h", "TrackerSansBold36")
TY = 36   # = ty de drawMain() en modo simplificado

def frame(state, sec, tenth, weight, mode, pct, right=None, rcol=(214, 214, 214), level=0.0, ok=False, armed=False):
    img = Image.new("RGB", (320, 172), (0, 0, 0)); d = ImageDraw.Draw(img)
    if level > 0.004:
        bottom = 172; surf = bottom - (bottom - 6) * min(level, 1.0)
        col = (16, 48, 24) if ok else (43, 27, 14); line = (63, 211, 106) if ok else (200, 140, 68)
        for x in range(320):
            y = int(surf + 1.2 * math.sin(x * 0.045) + 0.7 * math.sin(x * 0.11)); d.line([(x, y), (x, bottom)], fill=col); d.point((x, y), fill=line); d.point((x, y + 1), fill=line)
    if state != "BREW": b22.draw(img, 14, 4, mode, (207, 207, 207))
    if pct is not None:
        c = (34, 221, 0) if pct > 30 else ((255, 165, 0) if pct > 15 else (240, 0, 0))
        d.rounded_rectangle([270, 9, 298, 21], radius=2, outline=c); d.rectangle([298, 12, 299, 17], fill=c); d.rectangle([272, 11, 272 + max(0, int(pct * 24 / 100) - 1), 19], fill=c)
    tb = f"{sec // 60:02d}:{sec % 60:02d}"; tw = j96.w(tb); j96.draw(img, (320 - tw) // 2, TY, tb, (0, 255, 0) if ok else (255, 255, 255))
    if state in ("BREW", "DONE"):
        d.ellipse([(320 + tw) // 2 + 8 - 3, TY + 48 - 3, (320 + tw) // 2 + 8 + 3, TY + 48 + 3], fill=(138, 138, 138)); b22.draw(img, (320 + tw) // 2 + 15, TY + 34, str(tenth), (138, 138, 138))
    t = int(abs(weight) * 10 + 0.5); ip = str(t // 10); h = b36.h(); b36.draw(img, 14, 118, ip, (255, 255, 255)); w = b36.w(ip)
    d.rectangle([14 + w + 3, 118 + h - 6, 14 + w + 7, 118 + h - 2], fill=(255, 255, 255)); fr = str(t % 10)
    b36.draw(img, 14 + w + 12, 118, fr, (255, 255, 255)); b22.draw(img, 14 + w + 16 + b36.w(fr), 118 + h - 17, "g", (123, 125, 123))
    if right: b22.draw(img, 306, 126, right, rcol, "TR")
    if state == "READY":
        if armed: d.ellipse([247, 12, 253, 18], fill=(0, 255, 0))
        else: d.ellipse([247, 12, 253, 18], outline=(123, 125, 123))
    return img

if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "pantalla_firmware_simulada.png"
    S = [frame("READY", 0, 0, 0.0, "Espresso", 78, right="/ 36 g", rcol=(138, 138, 140)),
         frame("BREW", 14, 2, 21.6, "Espresso", 78, level=21.6 / 36),
         frame("DONE", 26, 5, 36.0, "Espresso", 78, right="1:2.0", level=1.0, ok=True),
         frame("READY", 0, 0, 0.0, "Prensa francesa", 12, right="/ 450 g", rcol=(138, 138, 140), armed=True)]
    W = Image.new("RGB", (320 * 2 + 30, 172 * 2 + 30), (250, 248, 244))
    for i, im in enumerate(S): W.paste(im, (10 + (i % 2) * 330, 10 + (i // 2) * 182))
    W.resize((W.width * 2, W.height * 2), Image.NEAREST).save(out); print(out)
