#!/usr/bin/env python3
"""Presets d'usine de Windup Dogs : 5 instruments × 8 = 40. Écrit presets/factory.json et tests/factory_presets.inc.
Règle maison : chaque nouvelle version ajoute au moins 20 presets."""
import json, pathlib
BANKS = ["Boîte à musique", "Piano jouet", "Clochettes", "Fanfare", "Tambour"]
DOGS = ["coureur", "chef", "cloches", "trompette", "tambour"]
NOTE = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
presets = []
DEF = {"inst": 0, "tone": .75, "meca": .35, "wobble": .15, "echo": .15, "space": .3, "level": -6.0, "attack": 1.0, "decay": 2500.0, "sustain": 0.0,
       "release": 600.0, "bright": .5, "tune": 0.0, "tickOn": 0, "tickDiv": 1, "tickLevel": .4, "swing": 0.0, "springOn": 1, "springTime": 40.0}
def add(bank, name, root, **kw):
    q = dict(DEF); q["inst"] = bank; q.update({k: (float(v) if isinstance(v, float) else v) for k, v in kw.items()})
    presets.append({"name": name, "bank": bank, "section": BANKS[bank], "root": root, "rootName": f"{NOTE[root % 12]}{root // 12 - 1}", "params": q})

B = 0   # boîte à musique
for n, r, kw in [("Berceuse Remontée", 72, dict(meca=.35, wobble=.12, space=.35, decay=2800.0)), ("Tiroir de Grand-Mère", 72, dict(meca=.55, wobble=.3, tone=.6, space=.25, decay=2200.0)),
                 ("Ballerine", 76, dict(bright=.7, space=.45, echo=.2, decay=3000.0)), ("Peigne d'Acier", 72, dict(bright=.85, meca=.25, wobble=.05, space=.2)),
                 ("Boîte Fatiguée", 69, dict(wobble=.6, meca=.6, tone=.55, springTime=15.0, space=.3)), ("Neige sur le Carrousel", 79, dict(space=.75, echo=.35, bright=.6, decay=3500.0)),
                 ("Tic-Tac Mélodie", 72, dict(tickOn=1, tickDiv=1, tickLevel=.45, meca=.3, space=.3)), ("Rêve Mécanique", 72, dict(space=.9, echo=.45, wobble=.25, tone=.65, decay=4000.0, release=1500.0))]:
    add(B, n, r, **kw)
B = 1   # piano jouet
for n, r, kw in [("Piano de Poche", 72, dict(meca=.4, space=.2, decay=1500.0)), ("Salle de Jeux", 67, dict(meca=.5, space=.45, bright=.6, decay=1700.0)),
                 ("Petit Concert", 72, dict(bright=.7, space=.35, echo=.15)), ("Piano Grinçant", 64, dict(wobble=.45, meca=.75, tone=.55, springTime=20.0)),
                 ("Marteaux de Feutre", 67, dict(bright=.25, tone=.5, meca=.2, decay=1300.0)), ("Cour de Récré", 72, dict(tickOn=1, tickDiv=2, tickLevel=.35, swing=.25, space=.25)),
                 ("Piano Hanté", 60, dict(space=.85, echo=.45, wobble=.4, tone=.5, decay=2400.0, release=1600.0)), ("Jouet Neuf", 76, dict(bright=.9, meca=.3, wobble=.03, space=.15))]:
    add(B, n, r, **kw)
B = 2   # clochettes
for n, r, kw in [("Clochettes d'Or", 79, dict(space=.45, decay=3200.0, bright=.6)), ("Grelots du Traîneau", 84, dict(bright=.75, decay=1400.0, echo=.25, space=.35)),
                 ("Cloche de Comptoir", 72, dict(meca=.5, bright=.5, decay=2600.0, space=.2)), ("Carillon du Manège", 76, dict(space=.6, echo=.3, decay=4000.0)),
                 ("Clochette Rouillée", 74, dict(tone=.5, wobble=.35, meca=.55, decay=2000.0)), ("Étoile Filante", 84, dict(space=.9, echo=.5, bright=.85, decay=5000.0, release=2000.0)),
                 ("Petite Église", 67, dict(space=.75, decay=5500.0, bright=.4, release=2500.0)), ("Cloches au Tempo", 79, dict(tickOn=1, tickDiv=1, tickLevel=.35, echo=.3, space=.4))]:
    add(B, n, r, **kw)
B = 3   # fanfare
for n, r, kw in [("Trompette en Fer", 67, dict(attack=15.0, decay=400.0, sustain=.75, release=180.0, bright=.55, space=.25)), ("Clairon du Chien", 72, dict(attack=8.0, sustain=.8, release=150.0, bright=.8, space=.2)),
                 ("Fanfare de Poche", 64, dict(attack=20.0, sustain=.7, release=250.0, bright=.45, echo=.2, space=.3)), ("Kazoo Mécanique", 67, dict(attack=10.0, sustain=.9, release=120.0, bright=.9, wobble=.35, meca=.5)),
                 ("Parade Fatiguée", 62, dict(attack=25.0, sustain=.7, release=300.0, wobble=.5, springTime=18.0, tone=.6)), ("Cor de Chasse Jouet", 60, dict(attack=40.0, sustain=.8, release=400.0, bright=.3, space=.45)),
                 ("Marche au Tic-Tac", 67, dict(attack=12.0, sustain=.75, release=160.0, tickOn=1, tickDiv=1, tickLevel=.5, space=.2)), ("Trompette dans la Brume", 64, dict(attack=60.0, sustain=.85, release=900.0, space=.8, echo=.4, tone=.55))]:
    add(B, n, r, **kw)
B = 4   # tambour (une percussion par touche)
for n, r, kw in [("Tambour du Chien", 60, dict(meca=.3, space=.2)), ("Boîte à Jouets", 60, dict(meca=.5, space=.35, tone=.7)),
                 ("Percussions de Bois", 61, dict(bright=.6, space=.25)), ("Fanfare Miniature", 60, dict(echo=.25, space=.3)),
                 ("Grenier Poussiéreux", 60, dict(tone=.5, wobble=.3, meca=.6, space=.45)), ("Rythme Remonté", 60, dict(tickOn=1, tickDiv=2, tickLevel=.45, swing=.3, space=.2)),
                 ("Petit Cirque", 60, dict(space=.55, echo=.3, bright=.7)), ("Jouets en Échos", 60, dict(echo=.55, space=.5, tone=.6))]:
    add(B, n, r, **kw)

assert len(presets) == 40 and len({p["name"] for p in presets}) == 40
root = pathlib.Path(__file__).resolve().parent.parent
lv = root / "presets" / "levels.json"
if lv.exists():
    levels = json.loads(lv.read_text(encoding="utf-8"))
    for p in presets:
        if p["name"] in levels: p["params"]["level"] = levels[p["name"]]
(root / "presets").mkdir(exist_ok=True)
(root / "presets" / "factory.json").write_text(json.dumps({"version": 1, "banks": BANKS, "dogs": DOGS, "presets": presets}, ensure_ascii=False, indent=1), encoding="utf-8")
ORDER = ["inst", "tone", "meca", "wobble", "echo", "space", "level", "attack", "decay", "sustain", "release", "bright", "tune", "tickOn", "tickDiv", "tickLevel", "swing", "springOn", "springTime"]
lines = ["// Généré par tools/generate_factory_presets.py", "// nom, note, " + ", ".join(ORDER)]
for p in presets:
    lines.append('{ "%s", %d, { %s } },' % (p["name"], p["root"], ", ".join("%.4ff" % float(p["params"][k]) for k in ORDER)))
(root / "tests").mkdir(exist_ok=True)
(root / "tests" / "factory_presets.inc").write_text("\n".join(lines) + "\n", encoding="utf-8")
print(len(presets), "presets")
