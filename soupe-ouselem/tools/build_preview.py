#!/usr/bin/env python3
"""Construit l'aperçu en ligne : ui/index.html en un seul fichier (images et presets intégrés).
Sortie : build/preview/index.html  (ou le dossier passé en argument)"""
import base64, json, pathlib, sys
root = pathlib.Path(__file__).resolve().parent.parent
html = (root / "ui" / "index.html").read_text(encoding="utf-8")
assets = {f.name: "data:image/webp;base64," + base64.b64encode(f.read_bytes()).decode() for f in sorted((root / "ui" / "assets").glob("*.webp"))}
factory = json.loads((root / "presets" / "factory.json").read_text(encoding="utf-8"))
layout = json.loads((root / "ui" / "assets" / "layout.json").read_text(encoding="utf-8"))
inject = "<script>window.SHK_FACTORY=" + json.dumps(factory, ensure_ascii=False) + ";window.SHK_ASSETS=" + json.dumps(assets) + ";window.SHK_LAYOUT=" + json.dumps(layout) + ";</script>\n"
html = html.replace("<script>\n\"use strict\";", inject + "<script>\n\"use strict\";", 1)
out = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else root / "build" / "preview"
out.mkdir(parents=True, exist_ok=True)
(out / "index.html").write_text(html, encoding="utf-8")
print(out / "index.html", f"{len(html) // 1024} Ko")
