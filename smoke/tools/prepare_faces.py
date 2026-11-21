#!/usr/bin/env python3
"""Visages des thèmes Bleu et Vert (bordures de fumée assorties au fond teinté).
Usage : python3 tools/prepare_faces.py <bleu bordure> <bleu lueur> <vert bordure> <vert carré> ui/assets
Le visage « vert carré » (cadrage carré) est découpé avec la silhouette du visage « vert bordure »
pour servir de version intense du thème Vert ; « bleu lueur » sert de version intense du thème Bleu."""
import pathlib, sys
import numpy as np
from PIL import Image, ImageFilter
bb, bl, vb, vc, out = sys.argv[1:6]; OUT = pathlib.Path(out)
ims = {k: Image.open(p).convert("RGBA") for k, p in [("bb", bb), ("bl", bl), ("vb", vb), ("vc", vc)]}
# version intense du vert : image carrée découpée avec la silhouette du visage à bordure verte (bord adouci)
mask = ims["vb"].getchannel("A").filter(ImageFilter.GaussianBlur(3))
vc = ims["vc"].copy(); vc.putalpha(Image.fromarray(np.minimum(np.array(mask), np.array(vc.getchannel("A")))))
ims["vc"] = vc
a = np.zeros(ims["bb"].size[::-1], bool)
for im in ims.values(): a |= np.array(im)[..., 3] > 12
ys, xs = np.nonzero(a); box = (xs.min(), ys.min(), xs.max() + 1, ys.max() + 1)
for k, name in [("bb", "visage_bleu"), ("bl", "visage_bleu_intense"), ("vb", "visage_vert"), ("vc", "visage_vert_intense")]:
    im = ims[k].crop(box); im = im.resize((600, round(im.height * 600 / im.width)), Image.LANCZOS)
    im.save(OUT / f"{name}.webp", "WEBP", quality=88, method=6); print(name, im.size)
