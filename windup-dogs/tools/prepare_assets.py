#!/usr/bin/env python3
"""Windup Dogs : prépare les images du pack Windup_Dogs_Assets pour l'interface (scène 960 × 640).
Usage : python3 tools/prepare_assets.py <dossier Windup_Dogs_Assets> ui/assets
- fond (01) : les pièces animées (chien batteur, clé, clochettes, boutons des curseurs) sont découpées en calques
  et effacées du fond ; la vitrine des engrenages est vidée (les vrais mécanismes animés y sont posés).
- mécanismes (05 à 09) : chaque pièce mobile est séparée de son support (engrenages du train, spirale du ressort,
  clés, cylindre sans picots, levier du marteau) avec son axe ; mesures dans mechanisms.json.
- commandes (02 à 04) : molettes (couronne fixe + chapeau qui tourne), boutons allumés, bille de curseur, chiens, médaillons."""
import json, math, pathlib, sys
import numpy as np
import cv2
from PIL import Image, ImageDraw, ImageFilter
from scipy import ndimage as nd

SRC, OUT = pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2]); OUT.mkdir(parents=True, exist_ok=True)
K = 960 / 1536            # échelle planche -> scène
meta = {}

def save(im, name, q=90): im.save(OUT / f"{name}.webp", "WEBP", quality=q, method=6)
def rgba(f): return Image.open(SRC / f).convert("RGBA")

# =============================================================================
# 1. Fond : calques découpés par différence avec une version « effacée »
# =============================================================================
plate = np.array(Image.open(SRC / "01_Wind-Up_Dogs_Main_Interface.png").convert("RGB"))
work = plate.copy()

def rowfill(img, hole):
    """remplit le trou ligne par ligne : interpolation entre les bords gauche et droit (moyenne sur 6 px),
    puis grain du mur ajouté ; donne un mur continu au lieu des taches d'un inpaint classique."""
    out = img.astype(float).copy(); rng = np.random.default_rng(3)
    hm = hole > 0
    for y in range(img.shape[0]):
        row = hm[y]
        if not row.any(): continue
        xs = np.flatnonzero(row); runs = np.split(xs, np.flatnonzero(np.diff(xs) > 1) + 1)
        for r in runs:
            a, b = r[0] - 1, r[-1] + 1
            if a < 0 or b >= img.shape[1]: continue
            ya, yb = max(0, y - 2), min(img.shape[0], y + 3)
            ca = img[ya:yb, max(0, a - 6):a + 1].reshape(-1, 3).mean(0); cb = img[ya:yb, b:b + 7].reshape(-1, 3).mean(0)
            t = (r - a) / (b - a); out[y, r] = ca[None] * (1 - t[:, None]) + cb[None] * t[:, None]
    blur = cv2.GaussianBlur(out, (0, 0), 2.2)
    grain = img.astype(float) - cv2.GaussianBlur(img.astype(float), (0, 0), 3)
    ring = cv2.dilate(hole, np.ones((31, 31), np.uint8)).astype(bool) & ~cv2.dilate(hole, np.ones((5, 5), np.uint8)).astype(bool)
    sd = np.median(np.abs(grain[ring])) * 1.2
    res = np.where(hm[..., None], blur + cv2.GaussianBlur(rng.normal(0, sd, img.shape[:2]), (0, 0), .7)[..., None], img)
    return np.clip(res, 0, 255).astype(np.uint8)

def lift(name, box, thr=34, grow=5):
    """découpe l'objet dans box : masque = écart avec le fond reconstruit ; efface l'objet du fond."""
    global work
    x0, y0, x1, y1 = box
    m = np.zeros(plate.shape[:2], np.uint8); m[y0:y1, x0:x1] = 255
    rough = cv2.inpaint(plate, m, 9, cv2.INPAINT_TELEA)
    diff = np.abs(plate.astype(int) - rough.astype(int)).sum(2)
    obj = np.zeros_like(m); obj[y0:y1, x0:x1] = (diff[y0:y1, x0:x1] > thr) * 255
    obj = cv2.morphologyEx(obj, cv2.MORPH_CLOSE, np.ones((7, 7), np.uint8))
    lab, n = nd.label(obj > 0); sizes = nd.sum(obj > 0, lab, range(1, n + 1))
    keep = np.isin(lab, [i + 1 for i, s in enumerate(sizes) if s > 120]); obj = (nd.binary_fill_holes(keep) * 255).astype(np.uint8)
    hole = cv2.dilate(obj, np.ones((grow * 2 + 1,) * 2, np.uint8))
    work = rowfill(work, hole)
    a = cv2.GaussianBlur(cv2.dilate(obj, np.ones((3, 3), np.uint8)), (3, 3), 0)
    spr = np.dstack([plate, a])[y0:y1, x0:x1]
    save(Image.fromarray(spr), name); meta[name] = {"box": [round(v * K, 2) for v in box]}
    return obj

lift("chien_batteur", (246, 96, 502, 319))
lift("cle_haut", (598, 136, 838, 257))
lift("cloche_g", (856, 211, 954, 292), thr=40)
lift("cloche_d", (970, 211, 1070, 292), thr=40)

# boutons des curseurs : la fente se reconstruit avec les lignes situées plus bas (fente verticale régulière)
SL = [(320, 697), (368, 713), (416, 685)]; R = 23
for cx, cy in SL:
    src = plate[cy + R + 4: cy + 3 * R + 5, cx - R - 6: cx + R + 7]
    work[cy - R: cy + R + 1, cx - R - 6: cx + R + 7] = src
    m = np.zeros(plate.shape[:2], np.uint8); cv2.rectangle(m, (cx - R - 6, cy - R - 2), (cx + R + 6, cy - R + 2), 255, -1); cv2.rectangle(m, (cx - R - 6, cy + R - 2), (cx + R + 6, cy + R + 2), 255, -1)
    work = cv2.inpaint(work, m, 3, cv2.INPAINT_TELEA)
meta["curseurs"] = {"x": [round(x * K, 2) for x, _ in SL], "top": round(650 * K, 2), "bottom": round(790 * K, 2)}

# vitrine des engrenages : verre sombre vide (les mécanismes animés sont posés dessus)
WIN = (494, 349, 1047, 539)
glass = Image.fromarray(work).convert("RGBA"); ov = Image.new("RGBA", glass.size, (0, 0, 0, 0)); d = ImageDraw.Draw(ov)
for i in range(WIN[3] - WIN[1]):
    t = i / (WIN[3] - WIN[1]); c = (int(10 + 10 * (1 - t)), int(26 + 14 * (1 - t)), int(28 + 14 * (1 - t)), 255)
    d.line([(WIN[0], WIN[1] + i), (WIN[2], WIN[1] + i)], fill=c)
mask = Image.new("L", glass.size, 0); ImageDraw.Draw(mask).rounded_rectangle(WIN, radius=30, fill=255)
glass.paste(ov, (0, 0), mask.filter(ImageFilter.GaussianBlur(1.2)))
work = np.array(glass.convert("RGB"))
meta["vitrine"] = [round(v * K, 2) for v in WIN]
Image.fromarray(work).save(OUT / "fond.webp", "WEBP", quality=88, method=6)

# reflet de la vitre (posé par-dessus les mécanismes) : reprend les reflets clairs de la planche d'origine
win = plate[WIN[1]:WIN[3], WIN[0]:WIN[2]].astype(float); lum = win.mean(2)
hl = np.clip((lum - 150) / 70, 0, 1); top = np.linspace(1, .25, hl.shape[0])[:, None]
refl = np.zeros(hl.shape + (4,), np.uint8); refl[..., :3] = 255; refl[..., 3] = (np.clip(hl * top * .55 + top * .06, 0, 1) * 255 * 0).astype(np.uint8)
# reflet doux dessiné : bande oblique claire en haut de la vitre
h, w = hl.shape; yy, xx = np.mgrid[0:h, 0:w]
band = np.exp(-((yy - 22 - (xx * .05)) / 14) ** 2) * .22 + np.exp(-((yy - 6) / 5) ** 2) * .18
refl[..., 3] = (np.clip(band, 0, 1) * 255).astype(np.uint8)
rm = np.array(mask.crop(WIN).filter(ImageFilter.GaussianBlur(1)))
refl[..., 3] = (refl[..., 3].astype(float) * rm / 255).astype(np.uint8)
save(Image.fromarray(refl), "vitre_reflet")

# =============================================================================
# 2. Train d'engrenages (06) : 5 roues séparées, axes et dents mesurés, sens corrects
# =============================================================================
im6 = np.array(rgba("06_Train_Engrenages.png")); rgb6 = im6[..., :3].astype(int)
G = [("g1", 487, 567, 278), ("g2", 895, 447, 190), ("g3", 1163, 430, 80), ("g4", 810, 729, 120), ("g5", 1138, 677, 175)]
r_, g_, b_ = rgb6[..., 0], rgb6[..., 1], rgb6[..., 2]
metal = (r_ > g_ + 8) & (r_ > b_ + 35) & (r_ > 70) & (im6[..., 3] > 200)
H6, W6 = metal.shape; yy, xx = np.mgrid[0:H6, 0:W6]
# centre affiné : barycentre du métal dans 0,75 R (roue symétrique)
gears = []
for name, cx, cy, ro in G:
    d = np.hypot(xx - cx, yy - cy); sel = metal & (d < ro * .75)
    gears.append([name, float(xx[sel].mean()), float(yy[sel].mean()), ro])
# attribution de chaque pixel à la roue la plus proche relativement à son rayon (zone d'engrènement partagée)
score = np.stack([np.hypot(xx - cx, yy - cy) / ro for _, cx, cy, ro in gears])
owner = score.argmin(0); inside = score.min(0) < 1.02
gear_px = metal & inside
info = []
for k, (name, cx, cy, ro) in enumerate(gears):
    m = (gear_px & (owner == k)).astype(np.uint8) * 255
    m = cv2.morphologyEx(m, cv2.MORPH_CLOSE, np.ones((5, 5), np.uint8)); m = (nd.binary_fill_holes(m > 0) & (np.hypot(xx - cx, yy - cy) < ro * 1.02) & ~((~metal) & (np.hypot(xx - cx, yy - cy) > ro * .55) & (np.hypot(xx - cx, yy - cy) < ro * .9) & (owner == k) & False)) * 255
    m = m.astype(np.uint8)
    # trous et rayons : garder le fond visible à travers (pas de métal = transparent) sauf petites ombres
    m = np.where(metal | (np.hypot(xx - cx, yy - cy) < ro * .2), m, 0).astype(np.uint8)
    m = cv2.morphologyEx(m, cv2.MORPH_CLOSE, np.ones((3, 3), np.uint8))
    # dents : comptées sur un cercle dans la zone des dents
    ang = np.linspace(0, 2 * np.pi, 1440, endpoint=False); rr = ro - 14
    px = np.clip((cx + rr * np.cos(ang)).astype(int), 0, W6 - 1); py = np.clip((cy + rr * np.sin(ang)).astype(int), 0, H6 - 1)
    s = nd.median_filter(metal[py, px].astype(int), size=15, mode="wrap"); teeth = int(np.sum(np.diff(np.r_[s, s[0]]) == 1))
    R = int(ro + 6); x0, y0 = int(cx) - R, int(cy) - R
    spr = np.zeros((2 * R, 2 * R, 4), np.uint8)
    sx0, sy0 = max(0, x0), max(0, y0); sx1, sy1 = min(W6, x0 + 2 * R), min(H6, y0 + 2 * R)
    spr[sy0 - y0:sy1 - y0, sx0 - x0:sx1 - x0, :3] = im6[sy0:sy1, sx0:sx1, :3]
    spr[sy0 - y0:sy1 - y0, sx0 - x0:sx1 - x0, 3] = cv2.GaussianBlur(m, (3, 3), 0)[sy0:sy1, sx0:sx1]
    info.append({"name": name, "cx": cx, "cy": cy, "r": ro, "teeth": teeth, "R": R, "ox": cx - x0, "oy": cy - y0, "img": Image.fromarray(spr)})
# fond du train : le métal des roues est remplacé par la toile du fond (texture prise sur le fond visible)
gm = (gear_px.astype(np.uint8) * 255)
gm = cv2.dilate(gm, np.ones((3, 3), np.uint8))
base = im6.copy()
teal = (~metal) & (im6[..., 3] > 200) & (rgb6[..., 1] > rgb6[..., 0] + 10)
tealpx = im6[teal][:, :3].astype(float); mu, sd = tealpx.mean(0), tealpx.std(0)
noise = np.random.default_rng(7).normal(0, 1, (H6, W6, 1)) * sd[None, None, :] * .55
fill = np.clip((mu + noise) * .62, 0, 255)          # toile du fond, dans l'ombre des roues          # plus sombre : le fond derrière les roues est dans leur ombre
base[..., :3] = np.where(gm[..., None] > 0, fill, base[..., :3]).astype(np.uint8)
# taille de sortie : 2 × la taille affichée (train haut de 118 px sur la scène)
ys, xs = np.nonzero(im6[..., 3] > 40); bx0, by0, bx1, by1 = xs.min(), ys.min(), xs.max() + 1, ys.max() + 1
disp_h = 118; sc = 2 * disp_h / (by1 - by0)
Image.fromarray(base).crop((bx0, by0, bx1, by1)).resize((round((bx1 - bx0) * sc), round((by1 - by0) * sc)), Image.LANCZOS).save(OUT / "train_fond.webp", "WEBP", quality=88, method=6)
# arbre d'engrènement (roue menante g1) : g1->g2, g1->g4, g2->g5, g2->g3 ; vitesse = -dents_menante / dents_menée
tree = {"g2": "g1", "g4": "g1", "g5": "g2", "g3": "g2"}
T = {i["name"]: i["teeth"] for i in info}; speed = {"g1": 1.0}
for n in ["g2", "g4", "g5", "g3"]: speed[n] = -speed[tree[n]] * T[tree[n]] / T[n]
gl = []
for i in info:
    img = i["img"].resize((round(i["img"].width * sc), round(i["img"].height * sc)), Image.LANCZOS); save(img, "train_" + i["name"])
    gl.append({"name": i["name"], "cx": round((i["cx"] - bx0) * sc / 2, 2), "cy": round((i["cy"] - by0) * sc / 2, 2), "w": img.width / 2, "h": img.height / 2,
               "ox": round(i["ox"] * sc / 2, 2), "oy": round(i["oy"] * sc / 2, 2), "teeth": i["teeth"], "speed": round(speed[i["name"]], 4), "parent": tree.get(i["name"])})
meta["train"] = {"w": round((bx1 - bx0) * sc / 2, 2), "h": disp_h, "gears": gl}
print("train :", [(g["name"], g["teeth"], g["speed"]) for g in gl])

# =============================================================================
# 3. Cylindre musical (08) : cylindre sans picots + picot seul + peigne + clé
# =============================================================================
im8 = np.array(rgba("08_Cylindre_Musical.png"))
CYL = (466, 282, 1231, 545)            # bande du cylindre (picots visibles)
x0, y0, x1, y1 = CYL; band = im8[y0:y1, x0:x1, :3].copy()
lum = cv2.cvtColor(band, cv2.COLOR_RGB2GRAY).astype(float)
th = lum - cv2.GaussianBlur(lum, (0, 0), 9)
pins = (th > 22).astype(np.uint8) * 255
pins = cv2.morphologyEx(pins, cv2.MORPH_OPEN, np.ones((3, 3), np.uint8))
lab, n = nd.label(pins > 0); keep = np.zeros_like(pins)
centers = []
for i, s in enumerate(nd.find_objects(lab)):
    hgt, wid = s[0].stop - s[0].start, s[1].stop - s[1].start
    if 5 <= hgt <= 30 and 5 <= wid <= 30: keep[lab == i + 1] = 255; centers.append(((s[1].start + s[1].stop) / 2 + x0, (s[0].start + s[0].stop) / 2 + y0))
hole = cv2.dilate(keep, np.ones((11, 11), np.uint8))
clean = im8.copy(); clean[y0:y1, x0:x1, :3] = cv2.inpaint(band, hole, 6, cv2.INPAINT_TELEA)
# la clé (à gauche) tourne sur l'axe horizontal : on la retire du support
KEY8 = (71, 330, 298, 662); kx0, ky0, kx1, ky1 = KEY8
key8 = Image.fromarray(im8[ky0:ky1, kx0:kx1]); clean[ky0:ky1, kx0:kx1 - 12, 3] = 0
ys, xs = np.nonzero(im8[..., 3] > 40); bx0, by0, bx1, by1 = xs.min(), ys.min(), xs.max() + 1, ys.max() + 1
disp_h = 118; sc8 = 2 * disp_h / (by1 - by0)
rs = lambda im: im.resize((max(1, round(im.width * sc8)), max(1, round(im.height * sc8))), Image.LANCZOS)
save(rs(Image.fromarray(clean).crop((bx0, by0, bx1, by1))), "cylindre_fond")
save(rs(key8), "cylindre_cle")
# un picot propre (le plus isolé) pour redessiner les picots en rotation
pc = max(centers, key=lambda c: min([math.hypot(c[0] - d[0], c[1] - d[1]) for d in centers if d != c] or [0]))
pin = Image.fromarray(im8[int(pc[1]) - 15:int(pc[1]) + 15, int(pc[0]) - 15:int(pc[0]) + 15]); save(rs(pin), "picot")
# lames du peigne (positions x) mesurées sur une ligne horizontale
row = im8[650, 521:1176, :3].astype(float).mean(1); bright = row > row.mean()
edges = np.flatnonzero(np.diff(bright.astype(int)) == 1); tines = [round((e + 521 + 9 - bx0) * sc8 / 2, 2) for e in edges]
meta["cylindre"] = {"w": round((bx1 - bx0) * sc8 / 2, 2), "h": disp_h, "band": [round((v - (bx0 if i % 2 == 0 else by0)) * sc8 / 2, 2) for i, v in enumerate(CYL)],
                    "key": [round((kx0 - bx0) * sc8 / 2, 2), round((ky0 - by0) * sc8 / 2, 2), round((kx1 - kx0) * sc8 / 2, 2), round((ky1 - ky0) * sc8 / 2, 2)], "keyAxisY": round((495 - by0) * sc8 / 2, 2),
                    "comb": [round((521 - bx0) * sc8 / 2, 2), round((545 - by0) * sc8 / 2, 2), round((1176 - bx0) * sc8 / 2, 2), round((770 - by0) * sc8 / 2, 2)], "tines": tines,
                    "pins": len(centers)}
print("cylindre :", len(centers), "picots,", len(tines), "lames")

# =============================================================================
# 4. Ressort moteur (07) : spirale qui tourne + clé qui tourne + cliquet fixe
# =============================================================================
im7 = np.array(rgba("07_Ressort_Moteur.png"))
cx7, cy7, r7 = 616, 808, 324
H7, W7 = im7.shape[:2]; yy, xx = np.mgrid[0:H7, 0:W7]
disc = np.hypot(xx - cx7, yy - cy7) < r7
pawl = np.zeros((H7, W7), np.uint8); cv2.fillPoly(pawl, [np.array([(735, 505), (905, 505), (940, 590), (945, 700), (900, 745), (790, 745), (735, 650)], np.int32)], 255)
pawl = (pawl > 0) & disc & (im7[..., 0].astype(int) > im7[..., 2].astype(int) + 40)   # partie laiton du cliquet
pawlm = cv2.dilate(pawl.astype(np.uint8) * 255, np.ones((9, 9), np.uint8))
spiral = im7.copy(); spiral[..., :3] = cv2.inpaint(im7[..., :3], pawlm, 9, cv2.INPAINT_TELEA)
R = r7 + 2; sp = spiral[cy7 - R:cy7 + R, cx7 - R:cx7 + R].copy()
yy2, xx2 = np.mgrid[0:2 * R, 0:2 * R]; sp[..., 3] = (np.clip(r7 - np.hypot(xx2 - R, yy2 - R), 0, 1.5) / 1.5 * 255).astype(np.uint8)
pawl_spr = np.zeros_like(im7); pawl_spr[pawl] = im7[pawl]; pawl_spr[..., 3] = cv2.GaussianBlur((pawl * 255).astype(np.uint8), (3, 3), 0)
KEY7 = (164, 13, 1083, 300)
key7 = im7[KEY7[1]:KEY7[3], KEY7[0]:KEY7[2]].copy()
base7 = im7.copy(); base7[KEY7[1]:KEY7[3] - 10, KEY7[0]:KEY7[2], 3] = 0
ys, xs = np.nonzero(im7[..., 3] > 40); bx0, by0, bx1, by1 = xs.min(), ys.min(), xs.max() + 1, ys.max() + 1
disp_h = 96; sc7 = 2 * disp_h / (by1 - by0)
rs = lambda im: im.resize((max(1, round(im.width * sc7)), max(1, round(im.height * sc7))), Image.LANCZOS)
save(rs(Image.fromarray(base7).crop((bx0, by0, bx1, by1))), "ressort_fond"); save(rs(Image.fromarray(sp)), "ressort_spirale")
save(rs(Image.fromarray(pawl_spr).crop((bx0, by0, bx1, by1))), "ressort_cliquet"); save(rs(Image.fromarray(key7)), "ressort_cle")
meta["ressort"] = {"w": round((bx1 - bx0) * sc7 / 2, 2), "h": disp_h, "cx": round((cx7 - bx0) * sc7 / 2, 2), "cy": round((cy7 - by0) * sc7 / 2, 2), "r": round(R * sc7 / 2, 2),
                   "key": [round((KEY7[0] - bx0) * sc7 / 2, 2), round((KEY7[1] - by0) * sc7 / 2, 2), round((KEY7[2] - KEY7[0]) * sc7 / 2, 2), round((KEY7[3] - KEY7[1]) * sc7 / 2, 2)], "keyAxisX": round((624 - bx0) * sc7 / 2, 2)}

# =============================================================================
# 5. Marteau et cloche (09) : levier séparé, pivot fixe
# =============================================================================
im9 = np.array(rgba("09_Marteau_Cloche.png")); off = np.array([105, 54])
poly = np.array([(448, 278), (540, 276), (600, 322), (700, 335), (1105, 290), (1150, 280), (1210, 292), (1220, 342), (1192, 372), (1140, 374), (870, 396), (735, 412), (600, 400), (540, 402), (448, 398)]) + off
lever = np.zeros(im9.shape[:2], np.uint8); cv2.fillPoly(lever, [poly.astype(np.int32)], 255)
lever = (lever > 0) & (im9[..., 3] > 30)
piv = np.array([790, 405]) + off; H9, W9 = lever.shape; yy, xx = np.mgrid[0:H9, 0:W9]
hub = np.hypot(xx - piv[0], yy - piv[1]) < 64
ped = np.zeros((H9, W9), np.uint8); cv2.fillPoly(ped, [(np.array([(612, 330), (945, 330), (945, 700), (612, 700)]) + off).astype(np.int32)], 255)
lev_only = lever & ~hub
lev_spr = np.zeros_like(im9); lev_spr[lev_only] = im9[lev_only]; lev_spr[..., 3] = np.where(lev_only, im9[..., 3], 0)
base9 = im9.copy()
under = lev_only & (ped > 0)
lm = cv2.dilate(under.astype(np.uint8) * 255, np.ones((5, 5), np.uint8))
base9[..., :3] = cv2.inpaint(im9[..., :3], lm, 9, cv2.INPAINT_TELEA)
base9[..., 3] = np.where(lev_only & ~(ped > 0), 0, base9[..., 3])
# zone du support sous le levier : reconstruite opaque seulement là où le support existe (sous l'arche)
hub_spr = np.zeros_like(im9); hub_spr[hub] = im9[hub]; hub_spr[..., 3] = np.where(hub, im9[..., 3], 0)
ys, xs = np.nonzero(im9[..., 3] > 40); bx0, by0, bx1, by1 = xs.min(), ys.min(), xs.max() + 1, ys.max() + 1
disp_w = 124; sc9 = 2 * disp_w / (bx1 - bx0)
rs = lambda im: im.resize((max(1, round(im.width * sc9)), max(1, round(im.height * sc9))), Image.LANCZOS)
for n, a in [("marteau_fond", base9), ("marteau_levier", lev_spr), ("marteau_pivot", hub_spr)]: save(rs(Image.fromarray(a).crop((bx0, by0, bx1, by1))), n)
meta["marteau"] = {"w": disp_w, "h": round((by1 - by0) * sc9 / 2, 2), "px": round((piv[0] - bx0) * sc9 / 2, 2), "py": round((piv[1] - by0) * sc9 / 2, 2),
                   "bell": [round((340 + 105 - bx0) * sc9 / 2, 2), round((400 + 54 - by0) * sc9 / 2, 2)]}

# =============================================================================
# 6. Commandes, chiens, médaillons
# =============================================================================
a2, a3, a4 = rgba("02_Controls_Sprite_Atlas.png"), rgba("03_Dog_Mascots_and_Ornaments.png"), rgba("04_Auxiliary_Panels_and_Icons.png")
def crop(im, box, w=None, h=None, name=None):
    c = im.crop(box)
    if w: c = c.resize((w, round(c.height * w / c.width)), Image.LANCZOS)
    if h: c = c.resize((round(c.width * h / c.height), h), Image.LANCZOS)
    if name: save(c, name)
    return c
knob = a2.crop((75, 7, 310, 244)); kc = (knob.width / 2, knob.height / 2 - 2)
knob = knob.resize((180, round(180 * knob.height / knob.width)), Image.LANCZOS); save(knob, "molette")
s = 180 / 235; capR = 74 * s; cxk, cyk = kc[0] * s, kc[1] * s
cap = np.array(knob); yy, xx = np.mgrid[0:cap.shape[0], 0:cap.shape[1]]
cap[..., 3] = (np.clip(capR - np.hypot(xx - cxk, yy - cyk), 0, 1.5) / 1.5 * cap[..., 3]).astype(np.uint8)
save(Image.fromarray(cap).crop((int(cxk - capR - 2), int(cyk - capR - 2), int(cxk + capR + 2), int(cyk + capR + 2))), "molette_chapeau")
meta["molette"] = {"w": 90, "h": knob.height / 2, "cx": cxk / 2, "cy": cyk / 2, "capR": capR / 2 + 1}
for n, b in [("bouton_rouge_on", (180, 260, 305, 383)), ("bouton_creme_on", (479, 259, 603, 383)), ("bouton_vert_on", (780, 259, 906, 382))]: crop(a2, b, w=96, name=n)
crop(a2, (764, 462, 830, 528), w=56, name="curseur_bille")
for n, b in [("chien_tambour", (25, 7, 278, 410)), ("chien_trompette", (328, 82, 579, 397)), ("chien_coureur", (600, 81, 923, 369)), ("chien_chef", (951, 12, 1232, 397))]: crop(a3, b, h=220, name=n)
for n, b in [("med_reglages", (227, 1049, 413, 1228)), ("med_cloche", (431, 1049, 615, 1228)), ("med_note", (636, 1049, 820, 1228)), ("med_cle", (840, 1049, 1025, 1228))]: crop(a4, b, w=96, name=n)
crop(a3, (1031, 1048, 1206, 1223), w=128, name="medaille_patte")
crop(rgba("05_Roue_Simple.png").crop((0, 0, 1254, 1254)), (0, 0, 1254, 1254), w=256, name="roue")
Image.open(SRC / "01_Wind-Up_Dogs_Main_Interface.png").convert("RGB").crop((246, 96, 502, 352)).resize((1024, 1024), Image.LANCZOS).save(OUT / "icon-1024.png")
(OUT / "layout.json").write_text(json.dumps(meta, indent=1))
print("ok")
