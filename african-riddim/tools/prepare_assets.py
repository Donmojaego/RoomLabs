#!/usr/bin/env python3
"""African Riddim : prépare les images de l'interface depuis le pack de Ghislain (assets_src/).
Sorties dans ui/assets/ :
  bg_<layout>.webp      fond 1536×1024 sans carte ni panneau (zones retouchées par inpainting)
  map_<layout>.webp     carte nettoyée (sélection « RDC » d'origine effacée), 1000 px
  lab_<layout>.png      étiquettes des régions de la carte (R+G*256 = n° de région, 0 = mer)
  inst_XX.webp          54 illustrations d'instruments (tiers des planches, 512×683 → 480 px)
  ctl_<layout>_*.webp   commandes découpées (pilules, liste, molettes, +/-, flèche)
  map.json              ancres des pays par carte + région sous chaque ancre
"""
import json, os, sys
import numpy as np, cv2
from PIL import Image
from scipy import ndimage
from scipy.optimize import linear_sum_assignment
sys.path.insert(0, os.path.dirname(__file__)); from countries import C
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.join(HERE, "..")
S = os.path.join(ROOT, "assets_src", "01_Assets_Interface"); P = os.path.join(ROOT, "assets_src", "02_Instruments_3_par_planche")
OUT = os.path.join(ROOT, "ui", "assets"); os.makedirs(OUT, exist_ok=True)
def save(im, name, q=88): im.save(os.path.join(OUT, name), "WEBP", quality=q, method=6)

LAYOUTS = { "bronze": ("01_AFRICAN_RIDDIM_Layout_Bronze.png", "03_Carte_Afrique_Bronze.png"),
            "uno": ("02_AFRICAN_RIDDIM_Layout_Bleu_Multicolore.png", "04_Carte_Afrique_UNO.png") }
MAP_S, MAP_X, MAP_Y = 0.70, -60, 92            # placement de la carte dans le fond 1536×1024
def paper(h, w, seed, amp):
    rng = np.random.default_rng(seed); n = rng.normal(0, 1, (h // 2 + 1, w // 2 + 1)).astype(np.float32)
    n = cv2.resize(cv2.GaussianBlur(n, (0, 0), 1.2), (w, h)); f = cv2.resize(rng.normal(0, 1, (h // 24 + 1, w // 24 + 1)).astype(np.float32), (w, h), interpolation=cv2.INTER_CUBIC)
    return (n * 0.6 + f * 0.4) * amp

def make_bg(key, RGB):
    """Fond propre : bandeau du titre repris de la maquette, le reste recomposé (pas de trace de l'ancienne carte)."""
    H, W = 1024, 1536; yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)
    if key == "bronze":
        base = np.array([15, 21, 32], np.float32); glow = np.array([28, 36, 52], np.float32)
        r = np.hypot((xx - W * 0.42) / (W * 0.7), (yy - H * 0.45) / (H * 0.8))
        img = base + (glow - base) * np.clip(1 - r, 0, 1)[..., None] ** 1.5
        img += paper(H, W, 3, 2.2)[..., None]
        # fines lignes de cuivre en bas, motif en chevrons discret
        for k in range(0, W, 28):
            pts = np.array([[k, H - 26], [k + 14, H - 40], [k + 28, H - 26]], np.int32)
            cv2.polylines(img, [pts], False, (120, 80, 46), 2, cv2.LINE_AA)
        cv2.line(img, (0, H - 48), (W, H - 48), (90, 62, 38), 1, cv2.LINE_AA); cv2.line(img, (0, H - 18), (W, H - 18), (90, 62, 38), 1, cv2.LINE_AA)
    else:
        pal = np.array([[233, 128, 110], [102, 190, 178], [244, 196, 96], [182, 150, 200], [240, 224, 190], [226, 110, 120], [120, 200, 170], [238, 160, 90]], np.float32)
        img = np.zeros((H, W, 3), np.float32); img[:] = [236, 214, 180]
        bands = 9
        for i in range(bands):
            ph = i * 1.7; amp = 70 + 25 * np.sin(i)
            edge = (i + 0.2) * H / (bands - 1.6) + amp * np.sin(xx / W * 2 * np.pi * 0.9 + ph) + 35 * np.sin(xx / W * 2 * np.pi * 2.3 + ph * 1.3) - 0.28 * xx
            m = (yy > edge).astype(np.float32); m = cv2.GaussianBlur(m, (0, 0), 1.2)
            col = pal[i % len(pal)]
            shade = 1 - 0.10 * np.clip((yy - edge) / 60, 0, 1)
            img = img * (1 - m[..., None]) + (col * shade[..., None]) * m[..., None]
            # liseré clair + pointillés le long de la vague
            line = np.exp(-((yy - edge) / 2.2) ** 2)
            img += line[..., None] * 38
        img += paper(H, W, 5, 9)[..., None]
        # motifs en chevrons sur la bande du bas
        for k in range(0, W, 34):
            for d in (0, 10):
                pts = np.array([[k, H - 22 - d], [k + 17, H - 38 - d], [k + 34, H - 22 - d]], np.int32)
                cv2.polylines(img, [pts], False, (250, 236, 205), 2, cv2.LINE_AA)
    img = np.clip(img, 0, 255)
    # bandeau du titre de la maquette, fondu vers le bas
    top = 128; fade = np.clip((top - yy[:top + 40]) / 22 + 0.0, 0, 1)
    band = RGB[:top + 40].astype(np.float32)
    img[:top + 40] = band * fade[..., None] + img[:top + 40] * (1 - fade[..., None])
    # flèche carte → panneau de la maquette
    ax0, ay0, ax1, ay1 = 660, 520, 770, 590
    img[ay0:ay1, ax0:ax1] = RGB[ay0:ay1, ax0:ax1] * 0 + img[ay0:ay1, ax0:ax1]
    return img.astype(np.uint8)

meta = { "mapPlace": [MAP_X, MAP_Y, round(1254 * MAP_S)], "maps": {} }

for key, (mock, mp) in LAYOUTS.items():
    RGB = np.array(Image.open(os.path.join(S, mock)).convert("RGB")); A = np.array(Image.open(os.path.join(S, mp)).convert("RGBA"))
    # ---- fond : effacer la carte, le zoom, « ENTRÉE » et le panneau de la maquette
    w = int(1254 * MAP_S); al = cv2.resize(A[..., 3], (w, w)); mask = np.zeros(RGB.shape[:2], np.uint8)
    y0, x0 = max(0, MAP_Y), max(0, MAP_X); sub = al[y0 - MAP_Y:, x0 - MAP_X:]; h = min(sub.shape[0], 1024 - y0); ww = min(sub.shape[1], 1536 - x0)
    mask[y0:y0 + h, x0:x0 + ww] = (sub[:h, :ww] > 20) * 255
    mask = cv2.dilate(mask, np.ones((61, 61), np.uint8)); mask[650:1000, 30:340] = 255
    mask[140:965, 770:1525] = 255
    bg = make_bg(key, RGB)
    save(Image.fromarray(bg), f"bg_{key}.webp", 90)

    # ---- carte : effacer la sélection d'origine (RDC) puis régions
    rgb = A[..., :3].astype(int); alpha = A[..., 3]; lum = rgb.mean(-1)
    if key == "uno":
        # liseré jaune de la RDC « sélectionnée » : jaune vif voisin de la région saumon qui contient la RDC
        salmon = (rgb[..., 0] > 185) & (rgb[..., 1] > 70) & (rgb[..., 1] < 150) & (rgb[..., 2] < 150) & (alpha > 100)
        sl, _ = ndimage.label(salmon); seed = sl[708, 683] if sl[708, 683] else np.bincount(sl[680:740, 650:715].ravel())[1:].argmax() + 1
        drc = cv2.dilate((sl == seed).astype(np.uint8), np.ones((31, 31), np.uint8)) > 0
        yellow = (rgb[..., 0] > 200) & (rgb[..., 1] > 165) & (rgb[..., 2] < 150) & (alpha > 60) & drc
        yellow = cv2.dilate(yellow.astype(np.uint8), np.ones((3, 3), np.uint8)) > 0
        border = (lum < 90) | yellow
        filled = ndimage.binary_fill_holes((sl == seed) | yellow)
        inside = ndimage.binary_erosion(filled, iterations=9)
        A2 = A.copy(); A2[yellow & ~inside, :3] = [24, 33, 58]
        sal = rgb[(sl == seed)].mean(0); A2[yellow & inside, :3] = np.clip(sal * 1.18 + 10, 0, 255).astype(np.uint8)
        border = (lum < 90) | (yellow & ~inside)
    else:
        border = (rgb[..., 0] > 170) & (rgb[..., 1] > 120)
        A2 = A.copy()
    inner = ndimage.binary_erosion((alpha > 128) & ~border, iterations=1)
    lab, n = ndimage.label(inner)
    sizes = ndimage.sum(np.ones_like(lab), lab, range(1, n + 1))
    keep = [i + 1 for i, s in enumerate(sizes) if s > 120]
    remap = np.zeros(n + 1, np.int32)
    for k, i in enumerate(keep): remap[i] = k + 1
    lab = remap[lab]
    # étendre les régions sur les frontières (clic sur un trait = pays voisin)
    dist, (iy, ix) = ndimage.distance_transform_edt(lab == 0, return_indices=True)
    lab = np.where((alpha > 60) & (dist < 8), lab[iy, ix], lab)
    cents = ndimage.center_of_mass(lab > 0, lab, range(1, len(keep) + 1))
    Pc = np.array(cents)[:, ::-1]
    lon = np.array([c[7] for c in C]); lat = np.array([c[8] for c in C])
    ys, xs = np.nonzero(alpha > 128)
    px = xs.min() + (lon + 17.5) / 68.9 * (xs.max() - xs.min()); py = ys.min() + (37.3 - lat) / 72.1 * (ys.max() - ys.min())
    for it in range(5):
        D = np.hypot(Pc[:, None, 0] - px[None], Pc[:, None, 1] - py[None]); r, c = linear_sum_assignment(D); good = D[r, c] < 110
        Ax = np.c_[lon[c[good]], lat[c[good]], np.ones(good.sum())]
        cx = np.linalg.lstsq(Ax, Pc[r[good], 0], rcond=None)[0]; cy = np.linalg.lstsq(Ax, Pc[r[good], 1], rcond=None)[0]
        L = np.c_[lon, lat, np.ones(len(lon))]; px = L @ cx; py = L @ cy
    if key == "bronze":
        # la RDC est peinte en cuivre clair (sélection) : on la ramène au ton des voisins
        cdi = [i for i, c in enumerate(C) if c[0] == "cd"][0]
        reg = lab[int(round(py[cdi])), int(round(px[cdi]))]
        if reg:
            hsv = cv2.cvtColor(A[..., :3], cv2.COLOR_RGB2HSV).astype(int)
            orange = (rgb[..., 0] > 120) & (rgb[..., 0] > 1.7 * rgb[..., 1]) & (hsv[..., 1] > 170) & (alpha > 100)
            ol, _ = ndimage.label(cv2.morphologyEx(orange.astype(np.uint8), cv2.MORPH_CLOSE, np.ones((7, 7), np.uint8)))
            sy, sx = int(round(py[cdi])), int(round(px[cdi])); win = ol[sy - 40:sy + 40, sx - 40:sx + 40].ravel(); win = win[win > 0]
            seed = np.bincount(win).argmax()
            full = cv2.dilate(ndimage.binary_fill_holes(ol == seed).astype(np.uint8), np.ones((7, 7), np.uint8)).astype(bool) & (alpha > 60)
            inner_m = ndimage.binary_erosion(full, iterations=4)
            edge = full & ~inner_m; m = inner_m
            gold = rgb[border & (alpha > 200) & ~cv2.dilate(full.astype(np.uint8), np.ones((40, 40), np.uint8)).astype(bool)].mean(0)
            ring = cv2.dilate(full.astype(np.uint8), np.ones((81, 81), np.uint8)).astype(bool) & ~full & (alpha > 200) & ~cv2.dilate(border.astype(np.uint8), np.ones((5, 5), np.uint8)).astype(bool)
            ref = rgb[ring].astype(np.float32); mu = ref.mean(0); sd = ref.std(0)
            hh, wwid = m.shape
            nz = paper(hh, wwid, 11, 1.0); nz = (nz - nz[m].mean()) / (nz[m].std() + 1e-6)
            nz2 = cv2.GaussianBlur(np.random.default_rng(4).normal(0, 1, (hh, wwid)).astype(np.float32), (0, 0), 14); nz2 /= nz2[m].std() + 1e-6
            fine = np.random.default_rng(9).normal(0, 1, (hh, wwid)).astype(np.float32)
            tex = mu[None, None] + (0.6 * nz + 0.6 * nz2 + 0.55 * fine)[..., None] * sd[None, None] * 0.75
            A2[..., :3] = np.where(m[..., None], np.clip(tex, 0, 255).astype(np.uint8), A2[..., :3])
            g = np.broadcast_to(gold, rgb.shape) * (0.92 + 0.16 * np.random.default_rng(2).random(lum.shape))[..., None]
            A2[..., :3] = np.where(edge[..., None], np.clip(g, 0, 255).astype(np.uint8), A2[..., :3])
    # sorties carte 1000 px
    Z = 1000 / 1254
    save(Image.fromarray(A2).resize((1000, 1000), Image.LANCZOS), f"map_{key}.webp", 90)
    labs = cv2.resize(lab.astype(np.uint16), (1000, 1000), interpolation=cv2.INTER_NEAREST)
    enc = np.zeros((1000, 1000, 3), np.uint8); enc[..., 0] = labs % 256; enc[..., 1] = labs // 256
    Image.fromarray(enc).save(os.path.join(OUT, f"lab_{key}.png"), optimize=True)
    dist0, (iy0, ix0) = ndimage.distance_transform_edt(lab == 0, return_indices=True)
    anchors = {}
    for i, c in enumerate(C):
        x, y = float(px[i]), float(py[i]); yi, xi = min(1253, max(0, int(round(y)))), min(1253, max(0, int(round(x))))
        reg = int(lab[yi, xi])
        if reg == 0 and dist0[yi, xi] < 25: reg = int(lab[iy0[yi, xi], ix0[yi, xi]])   # ancre sur un trait : région la plus proche
        anchors[c[0]] = [round(x * Z, 1), round(y * Z, 1), reg]
    meta["maps"][key] = { "anchors": anchors, "regions": len(keep) }
    print(key, "régions", len(keep), "pays sans région", [k for k, v in anchors.items() if v[2] == 0])

# ---- illustrations : trois par planche, de gauche à droite ; recadrées sur l'instrument, bords fondus (alpha)
planches = sorted(os.listdir(P))
for i, c in enumerate(C):
    f = [x for x in planches if x.startswith(f"{c[4]:02d}_")][0]
    im = Image.open(os.path.join(P, f)).convert("RGB"); W = im.width // 3
    a = np.array(im.crop((c[5] * W, 0, (c[5] + 1) * W, im.height))).astype(np.float32)
    lum = a.mean(-1); bgl = cv2.GaussianBlur(lum, (0, 0), 60)
    hsv = cv2.cvtColor(a.astype(np.uint8), cv2.COLOR_RGB2HSV)
    fg = ((lum - bgl) > 18) | ((lum > 95) & (hsv[..., 1] > 40))
    fg = cv2.morphologyEx(fg.astype(np.uint8), cv2.MORPH_OPEN, np.ones((5, 5), np.uint8))
    ys, xs = np.nonzero(fg); m = 40
    if len(ys) > 500:
        y0, y1 = np.percentile(ys, [0.5, 99.5]).astype(int); x0, x1 = np.percentile(xs, [0.5, 99.5]).astype(int)
    else:
        y0, y1, x0, x1 = 0, a.shape[0], 0, a.shape[1]
    y0, x0 = max(0, y0 - m), max(0, x0 - m); y1, x1 = min(a.shape[0], y1 + m), min(a.shape[1], x1 + m)
    crop = a[y0:y1, x0:x1]; h, w = crop.shape[:2]
    yy, xx = np.mgrid[0:h, 0:w]; d = np.minimum(np.minimum(xx, w - 1 - xx), np.minimum(yy, h - 1 - yy)).astype(np.float32)
    alpha = np.clip(d / 34, 0, 1) ** 1.6 * 255
    rgba = np.dstack([crop, alpha]).astype(np.uint8)
    out = Image.fromarray(rgba, "RGBA"); out.thumbnail((440, 440), Image.LANCZOS)
    save(out, f"inst_{i:02d}.webp", 84)

# ---- commandes : composantes de la planche (alpha), triées par ligne puis colonne
for key, f in (("uno", "05_Commandes_Bronze.png"),):   # planche aux couleurs vives (rouge/jaune/vert/cyan) = thème multicolore
    A = np.array(Image.open(os.path.join(S, f)).convert("RGBA")); al = A[..., 3] > 40
    lab, n = ndimage.label(cv2.dilate(al.astype(np.uint8), np.ones((9, 9), np.uint8)))
    objs = ndimage.find_objects(lab); boxes = [(o[0].start, o[1].start, o[0].stop, o[1].stop) for o in objs if (o[0].stop - o[0].start) * (o[1].stop - o[1].start) > 2000]
    boxes.sort(key=lambda b: (round(b[0] / 60), b[1]))
    meta.setdefault("controls", {})[key] = []
    for k, (y0, x0, y1, x1) in enumerate(boxes):
        im = Image.fromarray(A[y0:y1, x0:x1]); save(im, f"ctl_{key}_{k:02d}.webp", 90)
        meta["controls"][key].append([k, x1 - x0, y1 - y0])
    print(key, "commandes", len(boxes), [(b[3] - b[1], b[2] - b[0]) for b in boxes])
Image.open(os.path.join(S, "08_Titre_AFRICAN_RIDDIM.png")).save(os.path.join(OUT, "titre.webp"), "WEBP", quality=90)
json.dump(meta, open(os.path.join(ROOT, "ui", "assets", "map.json"), "w"), ensure_ascii=False)

# ---- molettes : plaque sans arc ni repère (dessinés en code), pour chaque jeu
def knob_plate(src, arc_test):
    a = np.array(Image.open(os.path.join(OUT, src)).convert("RGBA")).astype(np.float32)
    h, w = a.shape[:2]; cy, cx = h / 2, w / 2; R = min(h, w) / 2
    yy, xx = np.mgrid[0:h, 0:w]; r = np.hypot(yy - cy, xx - cx) / R
    rgb = a[..., :3]; hsv = cv2.cvtColor(rgb.astype(np.uint8), cv2.COLOR_RGB2HSV).astype(int)
    ann = (r > 0.79) & (r < 0.99)
    arc = ann & arc_test(rgb, hsv)
    arc = cv2.dilate(arc.astype(np.uint8), np.ones((5, 5), np.uint8)).astype(bool) & ann
    track = np.median(rgb[ann & ~arc & (a[..., 3] > 200)], axis=0)
    out = a.copy(); out[arc, :3] = track
    # repère blanc/clair sur le chapeau : effacé par inpainting
    cap = r < 0.62
    ind = cap & (rgb.mean(-1) > 185) & (hsv[..., 1] < 90)
    ind = cv2.dilate(ind.astype(np.uint8), np.ones((7, 7), np.uint8))
    out_rgb = cv2.inpaint(out[..., :3].astype(np.uint8), ind, 6, cv2.INPAINT_TELEA)
    out[..., :3] = out_rgb
    return out.astype(np.uint8), [round(float(c), 1) for c in track]

meta["knobs"] = {}
uno_arc = lambda rgb, hsv: (hsv[..., 1] > 150) & (hsv[..., 2] > 150) & ~((hsv[..., 0] > 100) & (hsv[..., 0] < 130) & (hsv[..., 2] < 200))
for k, f in enumerate(("ctl_uno_05.webp", "ctl_uno_06.webp", "ctl_uno_07.webp")):
    img, track = knob_plate(f, uno_arc)
    col = None
    save(Image.fromarray(img), f"knob_uno_{k}.webp", 92)
# bronze : plaque « uno » passée en métal sombre, anneau doré conservé
b = np.array(Image.open(os.path.join(OUT, "knob_uno_0.webp")).convert("RGBA")).astype(np.float32)
hsv = cv2.cvtColor(b[..., :3].astype(np.uint8), cv2.COLOR_RGB2HSV).astype(np.float32)
blue = (hsv[..., 0] > 95) & (hsv[..., 0] < 135)
grey = b[..., :3].mean(-1, keepdims=True) * np.array([0.62, 0.58, 0.56])
b[..., :3] = np.where(blue[..., None], grey + 6, b[..., :3] * np.array([1.02, 0.86, 0.70]))
save(Image.fromarray(np.clip(b, 0, 255).astype(np.uint8)), "knob_bronze_0.webp", 92)
json.dump(meta, open(os.path.join(ROOT, "ui", "assets", "map.json"), "w"), ensure_ascii=False)
