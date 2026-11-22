#!/usr/bin/env python3
"""Kaleidoscope — découpe les planches PNG de Ghislain (assets_src/) en éléments prêts pour l'interface (ui/assets/).

Pour chaque layout (retro, labo, holo, telescope) :
  chassis            châssis complet (le kaléidoscope est dessiné DESSOUS, visible par l'ouverture)
  cadre              cadre optique posé PAR-DESSUS l'animation (anneau seul, rogné au cercle)
  btn_on / btn_off   pilules des modes, texte effacé (le libellé est écrit par le code : net, traduisible)
  generer, decl, perso, layout   boutons principaux
  knob / cap         molette : anneau fixe + chapeau tourné par le code (repère vers le bas à 0)
  sel_preset, sel_menu, sel_menu2, master   sélecteurs et MASTER
  tog_on / tog_off   interrupteurs des moteurs
Écrit aussi ui/assets/meta.json (géométrie des chapeaux, couleurs des fenêtres de texte).
Dépendances : numpy, scipy, opencv-python, Pillow.
"""
import json, os
import numpy as np
import cv2
from PIL import Image
from scipy import ndimage

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC, OUT = os.path.join(ROOT, "assets_src"), os.path.join(ROOT, "ui", "assets")
L = {"retro": ("Retro_05", "Retro05", "KL_Retro05_Chassis_Complet.png", "KL_Retro05_Cadre_Lentille_Centrale.png"),
     "labo": ("Labo_Minimal_03", "Labo03", "KL_Labo03_Chassis_Complet.png", None),
     "holo": ("Holographique_04", "Holo04", "KL_Holo04_Chassis.png", "KL_Holo04_Cadre_Lentille_Optique.png"),
     "telescope": ("Telescope", "Telescope", "KL_Telescope_Chassis.png", "KL_Telescope_Cadre_Lentille_Optique.png")}
# chapeau des molettes : rayon relatif à la demi-largeur de la molette (mesuré sur les planches)
CAP = {"retro": .52, "labo": .655, "holo": .585, "telescope": .56}


def save(im, name, maxw=None, q=88):
    if maxw and im.width > maxw:
        im = im.resize((maxw, round(im.height * maxw / im.width)), Image.LANCZOS)
    im.save(os.path.join(OUT, name + ".webp"), "WEBP", quality=q, method=6)
    return im.size


def pieces(path):
    """les 4 éléments d'une planche, triés en lecture (haut→bas, gauche→droite)"""
    im = Image.open(path).convert("RGBA")
    a = np.array(im)[:, :, 3] > 20
    lab, n = ndimage.label(a)
    boxes = []
    for s in ndimage.find_objects(lab):
        if s is None: continue
        h, w = s[0].stop - s[0].start, s[1].stop - s[1].start
        if w * h > 5000: boxes.append((s[1].start, s[0].start, w, h))
    boxes.sort(key=lambda b: (round((b[1] + b[3] / 2) / 220), b[0]))
    return im, [im.crop((x, y, x + w, y + h)) for x, y, w, h in boxes]


def erase_text(im):
    """efface le texte gravé au centre d'une pilule : rectangle du texte rempli ligne par ligne
    (interpolation entre les bords gauche et droit + grain du fond) ; le code écrit ensuite le libellé"""
    rgba = np.array(im).astype(np.float32)
    rgb = rgba[:, :, :3]
    h, w = rgb.shape[:2]
    smooth = cv2.medianBlur(np.ascontiguousarray(rgb.astype(np.uint8)), 41).astype(np.float32)
    diff = np.abs(rgb - smooth).max(axis=2)
    box = np.zeros((h, w), bool); box[int(h * .3):int(h * .7), int(w * .27):int(w * .73)] = True
    m = (diff > 16) & box
    m = ndimage.binary_opening(m, iterations=1)
    ys, xs = np.nonzero(m)
    if len(xs) < 50: return im
    x0, x1 = max(int(w * .25), xs.min() - 10), min(int(w * .75), xs.max() + 10)
    y0, y1 = max(int(h * .28), ys.min() - 8), min(int(h * .72), ys.max() + 8)
    out = rgba.copy()
    t = np.linspace(0, 1, x1 - x0 + 1)[None, :, None]
    L = rgba[y0:y1 + 1, x0 - 3:x0].mean(axis=1, keepdims=True)
    R = rgba[y0:y1 + 1, x1 + 1:x1 + 4].mean(axis=1, keepdims=True)
    fill = L * (1 - t) + R * t
    # grain : bruit doux de même écart que le fond voisin (pas de motif répété)
    src = rgba[y0:y1 + 1, max(0, x0 - 40):x0 - 3, :3]
    if src.shape[1] > 8:
        sd = float((src - cv2.blur(src, (7, 7))).std())
        rng = np.random.default_rng(7)
        nz = cv2.GaussianBlur(rng.normal(0, sd * .7, (y1 - y0 + 1, x1 - x0 + 1)).astype(np.float32), (0, 0), 1.2)
        fill[:, :, :3] += nz[:, :, None]
    # fondu vertical aux bords haut/bas du rectangle
    v = np.ones(y1 - y0 + 1); e = min(8, len(v) // 4)
    v[:e] = np.linspace(0, 1, e); v[-e:] = np.linspace(1, 0, e)
    v = v[:, None, None]
    out[y0:y1 + 1, x0:x1 + 1] = out[y0:y1 + 1, x0:x1 + 1] * (1 - v) + fill * v
    out[:, :, 3] = rgba[:, :, 3]
    return Image.fromarray(np.clip(out, 0, 255).astype(np.uint8), "RGBA")


def circle_crop(im, cx, cy, r, feather=2.0):
    a = np.array(im).astype(np.float32)
    yy, xx = np.mgrid[0:im.height, 0:im.width]
    d = np.sqrt((xx - cx) ** 2 + (yy - cy) ** 2)
    m = np.clip((r - d) / feather + .5, 0, 1)
    a[:, :, 3] *= m
    x0, y0 = int(cx - r - 2), int(cy - r - 2)
    return Image.fromarray(a.astype(np.uint8), "RGBA").crop((x0, y0, x0 + int(2 * r + 4), y0 + int(2 * r + 4)))


def hole(chassis):
    """ouverture centrale du châssis (zone transparente entourée d'opaque) : centre et rayon"""
    a = np.array(chassis)[:, :, 3] < 30
    lab, n = ndimage.label(a)
    best = None
    sizes = ndimage.sum(a, lab, range(1, n + 1))
    for i in [int(j) + 1 for j in np.argsort(sizes)[::-1][:4]]:
        ys, xs = np.nonzero(lab == i)
        if xs.min() == 0 or ys.min() == 0 or xs.max() == a.shape[1] - 1 or ys.max() == a.shape[0] - 1: continue   # extérieur
        if best is None or len(xs) > best[0]: best = (len(xs), xs, ys)
    _, xs, ys = best
    cx, cy = (xs.min() + xs.max()) / 2, (ys.min() + ys.max()) / 2
    return cx, cy, (xs.max() - xs.min() + ys.max() - ys.min()) / 4


def main():
    os.makedirs(OUT, exist_ok=True)
    for f in os.listdir(OUT):
        if f.endswith(".webp") and f.split("_")[0] in L: os.remove(os.path.join(OUT, f))
    meta = {}
    for lid, (folder, tag, chassis_f, cadre_f) in L.items():
        d = os.path.join(SRC, folder); m = {}
        ch = Image.open(os.path.join(d, chassis_f)).convert("RGBA")
        cx, cy, r = hole(ch)
        m["hole"] = [round(cx / 2, 1), round(cy / 2, 1), round(r / 2, 1)]          # en unités d'interface (1000 x 1083)
        save(ch, f"{lid}_chassis", 1600, 86)
        if cadre_f:
            cd = Image.open(os.path.join(d, cadre_f)).convert("RGBA")
            # anneau seul : on ne garde que le disque de la bague (le plateau octogonal rétro déborderait sur les panneaux)
            a = np.array(cd)[:, :, 3] < 30
            lab, n = ndimage.label(a)
            sizes = ndimage.sum(a, lab, range(1, n + 1))
            inner = next(int(j) + 1 for j in np.argsort(sizes)[::-1]
                         if not (lab == int(j) + 1)[0].any() and not (lab == int(j) + 1)[:, 0].any())
            ys, xs = np.nonzero(lab == inner)
            icx, icy, ir = (xs.min() + xs.max()) / 2, (ys.min() + ys.max()) / 2, (xs.max() - xs.min() + ys.max() - ys.min()) / 4
            ring_r = ir * (1.16 if lid == "retro" else 1.6)
            ring = circle_crop(cd, icx, icy, min(ring_r, min(cd.size) / 2 - 3), 6)
            m["cadre"] = {"innerRatio": round(ir / (ring.width / 2), 4)}
            save(ring, f"{lid}_cadre", 1100, 88)
        # pilules des modes : on = SOLO allumé, off = DROP éteint, texte effacé
        _, mp = pieces(os.path.join(d, f"KL_{tag}_Boutons_Modes_01-04.png"))
        on, off = erase_text(mp[0]), erase_text(mp[3])
        m["pill"] = list(save(on, f"{lid}_btn_on", 480)); save(off, f"{lid}_btn_off", 480)
        _, pp = pieces(os.path.join(d, f"KL_{tag}_Boutons_Principaux_01-04.png"))
        for nm, im in zip(["generer", "decl", "perso", "layout"], pp):
            m[nm] = list(save(im, f"{lid}_{nm}", 480 if nm == "generer" else 240))
        _, kp = pieces(os.path.join(d, f"KL_{tag}_Boutons_Rotatifs_01-04.png"))
        knob = kp[2]                                     # repère vers le bas
        k = np.array(knob)[:, :, 3] > 30; ys, xs = np.nonzero(k)
        kcx, kcy = (xs.min() + xs.max()) / 2, (ys.min() + ys.max()) / 2
        cr = CAP[lid] * (xs.max() - xs.min()) / 2
        cap = circle_crop(knob, kcx, kcy, cr, 1.5)
        save(knob, f"{lid}_knob", 240); save(cap, f"{lid}_cap", round(240 * cap.width / knob.width))
        m["knob"] = {"w": knob.width, "h": knob.height, "cx": round(kcx / knob.width, 4), "cy": round(kcy / knob.height, 4), "r": round(cr / knob.width, 4)}
        _, sp = pieces(os.path.join(d, f"KL_{tag}_Selecteurs_Preset_Style_Humeur_Master_01-04.png"))
        names = ["sel_preset", "sel_menu", "sel_menu2", "master"]
        if lid in ("holo", "telescope"): names = ["sel_preset", "sel_menu", "sel_menu2", "master"]   # TL preset, TR style, BL humeur, BR master
        for nm, im in zip(names, sp):
            m[nm] = list(save(im, f"{lid}_{nm}", 480))
        if lid in ("retro", "labo"):
            # MASTER en glissière : on détache le curseur (déplacé par le code) et on efface sa place sur le rail
            sl = sp[3]; W, H = sl.size
            tcx, tcy, tr = .636 * W, .505 * H, .072 * W
            thumb = circle_crop(sl, tcx, tcy, tr, 1.5)
            a = np.array(sl).astype(np.float32)
            xa, xb = int(tcx - tr * 1.35), int(tcx + tr * 1.35)
            ya, yb = int(tcy - tr * 1.5), int(tcy + tr * 1.5)
            t = np.linspace(0, 1, xb - xa + 1)[None, :, None]
            a[ya:yb + 1, xa:xb + 1] = a[ya:yb + 1, xa - 2:xa - 1] * (1 - t) + a[ya:yb + 1, xb + 1:xb + 2] * t
            save(Image.fromarray(np.clip(a, 0, 255).astype(np.uint8), "RGBA"), f"{lid}_master", 480)
            save(thumb, f"{lid}_thumb", 90)
            m["slider"] = {"x0": .085, "x1": .915, "cy": .505, "thumb": round(2 * tr / W, 4)}
        if lid in ("holo", "telescope"):
            # MASTER en cadran : chapeau détaché, tourné par le code (repère vers le haut au milieu de la course)
            ms = Image.open(os.path.join(OUT, f"{lid}_master.webp")).convert("RGBA")
            fx, fy, fr = {"holo": (225 / 447, 190 / 304, 84 / 447), "telescope": (238 / 480, 158 / 299, 70 / 480)}[lid]
            save(circle_crop(ms, fx * ms.width, fy * ms.height, fr * ms.width, 1.5), f"{lid}_mcap", 200)
            m["dial"] = {"cx": round(fx, 4), "cy": round(fy, 4), "r": round(fr, 4)}
        _, tp = pieces(os.path.join(d, f"KL_{tag}_Toggles_Moteurs_01-04.png"))
        m["tog"] = list(save(tp[0], f"{lid}_tog_on", 200)); save(tp[1], f"{lid}_tog_off", 200)
        meta[lid] = m
        print(lid, m["hole"], "ok")
    json.dump(meta, open(os.path.join(OUT, "meta.json"), "w"), indent=1)
    tot = sum(os.path.getsize(os.path.join(OUT, f)) for f in os.listdir(OUT) if f.endswith(".webp"))
    print("ui/assets :", round(tot / 1e6, 2), "Mo")


if __name__ == "__main__":
    main()
