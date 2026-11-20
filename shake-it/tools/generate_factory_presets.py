#!/usr/bin/env python3
"""Génère presets/factory.json : les presets d'usine de Shake It.

Chaque preset : nom, section, modèle, note de référence (MIDI), tenue conseillée
et valeurs des paramètres (mêmes identifiants que source/Params.h).
Les noms sont originaux, sans nom d'artiste ni de marque.
Règle maison : chaque nouvelle version ajoute au moins 20 presets.
"""
import json
import pathlib
import random

random.seed(808)

SECTIONS = ["808 · classiques", "808 · modernes", "Sub", "Moog-style", "Bass"]
MODELS = {"808": 0, "Sub": 1, "Moog": 2, "Pluck": 3}
DRIVE = {"soft": 0, "hard": 1, "fold": 2}
NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]


def note_name(n):
    return f"{NOTE_NAMES[n % 12]}{n // 12 - 1}"


def p(name, section, model, root, outfit=0, **kw):
    params = {
        "tune": 0.0, "glide": 120.0, "glideMode": 1, "punch": 0.5, "decay": 1.5,
        "drive": 0.0, "driveType": 0, "output": -3.0, "harm": 0.0, "tone": 18000.0,
        "reso": 0.0, "attack": 1.0, "release": 120.0, "model": MODELS[model],
    }
    for k, v in kw.items():
        if k == "driveType":
            v = DRIVE[v]
        params[k] = v
    return {"name": name, "section": section, "root": root, "rootName": note_name(root),
            "outfit": outfit, "params": params}


presets = []
S1, S2, S3, S4, S5 = SECTIONS

# ---------------------------------------------------------------- 808 · classiques (30)
classic = [
    # nom, note, punch, decay, drive, type, harm, tone, glide
    ("Boom Propre", 29, 0.45, 2.4, 0.0, "soft", 0.10, 18000, 90),
    ("Long Dimanche", 31, 0.35, 4.5, 0.0, "soft", 0.05, 16000, 140),
    ("Courte Sèche", 33, 0.70, 0.45, 0.05, "soft", 0.15, 18000, 60),
    ("Rondeur", 28, 0.25, 2.0, 0.0, "soft", 0.0, 9000, 120),
    ("Coup de Poing", 30, 0.85, 1.2, 0.15, "soft", 0.20, 18000, 80),
    ("Trap Maison", 29, 0.55, 2.8, 0.25, "soft", 0.25, 18000, 110),
    ("Velours Bas", 26, 0.30, 3.2, 0.0, "soft", 0.0, 6000, 150),
    ("Brique", 31, 0.65, 1.6, 0.45, "soft", 0.30, 15000, 100),
    ("Grain Chaud", 29, 0.50, 2.2, 0.35, "soft", 0.35, 12000, 120),
    ("Asphalte", 27, 0.60, 2.6, 0.55, "hard", 0.20, 14000, 120),
    ("Trois Temps", 32, 0.50, 0.9, 0.10, "soft", 0.10, 18000, 70),
    ("Plafond Bas", 24, 0.40, 3.8, 0.0, "soft", 0.15, 10000, 160),
    ("Battement", 30, 0.75, 0.7, 0.20, "soft", 0.10, 18000, 60),
    ("Basse Ville", 29, 0.45, 3.0, 0.30, "soft", 0.40, 16000, 130),
    ("Lourde Nuit", 26, 0.35, 5.0, 0.20, "soft", 0.30, 11000, 180),
    ("Caoutchouc", 31, 0.90, 1.4, 0.0, "soft", 0.05, 18000, 90),
    ("Granite", 28, 0.55, 2.0, 0.65, "hard", 0.25, 13000, 110),
    ("Brume Sous Sol", 25, 0.20, 4.2, 0.10, "soft", 0.0, 5000, 200),
    ("Cloche Basse", 33, 0.60, 1.8, 0.15, "soft", 0.55, 18000, 90),
    ("Studio Sept", 29, 0.50, 2.5, 0.20, "soft", 0.30, 17000, 120),
    ("Tambour Plein", 30, 0.80, 1.0, 0.30, "soft", 0.20, 18000, 70),
    ("Mi-Longue", 28, 0.45, 1.7, 0.10, "soft", 0.15, 15000, 100),
    ("Bitume Chaud", 27, 0.55, 2.4, 0.50, "soft", 0.45, 12000, 140),
    ("Pied Lourd", 26, 0.70, 2.0, 0.35, "hard", 0.20, 14000, 100),
    ("Ombre Ronde", 29, 0.30, 3.4, 0.0, "soft", 0.10, 7000, 150),
    ("Claque", 31, 0.95, 0.6, 0.25, "hard", 0.10, 18000, 60),
    ("Cave Humide", 24, 0.40, 4.0, 0.25, "soft", 0.20, 8000, 170),
    ("Saturée Douce", 29, 0.50, 2.2, 0.70, "soft", 0.30, 13000, 120),
    ("Fond de Cale", 23, 0.35, 5.5, 0.15, "soft", 0.25, 9000, 200),
    ("Classique 29", 29, 0.50, 2.0, 0.15, "soft", 0.20, 18000, 110),
]
for i, (n, r, pu, d, dr, dt, h, t, g) in enumerate(classic):
    presets.append(p(n, S1, "808", r, outfit=i % 3, punch=pu, decay=d, drive=dr, driveType=dt,
                     harm=h, tone=float(t), glide=float(g), release=180.0))

# ---------------------------------------------------------------- 808 · modernes (30)
modern = [
    # drill : glides longs, sauts d'octave
    ("Glisse Drill", 30, 0.55, 3.0, 0.35, "soft", 0.35, 16000, 260, "drill"),
    ("Octave Volée", 29, 0.60, 2.6, 0.30, "soft", 0.30, 15000, 340, "drill"),
    ("Toboggan", 31, 0.50, 3.4, 0.25, "soft", 0.25, 14000, 420, "drill"),
    ("Pente Raide", 28, 0.65, 2.8, 0.45, "hard", 0.30, 14000, 220, "drill"),
    ("Slide Froid", 27, 0.45, 3.6, 0.20, "soft", 0.40, 12000, 380, "drill"),
    ("Long Couloir", 26, 0.40, 4.5, 0.30, "soft", 0.35, 11000, 500, "drill"),
    ("Virage Sec", 30, 0.70, 2.2, 0.40, "soft", 0.25, 16000, 180, "drill"),
    ("Pluie Grise", 29, 0.50, 3.2, 0.25, "soft", 0.45, 10000, 300, "drill"),
    ("Monte Descend", 31, 0.55, 2.9, 0.35, "soft", 0.30, 15000, 280, "drill"),
    ("Escalier", 28, 0.60, 2.4, 0.30, "hard", 0.25, 15000, 240, "drill"),
    # Detroit : attaques sèches, courtes
    ("Rebond Sec", 33, 0.95, 0.35, 0.30, "hard", 0.20, 18000, 40, "detroit"),
    ("Coup Court", 31, 0.90, 0.30, 0.20, "soft", 0.15, 18000, 30, "detroit"),
    ("Moteur Ville", 32, 0.85, 0.45, 0.40, "hard", 0.30, 17000, 50, "detroit"),
    ("Pas Pressé", 30, 1.00, 0.25, 0.25, "soft", 0.10, 18000, 30, "detroit"),
    ("Tic Basse", 34, 0.90, 0.40, 0.35, "hard", 0.25, 18000, 40, "detroit"),
    ("Carrosserie", 29, 0.85, 0.55, 0.45, "hard", 0.35, 16000, 50, "detroit"),
    ("Sprint", 33, 0.95, 0.28, 0.15, "soft", 0.10, 18000, 30, "detroit"),
    ("Hachoir", 31, 0.80, 0.50, 0.50, "hard", 0.20, 15000, 40, "detroit"),
    ("Trottoir", 30, 0.90, 0.38, 0.30, "soft", 0.25, 17000, 30, "detroit"),
    ("Frein à Main", 32, 1.00, 0.32, 0.40, "hard", 0.30, 18000, 40, "detroit"),
    # actuel : textures, distorsion
    ("Papier Verre", 29, 0.60, 2.2, 0.80, "hard", 0.50, 12000, 140, "dist"),
    ("Pliage", 28, 0.55, 2.4, 0.60, "fold", 0.30, 14000, 160, "dist"),
    ("Néon Sale", 30, 0.65, 1.8, 0.75, "soft", 0.60, 13000, 120, "dist"),
    ("Fer Rouillé", 27, 0.50, 2.8, 0.90, "hard", 0.40, 10000, 150, "dist"),
    ("Origami", 29, 0.60, 2.0, 0.45, "fold", 0.25, 16000, 130, "dist"),
    ("Crache Feu", 31, 0.75, 1.5, 1.00, "soft", 0.55, 11000, 100, "dist"),
    ("Sable Noir", 26, 0.45, 3.0, 0.70, "soft", 0.70, 9000, 180, "dist"),
    ("Circuit Ouvert", 28, 0.60, 2.2, 0.55, "fold", 0.45, 13000, 140, "dist"),
    ("Tôle", 30, 0.70, 1.6, 0.85, "hard", 0.35, 14000, 110, "dist"),
    ("Fumée Grasse", 27, 0.50, 2.6, 0.65, "soft", 0.65, 9500, 170, "dist"),
]
for i, (n, r, pu, d, dr, dt, h, t, g, kind) in enumerate(modern):
    gm = 0 if kind == "drill" else 1          # drill : glide toujours (portamento sur chaque note)
    rel = 90.0 if kind == "detroit" else 200.0
    presets.append(p(n, S2, "808", r, outfit=(i + 1) % 3, punch=pu, decay=d, drive=dr, driveType=dt,
                     harm=h, tone=float(t), glide=float(g), glideMode=gm, release=rel))

# ---------------------------------------------------------------- Sub (15)
subs = [
    ("Sinus Profond", 24, 0.0, 8.0, 0.0, 0.0, 18000, 4.0),
    ("Plancher", 26, 0.10, 8.0, 0.0, 0.0, 14000, 3.0),
    ("Feutre", 28, 0.0, 6.0, 0.0, 0.0, 600, 8.0),
    ("Coussin", 29, 0.05, 7.0, 0.0, 0.10, 900, 10.0),
    ("Petit Haut-Parleur", 29, 0.10, 6.0, 0.10, 0.60, 18000, 3.0),
    ("Téléphone Bas", 31, 0.10, 5.0, 0.15, 0.80, 18000, 3.0),
    ("Sub Rond", 27, 0.15, 8.0, 0.0, 0.15, 3000, 5.0),
    ("Vibration", 25, 0.20, 6.5, 0.05, 0.20, 5000, 4.0),
    ("Tapis", 28, 0.0, 8.0, 0.0, 0.05, 1500, 12.0),
    ("Sous-Marin", 23, 0.05, 8.0, 0.0, 0.10, 1200, 6.0),
    ("Chaleur", 29, 0.10, 7.0, 0.25, 0.30, 4000, 4.0),
    ("Barre Grave", 26, 0.20, 6.0, 0.20, 0.45, 18000, 2.0),
    ("Doux Nuage", 30, 0.0, 7.5, 0.0, 0.0, 800, 20.0),
    ("Fondation", 24, 0.15, 8.0, 0.10, 0.25, 8000, 3.0),
    ("Pression", 27, 0.25, 6.0, 0.30, 0.35, 10000, 3.0),
]
for i, (n, r, pu, d, dr, h, t, a) in enumerate(subs):
    presets.append(p(n, S3, "Sub", r, outfit=(i + 2) % 3, punch=pu, decay=d, drive=dr, harm=h,
                     tone=float(t), attack=a, release=220.0, glide=100.0))

# ---------------------------------------------------------------- Moog-style (10)
moogs = [
    ("Filtre Gras", 36, 0.6, 1.2, 0.0, 0.25, 600, 0.45),
    ("Pluck Analogique", 38, 0.8, 0.5, 0.0, 0.0, 450, 0.55),
    ("Carré Rond", 36, 0.4, 2.0, 0.1, 1.0, 900, 0.30),
    ("Résonance", 34, 0.7, 0.8, 0.0, 0.2, 380, 0.80),
    ("Mono Funk", 40, 0.9, 0.35, 0.15, 0.4, 500, 0.60),
    ("Lisse", 33, 0.3, 3.0, 0.0, 0.5, 1200, 0.20),
    ("Couac", 38, 1.0, 0.4, 0.2, 0.1, 300, 0.85),
    ("Acier Tiède", 35, 0.5, 1.5, 0.3, 0.6, 700, 0.40),
    ("Spirale", 36, 0.75, 0.9, 0.0, 0.3, 420, 0.70),
    ("Bas du Clavier", 31, 0.5, 2.2, 0.1, 0.5, 550, 0.35),
]
for i, (n, r, pu, d, dr, h, t, rs) in enumerate(moogs):
    presets.append(p(n, S4, "Moog", r, outfit=i % 3, punch=pu, decay=d, drive=dr, harm=h,
                     tone=float(t), reso=rs, release=90.0, glide=80.0, output=4.0))

# ---------------------------------------------------------------- Bass (10)
basses = [
    ("Corde Pincée", 40, "Pluck", 0.0, 2.5, 0.0, 0.2, 9000),
    ("Doigt Rond", 38, "Pluck", 0.0, 3.0, 0.0, 0.0, 5000),
    ("Électrique", 40, "Pluck", 0.0, 2.0, 0.2, 0.5, 14000),
    ("Mat", 36, "Pluck", 0.0, 1.5, 0.0, 0.0, 2500),
    ("Médiator", 40, "Pluck", 0.0, 1.8, 0.1, 0.3, 16000),
    ("Synthé Bouchon", 36, "Moog", 0.6, 0.6, 0.0, 0.0, 800),
    ("Synthé Velours", 33, "Moog", 0.3, 2.5, 0.0, 0.5, 1000),
    ("Fretless Imaginaire", 38, "Pluck", 0.0, 3.5, 0.0, 0.1, 6000),
    ("Synthé Mordant", 36, "Moog", 0.8, 0.7, 0.35, 0.2, 650),
    ("Basse Studio", 40, "Pluck", 0.0, 2.2, 0.05, 0.25, 11000),
]
for i, (n, r, m, pu, d, dr, h, t) in enumerate(basses):
    extra = {"reso": 0.35} if m == "Moog" else {}
    presets.append(p(n, S5, m, r, outfit=(i + 1) % 3, punch=pu, decay=d, drive=dr, harm=h,
                     tone=float(t), release=120.0, glide=60.0, output=(2.0 if m == "Moog" else -2.0), **extra))

assert len(presets) == 95, len(presets)
assert len({x["name"] for x in presets}) == len(presets), "noms en double"

out = pathlib.Path(__file__).resolve().parent.parent / "presets" / "factory.json"
out.write_text(json.dumps({"version": 1, "presets": presets}, ensure_ascii=False, indent=1), encoding="utf-8")
print(f"{len(presets)} presets -> {out}")

# Copie C++ pour le test du moteur (tests/EngineTest.cpp, sans JUCE)
inc = pathlib.Path(__file__).resolve().parent.parent / "tests" / "factory_presets.inc"
lines = ["// Généré par tools/generate_factory_presets.py — ne pas modifier à la main"]
for x in presets:
    q = x["params"]
    lines.append('{ "%s", %d, %d, %.3ff, %.3ff, %d, %.3ff, %.3ff, %.3ff, %d, %.3ff, %.3ff, %.3ff, %.3ff, %.3ff, %.3ff },' % (
        x["name"].replace('"', ''), x["root"], q["model"], q["tune"], q["glide"], q["glideMode"], q["punch"],
        q["decay"], q["drive"], q["driveType"], q["output"], q["harm"], q["tone"], q["reso"], q["attack"], q["release"]))
inc.write_text("\n".join(lines) + "\n", encoding="utf-8")
