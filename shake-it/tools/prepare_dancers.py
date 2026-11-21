#!/usr/bin/env python3
"""Danseuses de Shake It : planche de sprites par danseuse (pose debout + boucle de 12 images).
Usage : python3 tools/prepare_dancers.py <dossier Shake_It_Boucles> <planche Ceylon 3 poses> <planche Émeraude 3 poses> ui/assets
- Boucle : PNG 01 à 12 (1024 × 1080, déformation depuis la pose accroupie, 1 cycle).
- Pose debout : pose 1 de la planche 3 poses, recalée sur la boucle (même échelle, pieds au même endroit)
  grâce à la pose accroupie de la planche, qui est la base de la boucle.
Sortie : dancer_<nom>.webp (13 images côte à côte : debout, puis 01 à 12) + dancer_<nom>.json (taille d'une image, ligne des pieds)."""
import json, pathlib, sys
import numpy as np
from PIL import Image
from scipy import ndimage as nd
loops, sheets, OUT = pathlib.Path(sys.argv[1]), {"ceylon": sys.argv[2], "emeraude": sys.argv[3]}, pathlib.Path(sys.argv[4])
FOLDER = {"ceylon": "Ceylon_Shorty", "emeraude": "Vert_Emeraude"}
H = 600

def bbox(a): ys, xs = np.nonzero(a); return xs.min(), ys.min(), xs.max() + 1, ys.max() + 1
def figures(im):
    """les 3 silhouettes de la planche, de gauche à droite"""
    a = np.array(im)[..., 3] > 20; lab, n = nd.label(nd.binary_dilation(a, iterations=6))
    sizes = nd.sum(a, lab, range(1, n + 1)); keep = np.argsort(sizes)[-3:] + 1
    boxes = [bbox((lab == k) & a) for k in keep]; return sorted(boxes)
for name, folder in FOLDER.items():
    frames = [Image.open(loops / folder / "PNG" / f"{i:02d}.png").convert("RGBA") for i in range(1, 13)]
    sheet = Image.open(sheets[name]).convert("RGBA"); stand_b, squat_b, _ = figures(sheet)
    f2 = bbox(np.array(frames[1])[..., 3] > 20)
    k = (f2[3] - f2[1]) / (squat_b[3] - squat_b[1])                      # échelle planche -> boucle
    # pieds : bas de la silhouette, centre horizontal de la silhouette accroupie -> même point dans la boucle
    ox = (f2[0] + f2[2]) / 2 - (squat_b[0] + squat_b[2]) / 2 * k
    oy = f2[3] - squat_b[3] * k
    big = sheet.resize((round(sheet.width * k), round(sheet.height * k)), Image.LANCZOS)
    stand = Image.new("RGBA", frames[0].size, (0, 0, 0, 0))
    sx0, sy0, sx1, sy1 = [round(v * k) for v in stand_b]
    piece = big.crop((sx0, sy0, sx1, sy1))
    # la pose debout garde son propre centre horizontal (pieds écartés) mais se cale sur le centre de la boucle
    cx = (f2[0] + f2[2]) / 2
    stand.alpha_composite(piece, (round(cx - piece.width / 2), round(oy + sy0)))
    allf = [stand] + frames
    u = np.zeros(frames[0].size[::-1], bool)
    for f in allf: u |= np.array(f)[..., 3] > 12
    x0, y0, x1, y1 = bbox(u); x0 -= 6; x1 += 6; y0 -= 6; y1 += 4
    s = H / (y1 - y0); w = round((x1 - x0) * s)
    out = Image.new("RGBA", (w * 13, H), (0, 0, 0, 0))
    for i, f in enumerate(allf): out.alpha_composite(f.crop((x0, y0, x1, y1)).resize((w, H), Image.LANCZOS), (i * w, 0))
    out.save(OUT / f"dancer_{name}.webp", "WEBP", quality=88, method=6)
    feet = (f2[3] - y0) * s
    (OUT / f"dancer_{name}.json").write_text(json.dumps({"frameW": w, "frameH": H, "frames": 13, "feetY": round(feet, 1)}))
    print(name, "image", w, "x", H, "échelle planche", round(k, 3), "pieds", round(feet))
