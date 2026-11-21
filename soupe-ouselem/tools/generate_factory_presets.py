#!/usr/bin/env python3
"""Presets d'usine de « Ma petite soupe ouselem » : 6 sachets × 5 recettes.
Un sachet = un instrument + un module d'effet calé sur le tempo. Écrit presets/factory.json et tests/factory_presets.inc.
Règle maison : chaque nouvelle version ajoute au moins 20 presets."""
import json
import pathlib

INST = ["Violon", "Pad", "Piano", "Cloche", "Cordes", "Texture", "Basse", "Tambour"]
FX = ["Aucun", "Reverse", "Stutter", "Répétitions", "Découpage", "Étirement", "Filtre modulé", "Grain Delay"]
DIV = ["1/2", "1/4", "1/8", "1/16", "1/32", "1/4T", "1/8T", "1/16T"]
NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
SACHETS = [
    (1, "Violon renversé", "violon"), (2, "Pad mijoté", "clavier"), (3, "Gouttes au sésame", "cloche"),
    (4, "Nouilles longues", "violon"), (5, "Bouillon d'arpèges", "onde"), (6, "Fond de bol", "tambour"),
]
presets = []


def add(sachet, name, root, inst, fx, div, **kw):
    q = {"inst": INST.index(inst), "fx": FX.index(fx), "division": DIV.index(div), "reverseOn": 0, "stutterOn": 0,
         "mix": 0.5, "feedback": 0.4, "stretch": 0.5, "tone": 0.0, "damp": 0.4, "space": 0.3, "level": -6.0,
         "attack": 20.0, "decay": 800.0, "sustain": 0.8, "release": 600.0, "brightness": 0.5, "tune": 0.0,
         "arpOn": 0, "arpMode": 0, "arpOct": 1}
    q.update({k: (float(v) if isinstance(v, float) else v) for k, v in kw.items()})
    s = SACHETS[sachet - 1]
    presets.append({"name": name, "sachet": sachet, "section": s[1], "icon": s[2], "root": root,
                    "rootName": f"{NOTE_NAMES[root % 12]}{root // 12 - 1}", "params": q})


# 01 Violon renversé : violon + reverse synchronisé
add(1, "Violon Renversé", 67, "Violon", "Reverse", "1/4", mix=0.7, space=0.35, attack=90.0, release=700.0)
add(1, "Archet Miroir", 64, "Violon", "Reverse", "1/2", mix=0.85, space=0.45, attack=150.0, release=1200.0, brightness=0.4)
add(1, "Retour Vif", 72, "Violon", "Reverse", "1/8", mix=0.6, space=0.25, attack=40.0, release=400.0, brightness=0.7)
add(1, "Grain d'Archet", 67, "Violon", "Grain Delay", "1/8", mix=0.55, feedback=0.7, space=0.4, attack=80.0)
add(1, "Violon Brumeux", 62, "Violon", "Reverse", "1/4T", mix=0.75, space=0.6, damp=0.7, attack=200.0, release=1500.0, reverseOn=0, tone=-0.3)

# 02 Pad mijoté : pad + stutter à divisions musicales
add(2, "Pad Mijoté", 60, "Pad", "Stutter", "1/16", mix=0.6, attack=400.0, release=1500.0, space=0.4)
add(2, "Frémissement", 60, "Pad", "Stutter", "1/32", mix=0.5, attack=300.0, release=1200.0, space=0.35, brightness=0.6)
add(2, "Bouillon Lent", 57, "Pad", "Stutter", "1/8T", mix=0.55, attack=600.0, release=2000.0, space=0.5, tone=-0.2)
add(2, "Piano Mijoté", 60, "Piano", "Stutter", "1/16", mix=0.5, attack=2.0, decay=1500.0, sustain=0.3, release=500.0, space=0.3)
add(2, "Pad Granité", 64, "Pad", "Grain Delay", "1/4", mix=0.5, feedback=0.75, attack=500.0, release=1800.0, space=0.5)

# 03 Gouttes au sésame : percussions « cloc » et « touc » + répétitions
add(3, "Gouttes au Sésame", 72, "Cloche", "Répétitions", "1/8", mix=0.5, feedback=0.55, space=0.3, brightness=0.4)
add(3, "Cloc Touc", 60, "Tambour", "Répétitions", "1/8T", mix=0.45, feedback=0.5, space=0.2, brightness=0.3)
add(3, "Pluie de Graines", 79, "Cloche", "Répétitions", "1/16", mix=0.55, feedback=0.65, space=0.35, brightness=0.2, damp=0.6)
add(3, "Goutte Lointaine", 76, "Cloche", "Répétitions", "1/4", mix=0.6, feedback=0.7, space=0.55, damp=0.75, brightness=0.6)
add(3, "Touc Grave", 48, "Tambour", "Répétitions", "1/4T", mix=0.4, feedback=0.45, space=0.25, brightness=0.5)

# 04 Nouilles longues : cordes ou texture + étirement et relâchement prolongé
add(4, "Nouilles Longues", 60, "Cordes", "Étirement", "1/4", mix=0.55, stretch=0.6, attack=300.0, release=3000.0, space=0.45)
add(4, "Fil Étiré", 67, "Cordes", "Étirement", "1/4", mix=0.7, stretch=0.85, attack=250.0, release=4000.0, space=0.5, brightness=0.6)
add(4, "Pâte Souple", 55, "Texture", "Étirement", "1/2", mix=0.6, stretch=0.7, attack=500.0, release=3500.0, space=0.55, tone=-0.2)
add(4, "Cordes Tirées", 64, "Cordes", "Grain Delay", "1/8", mix=0.45, feedback=0.8, attack=200.0, release=2500.0, space=0.4)
add(4, "Ramen Infini", 60, "Texture", "Étirement", "1/4", mix=0.8, stretch=0.95, attack=800.0, release=5000.0, space=0.65, damp=0.7)

# 05 Bouillon d'arpèges : texture jouée en arpège + découpage rythmique
add(5, "Bouillon d'Arpèges", 60, "Texture", "Découpage", "1/16", arpOn=1, mix=0.55, attack=5.0, decay=300.0, sustain=0.5, release=200.0, space=0.3)
add(5, "Bulles Montantes", 64, "Texture", "Découpage", "1/16T", arpOn=1, arpOct=2, mix=0.5, attack=5.0, decay=250.0, sustain=0.4, release=180.0, space=0.35, brightness=0.7)
add(5, "Mijotage Haché", 57, "Pad", "Découpage", "1/16", arpOn=1, mix=0.65, attack=10.0, release=250.0, space=0.25)
add(5, "Arpège Descendant", 67, "Texture", "Grain Delay", "1/8", arpOn=1, arpMode=1, arpOct=2, mix=0.45, feedback=0.6, attack=5.0, decay=300.0, sustain=0.4, release=200.0)
add(5, "Clavier Découpé", 60, "Piano", "Découpage", "1/8T", arpOn=1, mix=0.5, attack=2.0, decay=900.0, sustain=0.3, release=250.0, space=0.25)

# 06 Fond de bol : basse sourde et douce + filtre modulé
add(6, "Fond de Bol", 36, "Basse", "Filtre modulé", "1/4", mix=0.6, attack=10.0, release=300.0, space=0.1, brightness=0.4)
add(6, "Bouillon Grave", 33, "Basse", "Filtre modulé", "1/2", mix=0.7, attack=15.0, release=400.0, space=0.15, brightness=0.6)
add(6, "Sous la Cuillère", 38, "Basse", "Filtre modulé", "1/8", mix=0.5, attack=5.0, release=250.0, space=0.1, brightness=0.5)
add(6, "Touc Profond", 36, "Tambour", "Filtre modulé", "1/4T", mix=0.45, attack=1.0, release=200.0, space=0.15, brightness=0.6)
add(6, "Basse Feutrée", 40, "Basse", "Aucun", "1/4", mix=0.0, attack=20.0, release=350.0, space=0.1, brightness=0.25, tone=-0.3)

assert len(presets) == 30
assert len({p["name"] for p in presets}) == 30

root = pathlib.Path(__file__).resolve().parent.parent
lv = root / "presets" / "levels.json"
if lv.exists():
    levels = json.loads(lv.read_text(encoding="utf-8"))
    for p in presets:
        if p["name"] in levels: p["params"]["level"] = levels[p["name"]]
(root / "presets").mkdir(exist_ok=True)
(root / "presets" / "factory.json").write_text(json.dumps(
    {"version": 1, "sachets": [{"n": n, "name": s, "icon": i} for n, s, i in SACHETS], "instruments": INST, "fx": FX,
     "divisions": DIV, "presets": presets}, ensure_ascii=False, indent=1), encoding="utf-8")
ORDER = ["inst", "fx", "division", "reverseOn", "stutterOn", "mix", "feedback", "stretch", "tone", "damp", "space", "level",
         "attack", "decay", "sustain", "release", "brightness", "tune", "arpOn", "arpMode", "arpOct"]
lines = ["// Généré par tools/generate_factory_presets.py", "// nom, note, " + ", ".join(ORDER)]
for p in presets:
    lines.append('{ "%s", %d, { %s } },' % (p["name"].replace('"', ""), p["root"], ", ".join("%.4ff" % float(p["params"][k]) for k in ORDER)))
(root / "tests").mkdir(exist_ok=True)
(root / "tests" / "factory_presets.inc").write_text("\n".join(lines) + "\n", encoding="utf-8")
print(len(presets), "presets")
