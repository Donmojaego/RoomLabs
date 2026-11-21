#!/usr/bin/env python3
"""Prépare les images de « Ma petite soupe ouselem » à partir des deux maquettes (docs/reference).

Les maquettes sont des images entières : les commandes y sont dessinées. Ce script
  1. retrouve le contour de chaque sachet (lignes ajustées sur les bords),
  2. remplace le sachet 04 (dessiné « sélectionné ») par une copie du sachet 01 déformée à sa place,
  3. efface (inpainting) ce que l'interface redessine : numéros des sachets, lueur et repère de sélection,
     potards, textes des menus, écran du preset,
  4. écrit le fond nettoyé, un calque par sachet et le calque de la surface du bol (animée).
Usage : python3 tools/prepare_assets.py <dossier des maquettes> <dossier de sortie ui/assets>
"""
import json
import pathlib
import sys

import cv2
import numpy as np
from PIL import Image

ROOT = pathlib.Path(__file__).resolve().parent.parent
LAY = json.loads((ROOT / "tools" / "layout.json").read_text(encoding="utf-8"))
SRC = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "docs" / "reference"
OUT = pathlib.Path(sys.argv[2]) if len(sys.argv) > 2 else ROOT / "ui" / "assets"
OUT.mkdir(parents=True, exist_ok=True)
FILES = {"soupe": "Ma_Petite_Soupe_Ouselem.png", "ramen": "Mon_Petit_Ramen.png"}
EDGES = {"soupe": (544, 700, range(558, 660, 4)), "ramen": (514, 688, range(530, 646, 4))}


def find_quads(gray, theme):
    """Contour de chaque sachet : droites ajustées sur les bords gauche/droit, rangée par rangée."""
    ytop, ybot, rows = EDGES[theme]
    boxes = LAY[theme]["sachets"]
    pts = [[] for _ in boxes]
    for y in rows:
        on = gray[y] > 160
        runs, s = [], None
        for x, v in enumerate(on):
            if v and s is None: s = x
            if not v and s is not None:
                if x - s > 8: runs.append((s, x))
                s = None
        merged = []
        for r in runs:
            if merged and r[0] - merged[-1][1] < 26: merged[-1] = (merged[-1][0], r[1])
            else: merged.append(r)
        for (a, b) in merged:
            if b - a < 100: continue
            for i, (x0, _, x1, _) in enumerate(boxes):
                if x0 - 12 <= a and b <= x1 + 12:
                    pts[i].append((y, a, b))
    quads = []
    for i, p in enumerate(pts):
        p = np.array(p, float)
        fl = np.polyfit(p[:, 0], p[:, 1], 1)
        fr = np.polyfit(p[:, 0], p[:, 2], 1)
        quads.append(np.array([[np.polyval(fl, ytop), ytop], [np.polyval(fr, ytop), ytop],
                               [np.polyval(fr, ybot), ybot], [np.polyval(fl, ybot), ybot]], np.float32))
    return quads


def quad_mask(shape, quad, grow=0):
    m = np.zeros(shape[:2], np.uint8)
    q = quad.copy()
    if grow:
        c = q.mean(0)
        q = c + (q - c) * (1 + grow / max(1, (q[:, 0].max() - q[:, 0].min()) / 2))
    cv2.fillPoly(m, [np.round(q).astype(np.int32)], 255)
    return m


def main():
    manifest = {}
    for theme, fname in FILES.items():
        lay = LAY[theme]
        img = cv2.imread(str(SRC / fname), cv2.IMREAD_COLOR)
        gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
        quads = find_quads(gray, theme)
        sel = lay["selected_glow"]

        # 2) sachet 04 : la lueur fausse ses bords détectés -> contour = moyenne de ses voisins 03 et 05
        quads[sel] = ((quads[sel - 1] + quads[sel + 1]) / 2).astype(np.float32)
        # pourtour (plateau, fond) : repris à côté des voisins, moitié gauche depuis le 03, moitié droite depuis le 05
        inner = quad_mask(img.shape, quads[sel])
        ring = cv2.dilate(inner, np.ones((45, 45), np.uint8)) - inner
        x0, y0, x1, y1 = lay["marker"]; ring[y0 - 6:y1, x0 - 6:x1 + 6] = 255; ring[inner > 0] = 0
        cx = quads[sel][:, 0].mean()
        dl = int(round(cx - quads[sel - 1][:, 0].mean())); dr = int(round(quads[sel + 1][:, 0].mean() - cx))
        topY = int(quads[sel][:, 1].min()) + 3
        above = ring.copy(); above[topY:] = 0          # au-dessus du sachet : fond du bol -> effacement doux
        ring[:topY] = 0
        ys, xs = np.nonzero(ring)
        src = img.copy()
        left = xs < cx
        img[ys[left], xs[left]] = src[ys[left], xs[left] - dl]
        img[ys[~left], xs[~left]] = src[ys[~left], np.minimum(xs[~left] + dr, img.shape[1] - 1)]
        # intérieur : copie du sachet 01 déformée à sa place (le 04 de la maquette est « sélectionné »)
        H = cv2.getPerspectiveTransform(quads[0], quads[sel])
        warped = cv2.warpPerspective(src, H, (img.shape[1], img.shape[0]), flags=cv2.INTER_CUBIC)
        img[inner > 0] = warped[inner > 0]

        # 3) masque d'effacement (coutures fines autour du 04 incluses)
        mask = np.zeros(img.shape[:2], np.uint8)
        seam = np.zeros_like(mask); cv2.line(seam, (int(cx), 0), (int(cx), img.shape[0]), 255, 3); seam &= ring
        mask |= seam | cv2.dilate(above, np.ones((3, 3), np.uint8))
        # numéros des sachets : pixels sombres ou saturés dans la bande des chiffres
        b0, b1 = lay["numbers_band"]
        hsv = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)
        for q in quads:
            qm = quad_mask(img.shape, q)
            band = np.zeros_like(qm); band[b0:b1] = 255
            ink = ((gray < 150) | (hsv[..., 1] > 90)).astype(np.uint8) * 255
            mask |= cv2.dilate(ink & qm & band, np.ones((5, 5), np.uint8))
        # potards (disque entier : l'interface les redessine)
        for (cx, cy) in lay["knobs"]:
            cv2.circle(mask, (cx, cy), lay["knob_r"], 255, -1)
        # textes des menus déroulants et de l'écran du preset
        for key in ("pill_instrument", "pill_fx", "preset_pill"):
            if not lay[key]: continue
            x0, y0, x1, y1 = lay[key]
            sub = img[y0:y1, x0:x1].astype(int)
            cream = np.median(sub.reshape(-1, 3), axis=0)
            ink = (np.abs(sub - cream).sum(2) > 60).astype(np.uint8) * 255
            ink = cv2.dilate(ink, np.ones((5, 5), np.uint8))
            mask[y0:y1, x0:x1] |= ink
        clean = cv2.inpaint(img, mask, 6, cv2.INPAINT_TELEA)
        # potards : fond lisse plutôt qu'un inpainting bavant — moyenne de l'anneau extérieur, flouté
        for (cx, cy) in lay["knobs"]:
            r = lay["knob_r"]
            disc = np.zeros(img.shape[:2], np.uint8); cv2.circle(disc, (cx, cy), r, 255, -1)
            blur = cv2.GaussianBlur(clean, (0, 0), 18)
            soft = cv2.GaussianBlur(disc.astype(np.float32) / 255, (0, 0), 4)[..., None]
            clean = (clean * (1 - soft) + blur * soft).astype(np.uint8)

        rgb = cv2.cvtColor(clean, cv2.COLOR_BGR2RGB)
        Image.fromarray(rgb).save(OUT / f"plate_{theme}.webp", "WEBP", quality=86, method=6)

        # 4) un calque par sachet (pour la sélection animée)
        sach = []
        for i, q in enumerate(quads):
            x0, y0 = np.floor(q.min(0)).astype(int); x1, y1 = np.ceil(q.max(0)).astype(int)
            m = quad_mask(img.shape, q)
            m = cv2.GaussianBlur(m, (0, 0), 0.8)
            rgba = np.dstack([rgb, m])[y0:y1, x0:x1]
            Image.fromarray(rgba).save(OUT / f"sachet_{theme}_{i + 1}.webp", "WEBP", quality=88, method=6)
            sach.append({"box": [int(x0), int(y0), int(x1), int(y1)], "quad": np.round(q, 1).tolist()})
        # surface du bol : ellipse adoucie
        ex0, ey0, ex1, ey1 = lay["soup_ellipse"]
        em = np.zeros(img.shape[:2], np.uint8)
        cv2.ellipse(em, ((ex0 + ex1) // 2, (ey0 + ey1) // 2), ((ex1 - ex0) // 2, (ey1 - ey0) // 2), 0, 0, 360, 255, -1)
        em = cv2.GaussianBlur(em, (0, 0), 6)
        Image.fromarray(np.dstack([rgb, em])[ey0:ey1, ex0:ex1]).save(OUT / f"surface_{theme}.webp", "WEBP", quality=86, method=6)
        manifest[theme] = {"sachets": sach, **{k: lay[k] for k in ("knobs", "knob_r", "pill_instrument", "pill_fx", "preset_pill", "toggles", "soup_ellipse", "steam", "numbers_band", "marker")}}
        print(theme, "ok", [s["box"] for s in sach])
    (OUT / "layout.json").write_text(json.dumps(manifest, indent=1), encoding="utf-8")


if __name__ == "__main__":
    main()
