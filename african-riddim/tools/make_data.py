#!/usr/bin/env python3
"""Écrit ui/assets/data.js : pays, instruments, catégories, ancres sur chaque carte (depuis map.json)."""
import json, os, sys
sys.path.insert(0, os.path.dirname(__file__)); from countries import C
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
meta = json.load(open(os.path.join(ROOT, "ui", "assets", "map.json"), encoding="utf-8"))
data = { "mapPlace": meta["mapPlace"], "countries": [], "anchors": { k: v["anchors"] for k, v in meta["maps"].items() } }
for i, c in enumerate(C):
    data["countries"].append({ "i": i, "id": c[0], "name": c[1], "short": c[2], "inst": c[3], "cat": c[6] })
open(os.path.join(ROOT, "ui", "assets", "data.js"), "w", encoding="utf-8").write("window.AFR_DATA = " + json.dumps(data, ensure_ascii=False) + ";\n")
print("data.js :", len(C), "pays")
