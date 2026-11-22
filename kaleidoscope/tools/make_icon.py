#!/usr/bin/env python3
"""Icône provisoire de Kaleidoscope (1024 px) : rosace à 8 miroirs, calculée — remplacée par icone.png du layout dès réception."""
import os, numpy as np
from PIL import Image, ImageFilter
N = 1024
y, x = np.mgrid[0:N, 0:N].astype(np.float32)
cx = cy = N / 2
dx, dy = (x - cx) / (N / 2), (y - cy) / (N / 2)
r = np.sqrt(dx * dx + dy * dy); a = np.arctan2(dy, dx)
seg = np.pi / 8
a = np.abs(((a % (2 * seg)) + 2 * seg) % (2 * seg) - seg)            # repli miroir
u, v = r * np.cos(a), r * np.sin(a)
f = np.sin(u * 13 + np.sin(v * 9) * 2) * np.cos(v * 17 - u * 5) + 0.6 * np.sin(r * 22 - 1.3)
hue = (0.62 + 0.35 * np.sin(r * 6 + f)) % 1.0
sat = 0.75 + 0.25 * np.sin(f * 2)
val = np.clip(0.35 + 0.65 * (0.5 + 0.5 * np.sin(f * 3 + r * 4)), 0, 1) * np.clip(1.15 - r ** 3, 0, 1)
h6 = hue * 6; i = np.floor(h6).astype(int) % 6; fr = h6 - np.floor(h6)
p, q, t = val * (1 - sat), val * (1 - sat * fr), val * (1 - sat * (1 - fr))
rgb = np.zeros((N, N, 3), np.float32)
for k, (R, G, B) in enumerate([(val, t, p), (q, val, p), (p, val, t), (p, q, val), (t, p, val), (val, p, q)]):
    m = i == k; rgb[m, 0], rgb[m, 1], rgb[m, 2] = R[m], G[m], B[m]
ring = np.exp(-((r - 0.93) / 0.025) ** 2)[..., None] * np.array([1.0, 0.82, 0.45])   # anneau laiton
rgb = np.clip(rgb * (r < 0.92)[..., None] + ring, 0, 1)
alpha = np.clip((0.985 - r) * 120, 0, 1)
img = Image.fromarray(np.dstack([rgb * 255, alpha * 255]).astype(np.uint8), "RGBA").filter(ImageFilter.SMOOTH)
out = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "ui", "assets", "icon-1024.png")
img.save(out); print(out)
