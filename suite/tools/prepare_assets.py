#!/usr/bin/env python3
"""Room Labs Audio — découpe le pack graphique (assets_src/) pour les plugins et l'application.

Lit Inventaire_PNG.csv (zones utiles) et la charte (cases des feuilles de commandes) :
  assets/<produit>/<theme>_face.webp     fond + contour assemblés, recadrés sur la zone utile
  assets/<produit>/<theme>_cadres.webp   cadres d'écrans (posés au-dessus des écrans dynamiques)
  assets/_commandes/<theme>_knob1..4.webp, _key1..4, _led1..4, _pad1..4   sprites partagés
  assets/meta.json                       tailles, cadres d'écrans détectés (coordonnées de la face), pivots
Produits : VST/<plugin>, Standalone/<écran>, Rack/Chassis, Rack/Modules/<module>.
"""
import csv, json, os
import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "assets_src")
OUT = os.path.join(ROOT, "assets")
THEMES = ["Vert", "Studio", "Pocket"]
KNOBS = [((130, 220), 80), ((380, 220), 71), ((630, 220), 62), ((880, 220), 53)]   # centres et rayons extérieurs (charte)
KEYS = [(60, 160), (300, 160), (540, 160), (780, 160)]; KEY_WH = (175, 87)
LEDS = [(120, 120), (360, 120), (600, 120), (840, 120)]; LED_WH = (33, 33)
PADS = [(86, 160), (306, 160), (526, 160), (746, 160)]; PAD_WH = (181, 161)


def slug(s):
    return s.lower().replace(" ", "_")


def save(im, path, q=90):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    im.save(path, "WEBP", quality=q, method=5)


def frames_of(img, rect):
    """cadres d'écrans = composantes opaques de la feuille 006, en coordonnées de la face"""
    a = np.array(img)[:, :, 3] > 10
    lab, n = ndimage.label(a)
    out = []
    for s in ndimage.find_objects(lab):
        x0, y0, x1, y1 = s[1].start, s[0].start, s[1].stop, s[0].stop
        if (x1 - x0) * (y1 - y0) < 900: continue
        out.append([x0 - rect[0], y0 - rect[1], x1 - x0, y1 - y0])
    out.sort(key=lambda b: (round(b[1] / 20), b[0]))
    return out


def main():
    rows = list(csv.DictReader(open(os.path.join(SRC, "Inventaire_PNG.csv"), encoding="utf-8-sig")))
    meta = {"products": {}, "controls": {}}
    groups = {}
    for r in rows:
        p = r["fichier"].split("/")
        if p[1] == "VST": prod, theme = "vst/" + slug(p[2]), p[3]
        elif p[1] == "Standalone" and not p[2].startswith("_"): prod, theme = "app/" + slug(p[2]), p[3]
        elif p[1] == "Rack" and p[2] == "Chassis": prod, theme = "rack/chassis", p[3]
        elif p[1] == "Rack" and p[2] == "Modules": prod, theme = "rack/" + slug(p[3]), "Vert"
        else: continue
        groups.setdefault((prod, theme), {})[r["role"]] = (os.path.join(SRC, r["fichier"]), [int(r["x"]), int(r["y"]), int(r["largeur"]), int(r["hauteur"])])
    for (prod, theme), g in sorted(groups.items()):
        rect = g["fond"][1]
        x, y, w, h = rect
        face = Image.new("RGBA", (1020, 1200), (0, 0, 0, 0))
        for role in ("fond", "contour", "accent_rack"):
            if role in g: face.alpha_composite(Image.open(g[role][0]).convert("RGBA"))
        save(face.crop((x, y, x + w, y + h)), os.path.join(OUT, prod, f"{theme.lower()}_face.webp"))
        m = meta["products"].setdefault(prod, {"size": [w, h], "frames": [], "themes": []})
        m["themes"].append(theme.lower())
        if "cadres_ecrans" in g:
            cad = Image.open(g["cadres_ecrans"][0]).convert("RGBA")
            save(cad.crop((x, y, x + w, y + h)), os.path.join(OUT, prod, f"{theme.lower()}_cadres.webp"))
            m["frames"] = frames_of(cad, rect)
        if "pads" in g:
            m["pads"] = True
            pads = Image.open(g["pads"][0]).convert("RGBA")
            for i, (px, py) in enumerate(PADS):
                save(pads.crop((px, py, px + PAD_WH[0], py + PAD_WH[1])), os.path.join(OUT, "_commandes", f"{theme.lower()}_pad{i + 1}.webp"))
    # commandes partagées (identiques pour tous les plugins d'un thème)
    for theme in THEMES:
        base = os.path.join(SRC, "Assets", "VST", "Noise_Tools", theme)
        kn = Image.open(os.path.join(base, "003_molettes.png")).convert("RGBA")
        for i, ((cx, cy), r) in enumerate(KNOBS):
            m = r + 14
            save(kn.crop((cx - m, cy - m, cx + m, cy + m)), os.path.join(OUT, "_commandes", f"{theme.lower()}_knob{i + 1}.webp"))
        ky = Image.open(os.path.join(base, "004_touches.png")).convert("RGBA")
        for i, (kx, ky0) in enumerate(KEYS):
            save(ky.crop((kx, ky0, kx + KEY_WH[0], ky0 + KEY_WH[1])), os.path.join(OUT, "_commandes", f"{theme.lower()}_key{i + 1}.webp"))
            if i in (1, 3):   # touche allumée : variante sans le point central (le libellé prend la place, le voyant est redessiné à gauche)
                k2 = ky.crop((kx, ky0, kx + KEY_WH[0], ky0 + KEY_WH[1])).convert("RGBA"); a = np.array(k2)
                cy2, cx2 = KEY_WH[1] // 2, KEY_WH[0] // 2
                a[cy2 - 12:cy2 + 13, cx2 - 12:cx2 + 13] = a[cy2 - 12:cy2 + 13, cx2 + 20:cx2 + 45]
                save(Image.fromarray(a), os.path.join(OUT, "_commandes", f"{theme.lower()}_key{i + 1}n.webp"))
        ld = Image.open(os.path.join(base, "005_leds.png")).convert("RGBA")
        for i, (lx, ly) in enumerate(LEDS):
            save(ld.crop((lx, ly, lx + LED_WH[0], ly + LED_WH[1])), os.path.join(OUT, "_commandes", f"{theme.lower()}_led{i + 1}.webp"))
        # icônes du standalone
        ic = os.path.join(SRC, "Assets", "Standalone", "_Icones", theme)
        if os.path.isdir(ic):
            for f in os.listdir(ic):
                im = Image.open(os.path.join(ic, f)).convert("RGBA").crop((430, 535, 430 + 161, 535 + 117))
                save(im, os.path.join(OUT, "_icones", f"{theme.lower()}_{slug(f[4:-4])}.webp"))
    meta["controls"] = {"knobs": [{"r": r, "margin": 14, "size": 2 * (r + 14)} for _, r in KNOBS], "key": list(KEY_WH), "led": list(LED_WH), "pad": list(PAD_WH)}
    json.dump(meta, open(os.path.join(OUT, "meta.json"), "w"), indent=1)
    tot = sum(os.path.getsize(os.path.join(dp, f)) for dp, _, fs in os.walk(OUT) for f in fs)
    print(len(meta["products"]), "produits ;", round(tot / 1e6, 1), "Mo")


if __name__ == "__main__":
    main()
