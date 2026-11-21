#!/usr/bin/env python3
"""Construit l'aperçu en ligne : ui/index.html en un seul fichier autonome.

Les images de ui/assets sont intégrées (data:), les presets d'usine aussi
(window.SHK_FACTORY). Sortie : build/preview/index.html
    python3 tools/build_preview.py [dossier_de_sortie]
"""
import base64
import json
import pathlib
import sys

root = pathlib.Path(__file__).resolve().parent.parent
html = (root / "ui" / "index.html").read_text(encoding="utf-8")

for f in sorted((root / "ui" / "assets").glob("*.webp")):
    uri = "data:image/webp;base64," + base64.b64encode(f.read_bytes()).decode()
    html = html.replace(f"assets/{f.name}", uri)
# les danseuses sont chargées par un modèle `assets/dancer_${d.n}.webp` : on remplace le modèle par une table
dancers = {n: "data:image/webp;base64," + base64.b64encode((root / "ui" / "assets" / f"dancer_{n}.webp").read_bytes()).decode()
           for n in ("ceylon", "emeraude")}
html = html.replace("im.src = `assets/dancer_${d.n}.webp`;", "im.src = window.SHK_DANCERS[d.n];")

factory = json.loads((root / "presets" / "factory.json").read_text(encoding="utf-8"))
inject = ("<script>window.SHK_FACTORY=" + json.dumps(factory, ensure_ascii=False)
          + ";window.SHK_DANCERS=" + json.dumps(dancers) + ";</script>\n")
html = html.replace("<script>\n\"use strict\";", inject + "<script>\n\"use strict\";", 1)

out = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else root / "build" / "preview"
out.mkdir(parents=True, exist_ok=True)
(out / "index.html").write_text(html, encoding="utf-8")
print(out / "index.html", f"{len(html) // 1024} Ko")
