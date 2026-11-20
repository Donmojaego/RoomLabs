#!/usr/bin/env python3
"""Génère presets/factory.json (presets d'usine de Candy Shop) et tests/factory_presets.inc.

8 bonbons × 10 presets + 5 kits de boîte à rythmes (le kit est un réglage de chaque preset).
Les noms sont originaux, sans marque. Règle maison : chaque nouvelle version ajoute au moins 20 presets.
"""
import json
import pathlib

ALGO = {"mallet": 0, "synth": 1, "fm": 2}
WAVE = {"sine": 0, "tri": 1, "saw": 2, "square": 3}
CHORD = {"": 0, "maj": 1, "min": 2, "maj7": 3, "m7": 4, "sus4": 5, "power": 6, "add9": 7}
NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
KITS = ["Bocal", "Sucre Sec", "Caramel Mou", "Pétillant", "Mécanique"]

CANDIES = [
    ("fraise", "Plug Seed Pack"), ("banane", "Marimbas"), ("cola", "Synths"), ("violet", "Pads"),
    ("cerise", "Percussion"), ("sucette", "Sucette"), ("carres", "Carrés fruités"), ("lacets", "Lacets acidulés"),
]


def note_name(n):
    return f"{NOTE_NAMES[n % 12]}{n // 12 - 1}"


def base():
    return {"algo": 1, "wave": 2, "detune": 8.0, "unison": 2, "fmRatio": 3.5, "fmAmount": 0.0, "fmDecay": 0.4,
            "noise": 0.0, "pitchDrop": 0.0, "tune": 0.0, "attack": 2.0, "decay": 300.0, "sustain": 0.7,
            "release": 250.0, "tone": 0.0, "cutoff": 8000.0, "reso": 0.1, "fenv": 0.3, "fdecay": 250.0,
            "space": 0.2, "chorus": 0.0, "level": -6.0, "velSens": 0.6, "chord": 0, "arpOn": 0, "arpRate": 3,
            "arpMode": 0, "arpOct": 1, "arpGate": 0.6, "kit": 0, "drumLevel": -4.0}


presets = []


def add(candy, name, root, kit=0, **kw):
    q = base()
    for k, v in kw.items():
        if k == "algo": v = ALGO[v]
        if k == "wave": v = WAVE[v]
        if k == "chord": v = CHORD[v]
        q[k] = float(v) if isinstance(v, float) else v
    q["kit"] = kit
    label = dict(CANDIES)[candy]
    presets.append({"name": name, "candy": candy, "section": label, "root": root, "rootName": note_name(root), "params": q})


# ---------------- Plug Seed Pack (fraise) : cloches plugg, synthés de fête, accents percussifs
for i, (n, r, rt, amt, dec, drop, sp, kit) in enumerate([
    ("Graine Brillante", 72, 3.5, 0.55, 0.35, 0, 0.25, 3), ("Cloche Plugg", 72, 2.0, 0.6, 0.6, 0, 0.3, 0),
    ("Fraise Fizz", 76, 5.0, 0.45, 0.25, 2, 0.2, 3), ("Paillette", 79, 7.0, 0.35, 0.18, 0, 0.35, 1),
    ("Tintement Rose", 72, 3.0, 0.7, 0.9, 0, 0.4, 0), ("Pop Sucré", 67, 1.5, 0.5, 0.15, 7, 0.15, 3),
    ("Semence", 69, 4.0, 0.4, 0.5, 0, 0.3, 4), ("Accent Fête", 64, 1.0, 0.8, 0.12, 12, 0.1, 3),
    ("Bulle Rouge", 74, 2.5, 0.3, 0.4, 3, 0.25, 0), ("Plugg Nuit", 67, 3.5, 0.65, 1.2, 0, 0.5, 2)]):
    add("fraise", n, r, kit, algo="fm", fmRatio=rt, fmAmount=amt, fmDecay=dec, pitchDrop=drop, attack=1.0,
        decay=300 + dec * 900, sustain=0.0, release=300.0, cutoff=12000.0, fenv=0.1, space=sp, level=-8.0, tone=0.2)

# ---------------- Marimbas (banane)
for i, (n, r, dec, tn, sp, drop, kit) in enumerate([
    ("Marimba Rond", 60, 0.35, 0.0, 0.2, 0, 0), ("Banane Rebond", 64, 0.25, 0.3, 0.15, 1, 3),
    ("Bois Doux", 57, 0.5, -0.4, 0.25, 0, 0), ("Marimba Sec", 60, 0.15, 0.2, 0.05, 0, 1),
    ("Tropique", 67, 0.4, 0.4, 0.35, 0, 2), ("Lame Douce", 55, 0.6, -0.2, 0.3, 0, 0),
    ("Rebondissant", 62, 0.22, 0.5, 0.2, 2, 3), ("Marimba Lointain", 60, 0.45, -0.1, 0.6, 0, 2),
    ("Kalimba Jaune", 69, 0.3, 0.6, 0.25, 0, 1), ("Grave Bois", 48, 0.7, -0.3, 0.2, 0, 4)]):
    add("banane", n, r, kit, algo="mallet", fmDecay=dec, tone=tn, space=sp, pitchDrop=drop, attack=0.5,
        decay=dec * 1500, sustain=0.0, release=200.0, level=-6.0, velSens=0.7)

# ---------------- Synths (cola) : lead pétillant
for i, (n, r, w, det, un, cut, fe, rs, sp, kit) in enumerate([
    ("Cola Pétillant", 72, "saw", 14, 3, 2500, 0.5, 0.25, 0.2, 3), ("Bulles Lead", 72, "square", 10, 2, 3000, 0.4, 0.2, 0.15, 3),
    ("Sirop Vif", 76, "saw", 20, 3, 1800, 0.6, 0.35, 0.25, 0), ("Gazeux", 69, "saw", 8, 2, 4000, 0.3, 0.1, 0.3, 1),
    ("Capsule", 72, "square", 4, 1, 2200, 0.55, 0.4, 0.1, 4), ("Néon Sucre", 74, "saw", 25, 3, 3200, 0.45, 0.15, 0.35, 3),
    ("Zeste", 77, "tri", 6, 2, 6000, 0.2, 0.1, 0.2, 0), ("Effervescent", 72, "saw", 16, 3, 1500, 0.7, 0.5, 0.2, 3),
    ("Soda Rétro", 67, "square", 12, 2, 2800, 0.3, 0.2, 0.25, 2), ("Pétille Grave", 60, "saw", 10, 3, 1200, 0.6, 0.3, 0.15, 4)]):
    add("cola", n, r, kit, algo="synth", wave=w, detune=float(det), unison=un, cutoff=float(cut), fenv=fe, reso=rs,
        fdecay=220.0, attack=2.0, decay=400.0, sustain=0.75, release=200.0, space=sp, chorus=0.15, level=-9.0)

# ---------------- Pads (violet)
for i, (n, r, w, det, att, rel, cut, sp, ch, kit) in enumerate([
    ("Violet Souple", 60, "saw", 18, 600, 1200, 1800, 0.5, 0.5, 2), ("Nuage Mauve", 60, "tri", 10, 900, 1800, 3000, 0.6, 0.6, 2),
    ("Guimauve", 57, "saw", 22, 400, 1000, 1200, 0.45, 0.4, 0), ("Coloré", 64, "square", 14, 500, 1300, 2200, 0.5, 0.5, 1),
    ("Velours Raisin", 55, "saw", 12, 1200, 2000, 900, 0.55, 0.3, 2), ("Aube Sucrée", 67, "tri", 8, 800, 1600, 5000, 0.7, 0.6, 0),
    ("Doux Halo", 60, "sine", 6, 1000, 2000, 6000, 0.7, 0.7, 2), ("Pastel", 62, "saw", 16, 700, 1500, 2600, 0.6, 0.55, 1),
    ("Lavande", 59, "square", 20, 600, 1400, 1500, 0.5, 0.45, 4), ("Gelée Planante", 60, "saw", 25, 1500, 2000, 2000, 0.8, 0.6, 2)]):
    add("violet", n, r, kit, algo="synth", wave=w, detune=float(det), unison=3, attack=float(att), decay=800.0, sustain=0.85,
        release=float(rel), cutoff=float(cut), fenv=0.1, reso=0.1, space=sp, chorus=ch, level=-2.0, velSens=0.3)

# ---------------- Percussion (cerise) : synthés courts et brillants, notes percussives
for i, (n, r, algo, w, dec, cut, fe, nz, drop, kit) in enumerate([
    ("Cerise Pluck", 72, "synth", "saw", 180, 1500, 0.8, 0.0, 0, 3), ("Blip Brillant", 79, "synth", "square", 90, 3000, 0.6, 0.0, 0, 3),
    ("Noyau", 60, "fm", "sine", 150, 9000, 0.0, 0.0, 5, 0), ("Tic Sucré", 84, "synth", "square", 60, 5000, 0.5, 0.1, 0, 4),
    ("Goutte", 72, "fm", "sine", 200, 9000, 0.0, 0.0, 12, 1), ("Craquant", 67, "synth", "saw", 120, 2000, 0.9, 0.25, 0, 3),
    ("Perle Rouge", 76, "fm", "sine", 260, 9000, 0.0, 0.0, 0, 0), ("Tom Bonbon", 48, "synth", "sine", 220, 3000, 0.2, 0.05, 14, 2),
    ("Clochette", 84, "fm", "sine", 300, 9000, 0.0, 0.0, 0, 1), ("Pincée Vive", 64, "synth", "tri", 140, 4000, 0.7, 0.0, 2, 3)]):
    extra = {"fmRatio": 3.0 + i % 3, "fmAmount": 0.45, "fmDecay": dec / 1000 * 0.8} if algo == "fm" else {}
    add("cerise", n, r, kit, algo=algo, wave=w, attack=0.5, decay=float(dec), sustain=0.0, release=80.0, cutoff=float(cut),
        fenv=fe, fdecay=float(dec) * 0.6, noise=nz, pitchDrop=drop, unison=1, space=0.15, level=-7.0, velSens=0.75, **extra)

# ---------------- Sucette : lead mélodique
for i, (n, r, w, un, det, cut, att, sp, ch, kit) in enumerate([
    ("Sucette Douce", 72, "tri", 1, 0, 5000, 10, 0.3, 0.2, 0), ("Spirale", 72, "square", 2, 6, 3500, 5, 0.25, 0.3, 1),
    ("Mélodie Rose", 76, "saw", 2, 8, 2800, 8, 0.35, 0.25, 0), ("Bâton Sucré", 69, "square", 1, 0, 2000, 3, 0.2, 0.0, 4),
    ("Flûte Bonbon", 74, "sine", 1, 0, 7000, 40, 0.4, 0.3, 2), ("Chant Caramel", 67, "tri", 2, 5, 3000, 25, 0.35, 0.4, 2),
    ("Tourbillon", 72, "saw", 3, 12, 3800, 6, 0.3, 0.4, 3), ("Sifflet", 84, "sine", 1, 0, 9000, 15, 0.3, 0.0, 1),
    ("Lead Barbe à Papa", 72, "square", 3, 10, 2600, 12, 0.45, 0.5, 2), ("Sucette Rétro", 67, "square", 2, 4, 1800, 4, 0.2, 0.1, 4)]):
    add("sucette", n, r, kit, algo="synth", wave=w, unison=un, detune=float(det), cutoff=float(cut), attack=float(att),
        decay=500.0, sustain=0.8, release=300.0, fenv=0.25, reso=0.15, space=sp, chorus=ch, level=-8.0)

# ---------------- Carrés fruités : accords courts et jouables
for i, (n, r, ch, w, dec, cut, sp, kit) in enumerate([
    ("Carré Majeur", 60, "maj", "saw", 350, 2200, 0.25, 0), ("Carré Mineur", 60, "min", "saw", 350, 2200, 0.25, 0),
    ("Fruit Sept", 60, "maj7", "square", 400, 2600, 0.3, 1), ("Acidulé m7", 57, "m7", "saw", 300, 1800, 0.3, 2),
    ("Suspendu", 62, "sus4", "tri", 450, 4000, 0.35, 0), ("Coup de Quinte", 55, "power", "saw", 250, 1500, 0.15, 4),
    ("Neuvième Douce", 60, "add9", "tri", 500, 3500, 0.4, 2), ("Stab Fruité", 64, "min", "square", 180, 2000, 0.15, 3),
    ("Accord Gelée", 59, "maj7", "saw", 600, 1600, 0.45, 2), ("Pop Accords", 62, "maj", "square", 220, 3000, 0.2, 3)]):
    add("carres", n, r, kit, algo="synth", wave=w, chord=ch, unison=2, detune=10.0, attack=2.0, decay=float(dec), sustain=0.2,
        release=250.0, cutoff=float(cut), fenv=0.45, fdecay=float(dec) * 0.7, reso=0.15, space=sp, chorus=0.25, level=-13.0)

# ---------------- Lacets acidulés : arpèges élastiques
for i, (n, r, rate, mode, octs, gate, w, ch, cut, kit) in enumerate([
    ("Lacet Montant", 60, 3, 0, 2, 0.5, "saw", "", 2400, 3), ("Élastique", 60, 3, 2, 2, 0.6, "square", "", 2800, 3),
    ("Spaghetti Acide", 57, 5, 0, 1, 0.4, "saw", "min", 1600, 4), ("Ficelle Triolet", 60, 4, 2, 2, 0.5, "tri", "", 4000, 0),
    ("Descente Sucrée", 64, 3, 1, 2, 0.6, "saw", "", 2600, 1), ("Hasard Fruité", 60, 3, 3, 2, 0.5, "square", "maj", 3000, 3),
    ("Lacet Lent", 60, 1, 0, 2, 0.7, "tri", "", 5000, 2), ("Arpège Accord", 60, 3, 0, 1, 0.5, "saw", "m7", 2200, 0),
    ("Rebond Acidulé", 62, 4, 4, 1, 0.45, "square", "", 2000, 3), ("Tresse", 55, 3, 2, 3, 0.55, "saw", "sus4", 2500, 4)]):
    add("lacets", n, r, kit, algo="synth", wave=w, chord=ch, arpOn=1, arpRate=rate, arpMode=mode, arpOct=octs,
        arpGate=gate, unison=2, detune=8.0, attack=1.0, decay=200.0, sustain=0.5, release=120.0, cutoff=float(cut),
        fenv=0.5, fdecay=150.0, reso=0.3, space=0.25, chorus=0.2, level=-9.0)

# niveaux calibrés (tools/calibrate_levels.cpp) : chaque preset vers -6 dBFS
_lv = pathlib.Path(__file__).resolve().parent.parent / "presets" / "levels.json"
if _lv.exists():
    _levels = json.loads(_lv.read_text(encoding="utf-8"))
    for p in presets:
        if p["name"] in _levels:
            p["params"]["level"] = _levels[p["name"]]

assert len(presets) == 80, len(presets)
assert len({p["name"] for p in presets}) == len(presets), "noms en double"

root = pathlib.Path(__file__).resolve().parent.parent
(root / "presets" / "factory.json").write_text(json.dumps(
    {"version": 1, "candies": [{"id": c, "label": l} for c, l in CANDIES], "kits": KITS, "presets": presets},
    ensure_ascii=False, indent=1), encoding="utf-8")

ORDER = ["algo", "wave", "detune", "unison", "fmRatio", "fmAmount", "fmDecay", "noise", "pitchDrop", "tune", "attack",
         "decay", "sustain", "release", "tone", "cutoff", "reso", "fenv", "fdecay", "space", "chorus", "level", "velSens",
         "chord", "arpOn", "arpRate", "arpMode", "arpOct", "arpGate", "kit", "drumLevel"]
lines = ["// Généré par tools/generate_factory_presets.py — ne pas modifier à la main",
         "// nom, note, " + ", ".join(ORDER)]
for p in presets:
    q = p["params"]
    lines.append('{ "%s", %d, { %s } },' % (p["name"].replace('"', ""), p["root"], ", ".join("%.4ff" % float(q[k]) for k in ORDER)))
(root / "tests" / "factory_presets.inc").write_text("\n".join(lines) + "\n", encoding="utf-8")
print(f"{len(presets)} presets, {len(KITS)} kits")
