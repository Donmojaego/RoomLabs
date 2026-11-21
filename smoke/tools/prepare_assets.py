#!/usr/bin/env python3
"""Prépare les images de Smoke (pack SMOKE_Assets) : recadrage des objets, tailles d'affichage @2x, WebP.
Les libellés dessinés sous les boutons et molettes ne sont pas repris : l'interface écrit ses propres libellés, plus lisibles.
Usage : python3 tools/prepare_assets.py <dossier SMOKE_Assets> <ui/assets>"""
import pathlib, sys
import numpy as np
from PIL import Image
from scipy import ndimage as nd

SRC = pathlib.Path(sys.argv[1]); OUT = pathlib.Path(sys.argv[2]); OUT.mkdir(parents=True, exist_ok=True)

def obj_box(im, largest=False):
    a = np.array(im)[..., 3] > 12
    if largest:
        lab, n = nd.label(a); big = np.argmax(nd.sum(a, lab, range(1, n + 1))) + 1; a = lab == big
    ys, xs = np.nonzero(a)
    return xs.min(), ys.min(), xs.max() + 1, ys.max() + 1

def save(name, src, largest=False, width=None, height=None, pad=4):
    im = Image.open(SRC / src).convert("RGBA")
    x0, y0, x1, y1 = obj_box(im, largest)
    im = im.crop((max(0, x0 - pad), max(0, y0 - pad), min(im.width, x1 + pad), min(im.height, y1 + pad)))
    if width: im = im.resize((width, round(im.height * width / im.width)), Image.LANCZOS)
    if height: im = im.resize((round(im.width * height / im.height), height), Image.LANCZOS)
    im.save(OUT / f"{name}.webp", "WEBP", quality=90, method=6)
    print(name, im.size)

Image.open(SRC / "SMOKE_BG_Prune_Fumee_1536x1024.png").convert("RGB").save(OUT / "bg.webp", "WEBP", quality=86, method=6)
for n, f in [("logo_prune", "SMOKE_Logo.png"), ("logo_bleu", "SMOKE_Logo_Bleu.png"), ("logo_vert", "SMOKE_Logo_Vert.png")]: save(n, f, width=640)
save("lockup", "SMOKE_Logo_RoomLabs_Audio.png", width=520)
for n, f in [("icon_eq", "SMOKE_Icon_Egaliseur.png"), ("icon_gear", "SMOKE_Icon_Reglages.png"), ("icon_power", "SMOKE_Icon_Power.png")]: save(n, f, height=96)
for n, f in [("yeux_normaux", "SMOKE_Yeux_Normaux.png"), ("yeux_rouges", "SMOKE_Yeux_Rouges.png")]: save(n, f, width=760)
for n, f in [("joint_papier", "SMOKE_Joint_Papier.png"), ("joint_woodback", "SMOKE_Joint_Woodback_Large.png"), ("joint_vert", "SMOKE_Joint_Woodback_Mode_Vert.png")]: save(n, f, width=900)
for n, f in [("fumee_prune", "SMOKE_Nappe_Fumee_Animation.png"), ("fumee_bleu", "SMOKE_Nappe_Fumee_Bleue.png"), ("fumee_vert", "SMOKE_Nappe_Fumee_Verte.png")]: save(n, f, height=420)
for n in ["Keys", "Rhodes", "Risers", "Sub"]: save("btn_" + n.lower(), f"SMOKE_Bouton_{n}.png", largest=True, width=300)
for n in ["Tone", "Space", "Dream", "Clean"]: save("knob_" + n.lower(), f"SMOKE_Molette_{n}.png", largest=True, width=200)

# Molettes : le repère dessiné en haut est effacé. La face reste fixe (l'éclairage venu du bas ne tourne pas)
# et l'interface dessine son propre repère et l'anneau de valeur.
import cv2
for n in ["tone", "space", "dream", "clean"]:
    f = OUT / f"knob_{n}.webp"; im = np.array(Image.open(f).convert("RGBA")); h, w = im.shape[:2]
    rgb = np.ascontiguousarray(im[..., :3]); r, g = rgb[..., 0].astype(int), rgb[..., 1].astype(int)
    mask = np.zeros((h, w), np.uint8); top = int(h * .26); cx = w // 2
    sel = (r[:top, cx - 14:cx + 14] > 150) & (r[:top, cx - 14:cx + 14] - g[:top, cx - 14:cx + 14] > 40)
    mask[:top, cx - 14:cx + 14] = sel.astype(np.uint8) * 255
    mask = cv2.dilate(mask, np.ones((7, 7), np.uint8))
    im[..., :3] = cv2.inpaint(rgb, mask, 6, cv2.INPAINT_TELEA)
    Image.fromarray(im).save(f, "WEBP", quality=90, method=6); print(f.name, "repère effacé :", int(mask.sum() / 255), "px")
# icône de l'application
Image.open(SRC / "SMOKE_Joint_Papier.png").convert("RGBA").resize((1024, 1024), Image.LANCZOS).save(OUT / "icon-1024.png")
