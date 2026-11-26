#!/usr/bin/env python3
"""Aperçu autonome (un seul HTML, images intégrées) pour le navigateur / téléphone : docs/apercu.html"""
import base64, os, re
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."); A = os.path.join(ROOT, "ui", "assets")
html = open(os.path.join(ROOT, "ui", "index.html"), encoding="utf-8").read()
def uri(name):
    p = os.path.join(A, name); ext = name.rsplit(".", 1)[1]
    mime = {"webp": "image/webp", "png": "image/png"}[ext]
    return f"data:{mime};base64," + base64.b64encode(open(p, "rb").read()).decode()
files = {f: uri(f) for f in os.listdir(A) if f.endswith((".webp", ".png"))}
js = "window.AFR_FILES=" + __import__("json").dumps(files) + ";"
# les noms de fichiers dans le code et le CSS deviennent des données intégrées
for f, u in files.items():
    html = html.replace(f"url({f})", f"url({u})")
# chargement dynamique (src = `map_${l}.webp` …) : petite fonction de résolution
html = html.replace('"use strict";\nconst $ = s => document.querySelector(s);', '"use strict";\nconst $ = s => document.querySelector(s);\nconst F = n => (window.AFR_FILES && window.AFR_FILES[n]) || n;', 1)
for pat in [r"`bg_\$\{l\}\.webp`", r"`map_\$\{l\}\.webp`", r"`lab_\$\{l\}\.png`", r'`inst_\$\{String\(c\.i\)\.padStart\(2, "0"\)\}\.webp`', r"`knob_uno_\$\{k\}\.webp`", r'"knob_bronze_0\.webp"']:
    html = re.sub(pat, lambda m: "F(" + m.group(0) + ")", html)
html = html.replace("url(${CAT_SPR[c][0]})", "url(${F(CAT_SPR[c][0])})")
html = html.replace('<script src="data.js"></script>', "<script>" + open(os.path.join(A, "data.js"), encoding="utf-8").read() + js + "</script>")
html = html.replace('<script src="rl_license_ui.js"></script>', "<script>" + open(os.path.join(ROOT, "..", "common", "licensing", "rl_license_ui.js"), encoding="utf-8").read() + "</script>")
out = os.path.join(ROOT, "docs", "apercu.html"); open(out, "w", encoding="utf-8").write(html)
print(out, round(len(html) / 1e6, 1), "Mo")
