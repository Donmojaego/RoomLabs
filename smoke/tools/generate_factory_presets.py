#!/usr/bin/env python3
"""Presets d'usine de Smoke : 4 banques (Keys, Rhodes, Risers, Sub) × 15 (v0.1.2). Écrit presets/factory.json et tests/factory_presets.inc.
Règle maison : chaque nouvelle version ajoute au moins 20 presets."""
import json, pathlib

MODELS = {"rhodes": 0, "felt": 1, "pad": 2, "riser": 3, "sub": 4, "house": 5}
CHORDS = {"": 0, "m9": 1, "maj9": 2, "m11": 3, "9": 4, "maj7": 5}
BANKS = ["Keys", "Rhodes", "Risers", "Sub"]
NOTE = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
presets = []


def add(bank, name, root, model, **kw):
    q = {"model": MODELS[model], "tone": 0.7, "space": 0.3, "mod": 0.3, "dream": 0.2, "clean": 0.5, "level": -6.0,
         "attack": 5.0, "decay": 1500.0, "sustain": 0.6, "release": 500.0, "bright": 0.5, "tremolo": 0.0, "rise": 4.0,
         "glide": 80.0, "chord": 0, "tune": 0.0}
    for k, v in kw.items():
        if k == "chord": v = CHORDS[v]
        q[k] = float(v) if isinstance(v, float) else v
    presets.append({"name": name, "bank": BANKS.index(bank), "section": bank, "root": root,
                    "rootName": f"{NOTE[root % 12]}{root // 12 - 1}", "params": q})


# ---------------- KEYS : pianos feutrés, claviers traités, accords R&B, pianos qui dérivent
for n, r, ch, br, mod, sp, dr, cl, rel in [
    ("Feutre de Minuit", 60, "", 0.3, 0.15, 0.35, 0.25, 0.6, 700), ("Piano Velours", 60, "", 0.45, 0.1, 0.3, 0.2, 0.7, 600),
    ("Accord m9 Feutré", 57, "m9", 0.35, 0.2, 0.4, 0.3, 0.6, 900), ("Maj9 du Dimanche", 60, "maj9", 0.4, 0.2, 0.4, 0.3, 0.6, 900),
    ("Piano qui Dérive", 60, "", 0.35, 0.7, 0.4, 0.35, 0.5, 800), ("Cassette Douce", 57, "", 0.3, 0.55, 0.25, 0.2, 0.2, 500),
    ("Onze Mineur", 55, "m11", 0.3, 0.25, 0.45, 0.4, 0.6, 1000), ("Neuvième Brumeuse", 60, "9", 0.35, 0.35, 0.5, 0.45, 0.5, 1100),
    ("Feutre Sec", 62, "", 0.5, 0.05, 0.12, 0.05, 0.9, 350), ("Brouillard au Piano", 60, "maj7", 0.25, 0.45, 0.6, 0.7, 0.4, 1500)]:
    add("Keys", n, r, "felt", bright=br, mod=mod, space=sp, dream=dr, clean=cl, release=float(rel), chord=ch,
        decay=3500.0, sustain=0.0, attack=2.0, tone=0.75)

# ---------------- RHODES
for n, r, ch, br, trem, mod, sp, dr, cl in [
    ("Rhodes Velours", 60, "", 0.45, 0.25, 0.2, 0.3, 0.2, 0.6), ("Néon Rhodes", 60, "", 0.7, 0.4, 0.15, 0.25, 0.15, 0.5),
    ("Rhodes m9", 57, "m9", 0.5, 0.3, 0.25, 0.35, 0.25, 0.6), ("Slow Jam", 55, "maj9", 0.4, 0.35, 0.3, 0.4, 0.3, 0.5),
    ("Tine Sucré", 64, "", 0.85, 0.2, 0.1, 0.2, 0.1, 0.8), ("Rhodes Enfumé", 60, "", 0.4, 0.5, 0.55, 0.5, 0.45, 0.3),
    ("Neuf Doux", 58, "9", 0.5, 0.3, 0.3, 0.35, 0.3, 0.5), ("Clean Rhodes", 60, "", 0.55, 0.0, 0.05, 0.15, 0.05, 1.0),
    ("Rhodes Lointain", 60, "maj7", 0.35, 0.3, 0.4, 0.65, 0.7, 0.5), ("Accords de Nuit", 57, "m11", 0.45, 0.4, 0.35, 0.45, 0.4, 0.4)]:
    add("Rhodes", n, r, "rhodes", bright=br, tremolo=trem, mod=mod, space=sp, dream=dr, clean=cl, chord=ch,
        decay=2800.0, sustain=0.15, release=600.0, attack=2.0, tone=0.78)

# ---------------- RISERS : nappes mouvantes, risers, longues queues
for n, r, model, att, rise, br, mod, sp, dr in [
    ("Nappe Prune", 60, "pad", 900, 4, 0.4, 0.4, 0.5, 0.6), ("Montée Lente", 60, "riser", 50, 6, 0.5, 0.3, 0.4, 0.5),
    ("Brume Mouvante", 57, "pad", 1500, 4, 0.3, 0.6, 0.6, 0.75), ("Riser Court", 60, "riser", 20, 2, 0.6, 0.2, 0.35, 0.4),
    ("Nuage Lumineux", 64, "pad", 700, 4, 0.7, 0.35, 0.55, 0.6), ("Ascension", 55, "riser", 100, 8, 0.4, 0.4, 0.5, 0.7),
    ("Plafond de Fumée", 60, "pad", 2000, 4, 0.25, 0.5, 0.7, 0.9), ("Tension Douce", 60, "riser", 300, 4, 0.5, 0.3, 0.45, 0.55),
    ("Queue Infinie", 60, "pad", 1200, 4, 0.35, 0.45, 0.8, 1.0), ("Lever de Rideau", 62, "riser", 30, 3, 0.7, 0.25, 0.4, 0.45)]:
    add("Risers", n, r, model, attack=float(att), rise=float(rise), bright=br, mod=mod, space=sp, dream=dr,
        decay=2000.0, sustain=0.85, release=2500.0, clean=0.6, tone=0.7)

# ---------------- SUB : subs ronds, basses planantes, basses électro (house, hip-hop)
for n, r, model, br, glide, rel, cl, tone, mod in [
    ("Sub Rond", 36, "sub", 0.1, 60, 250, 0.8, 0.5, 0.0), ("Sub Feutré", 33, "sub", 0.0, 80, 300, 0.9, 0.4, 0.05),
    ("Sub Audible", 36, "sub", 0.7, 60, 250, 0.5, 0.6, 0.0), ("Basse Planante", 38, "sub", 0.35, 180, 600, 0.6, 0.55, 0.35),
    ("Glisse Grave", 36, "sub", 0.25, 260, 400, 0.7, 0.5, 0.1), ("House Profonde", 36, "house", 0.4, 30, 200, 0.5, 0.55, 0.05),
    ("Basse Électro", 38, "house", 0.7, 20, 180, 0.3, 0.65, 0.05), ("Garage Rond", 36, "house", 0.3, 50, 220, 0.6, 0.5, 0.1),
    ("Hip-Hop Moelleux", 33, "sub", 0.45, 100, 350, 0.4, 0.5, 0.15), ("Basse de Fumée", 36, "house", 0.5, 120, 400, 0.4, 0.45, 0.4)]:
    add("Sub", n, r, model, bright=br, glide=float(glide), release=float(rel), clean=cl, tone=tone, mod=mod,
        decay=1500.0, sustain=0.85, attack=3.0, space=0.05, dream=0.0)

# ---------------- v0.1.2 : +20 presets (5 par banque)
for n, r, ch, br, mod, sp, dr, cl, rel in [
    ("Velours de Minuit", 57, "m9", 0.3, 0.25, 0.45, 0.4, 0.5, 1000), ("Piano Cendré", 60, "", 0.25, 0.4, 0.35, 0.3, 0.4, 800),
    ("Sept Majeur Feutré", 60, "maj7", 0.4, 0.15, 0.35, 0.25, 0.7, 700), ("Fin de Soirée", 55, "m11", 0.3, 0.5, 0.55, 0.55, 0.4, 1300),
    ("Touches de Soie", 64, "", 0.55, 0.1, 0.25, 0.15, 0.8, 500)]:
    add("Keys", n, r, "felt", bright=br, mod=mod, space=sp, dream=dr, clean=cl, release=float(rel), chord=ch,
        decay=3500.0, sustain=0.0, attack=2.0, tone=0.75)
for n, r, ch, br, trem, mod, sp, dr, cl in [
    ("Rhodes Cannelle", 60, "maj9", 0.45, 0.3, 0.2, 0.35, 0.25, 0.6), ("Trémolo Fumé", 57, "", 0.4, 0.65, 0.3, 0.35, 0.3, 0.4),
    ("Rhodes Ambré", 62, "m9", 0.55, 0.2, 0.15, 0.3, 0.2, 0.7), ("Velvet Tine", 64, "", 0.75, 0.15, 0.1, 0.25, 0.15, 0.7),
    ("Rhodes de l'Aube", 55, "9", 0.35, 0.35, 0.45, 0.55, 0.6, 0.4)]:
    add("Rhodes", n, r, "rhodes", bright=br, tremolo=trem, mod=mod, space=sp, dream=dr, clean=cl, chord=ch,
        decay=2800.0, sustain=0.15, release=600.0, attack=2.0, tone=0.78)
for n, r, model, att, rise, br, mod, sp, dr in [
    ("Volute Lente", 60, "pad", 1100, 4, 0.35, 0.5, 0.6, 0.7), ("Montée de Fumée", 57, "riser", 80, 5, 0.45, 0.35, 0.45, 0.6),
    ("Nappe de Velours", 62, "pad", 800, 4, 0.45, 0.3, 0.5, 0.55), ("Élévation", 60, "riser", 40, 7, 0.55, 0.3, 0.5, 0.65),
    ("Horizon Prune", 55, "pad", 1600, 4, 0.3, 0.55, 0.7, 0.85)]:
    add("Risers", n, r, model, attack=float(att), rise=float(rise), bright=br, mod=mod, space=sp, dream=dr,
        decay=2000.0, sustain=0.85, release=2500.0, clean=0.6, tone=0.7)
for n, r, model, br, glide, rel, cl, tone, mod in [
    ("Sub de Velours", 36, "sub", 0.2, 90, 300, 0.8, 0.45, 0.05), ("House de Nuit", 38, "house", 0.55, 25, 200, 0.4, 0.6, 0.05),
    ("Glisse Fumée", 33, "sub", 0.3, 220, 450, 0.6, 0.5, 0.2), ("Basse Chaleur", 36, "house", 0.35, 60, 260, 0.6, 0.5, 0.1),
    ("Sub Nuage", 34, "sub", 0.15, 140, 500, 0.75, 0.45, 0.3)]:
    add("Sub", n, r, model, bright=br, glide=float(glide), release=float(rel), clean=cl, tone=tone, mod=mod,
        decay=1500.0, sustain=0.85, attack=3.0, space=0.05, dream=0.0)

assert len(presets) == 60 and len({p["name"] for p in presets}) == 60
root = pathlib.Path(__file__).resolve().parent.parent
lv = root / "presets" / "levels.json"
if lv.exists():
    levels = json.loads(lv.read_text(encoding="utf-8"))
    for p in presets:
        if p["name"] in levels: p["params"]["level"] = levels[p["name"]]
(root / "presets").mkdir(exist_ok=True)
(root / "presets" / "factory.json").write_text(json.dumps({"version": 1, "banks": BANKS, "presets": presets}, ensure_ascii=False, indent=1), encoding="utf-8")
ORDER = ["model", "tone", "space", "mod", "dream", "clean", "level", "attack", "decay", "sustain", "release", "bright", "tremolo", "rise", "glide", "chord", "tune"]
lines = ["// Généré par tools/generate_factory_presets.py", "// nom, note, " + ", ".join(ORDER)]
for p in presets:
    lines.append('{ "%s", %d, { %s } },' % (p["name"], p["root"], ", ".join("%.4ff" % float(p["params"][k]) for k in ORDER)))
(root / "tests").mkdir(exist_ok=True)
(root / "tests" / "factory_presets.inc").write_text("\n".join(lines) + "\n", encoding="utf-8")
print(len(presets), "presets")
