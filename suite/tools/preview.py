#!/usr/bin/env python3
"""Aperçu autonome d'un plugin (un seul fichier HTML, images intégrées) pour le navigateur / téléphone.
   python tools/preview.py <dossier_plugin> <produit vst/xxx> <sortie.html>"""
import base64, json, os, re, sys
plug, product, out = sys.argv[1:4]
root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
A = os.path.join(root, "assets")
assets = {}
for d in (os.path.join(A, product), os.path.join(A, "_commandes")):
    for f in sorted(os.listdir(d)):
        if f.endswith(".webp"):
            assets[f] = "data:image/webp;base64," + base64.b64encode(open(os.path.join(d, f), "rb").read()).decode()
meta = json.load(open(os.path.join(A, "meta.json")))
html = open(os.path.join(root, "plugins", plug, "ui", "index.html"), encoding="utf-8").read()
css = open(os.path.join(root, "common", "ui", "rl.css"), encoding="utf-8").read()
js = open(os.path.join(root, "common", "ui", "rl.js"), encoding="utf-8").read()
inject = f"<script>window.RL_ASSETS={json.dumps(assets)};window.RL_META={json.dumps(meta)};</script>\n<script>{js}</script>"
html = html.replace('<link rel="stylesheet" href="rl.css">', f"<style>{css}</style>")
html = html.replace('<script src="rl.js"></script>', inject)
html = html.replace("<meta charset=\"utf-8\">", "<meta charset=\"utf-8\">\n<meta name=\"viewport\" content=\"width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no\">")
open(out, "w", encoding="utf-8").write(html)
print(out, round(len(html) / 1024), "Ko")
