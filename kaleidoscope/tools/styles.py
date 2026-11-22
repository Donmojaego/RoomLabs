#!/usr/bin/env python3
"""Kaleidoscope — bibliothèque de styles et d'humeurs (source unique).
Écrit source/gen/Styles.inc (moteur C++) et ui/styles.json (interface).
Chaque style = une grammaire : tempo, gammes préférées, progressions, rythmes, swing, registre, densité,
façon d'accompagner. Les noms d'artistes du cahier des charges ne servent que de références d'ambiance."""
import json, pathlib

SCALES = [  # id, nom, intervalles
    ("major", "Majeur", [0, 2, 4, 5, 7, 9, 11]), ("minor", "Mineur naturel", [0, 2, 3, 5, 7, 8, 10]),
    ("harmonic", "Mineur harmonique", [0, 2, 3, 5, 7, 8, 11]), ("melodic", "Mineur mélodique", [0, 2, 3, 5, 7, 9, 11]),
    ("dorian", "Dorien", [0, 2, 3, 5, 7, 9, 10]), ("phrygian", "Phrygien", [0, 1, 3, 5, 7, 8, 10]),
    ("lydian", "Lydien", [0, 2, 4, 6, 7, 9, 11]), ("mixolydian", "Mixolydien", [0, 2, 4, 5, 7, 9, 10]),
    ("locrian", "Locrien", [0, 1, 3, 5, 6, 8, 10]), ("minpenta", "Pentatonique mineure", [0, 3, 5, 7, 10]),
    ("majpenta", "Pentatonique majeure", [0, 2, 4, 7, 9]), ("blues", "Blues", [0, 3, 5, 6, 7, 10]),
    ("phrygdom", "Phrygien dominant", [0, 1, 4, 5, 7, 8, 10]), ("double", "Double harmonique", [0, 1, 4, 5, 7, 8, 11]),
    ("hirajoshi", "Hirajoshi", [0, 2, 3, 7, 8]), ("insen", "In-sen", [0, 1, 5, 7, 10]),
    ("bhairav", "Raga Bhairav", [0, 1, 4, 5, 7, 8, 11]), ("kafi", "Raga Kafi", [0, 2, 3, 5, 7, 9, 10]),
    ("wholetone", "Par tons", [0, 2, 4, 6, 8, 10]), ("hungarian", "Mineur hongrois", [0, 2, 3, 6, 7, 8, 11]),
]
MOODS = [  # id, nom, gammes favorisées (+), densité, registre, tension, legato, énergie, tempo
    ("triste", "Triste", ["minor", "harmonic", "dorian", "minpenta"], 0.75, -3, 0.55, 0.75, 0.3, 0.9),
    ("joyeuse", "Joyeuse", ["major", "mixolydian", "majpenta", "lydian"], 1.1, 3, 0.35, 0.45, 0.75, 1.05),
    ("planante", "Planante", ["lydian", "dorian", "major", "majpenta"], 0.6, 2, 0.6, 0.9, 0.25, 0.92),
    ("sombre", "Sombre", ["phrygian", "harmonic", "minor", "locrian", "hungarian"], 0.85, -6, 0.65, 0.6, 0.5, 0.95),
    ("romantique", "Romantique", ["major", "minor", "dorian", "melodic"], 0.85, 0, 0.7, 0.8, 0.4, 0.95),
    ("energique", "Énergique", ["minor", "minpenta", "mixolydian", "dorian"], 1.3, 2, 0.4, 0.35, 0.95, 1.08),
    ("epique", "Épique", ["minor", "harmonic", "dorian", "major"], 1.0, 0, 0.6, 0.6, 0.85, 1.0),
    ("mysterieuse", "Mystérieuse", ["phrygian", "hirajoshi", "wholetone", "double", "dorian"], 0.75, 0, 0.75, 0.6, 0.45, 0.95),
    ("luxueuse", "Luxueuse", ["dorian", "minor", "melodic", "major"], 0.9, 0, 0.8, 0.7, 0.55, 0.97),
    ("nostalgique", "Nostalgique", ["major", "dorian", "mixolydian", "minor"], 0.85, 0, 0.6, 0.7, 0.4, 0.95),
]
RHYTHMS = {  # poids par double-croche (16 positions) ; 1 = appui fort
    "fourfloor": [1, 0, .3, 0, .8, 0, .3, 0, .9, 0, .3, 0, .8, 0, .3, .1],
    "straight8": [1, .05, .6, .05, .9, .05, .6, .05, .95, .05, .6, .05, .9, .05, .6, .1],
    "busy16": [1, .5, .7, .5, .9, .5, .7, .5, .95, .5, .7, .5, .9, .5, .7, .5],
    "sparse": [1, 0, 0, .2, .3, 0, .6, 0, .4, 0, .5, 0, .3, 0, .2, 0],
    "trap": [1, .1, .3, .5, .2, .1, .7, .2, .4, .3, .6, .1, .3, .5, .4, .2],
    "tresillo": [1, 0, .1, .9, 0, .1, .9, 0, .6, 0, .2, .8, 0, .2, .7, 0],
    "dembow": [1, 0, .1, .9, .8, 0, .2, .7, .9, 0, .1, .9, .8, 0, .2, .7],
    "offbeat": [.3, 0, 1, 0, .2, 0, 1, 0, .3, 0, 1, 0, .2, 0, 1, .1],
    "afro": [1, 0, .3, .8, .1, .2, .9, .6, .2, 0, .8, .2, .7, .1, .6, .3],
    "amapiano": [.9, .1, .2, .8, .1, .2, .9, .1, .3, .2, .8, .1, .2, .9, .3, .2],
    "clave": [1, 0, .1, .9, 0, .1, .8, 0, .2, 0, .9, 0, .9, 0, .2, 0],
    "garage": [1, .1, .2, .7, .3, .1, .8, .3, .5, .2, .7, .4, .3, .2, .8, .3],
    "ballad": [1, 0, .2, 0, .5, 0, .3, 0, .8, 0, .2, 0, .5, 0, .4, 0],
    "rock": [1, 0, .5, 0, 1, 0, .5, 0, 1, 0, .5, 0, 1, 0, .5, .2],
    "classical": [1, 0, .4, 0, .7, 0, .4, 0, .9, 0, .4, 0, .7, 0, .4, 0],
    "kompa": [1, 0, .2, .8, 0, .2, .8, 0, .9, 0, .3, .8, .2, .2, .7, 0],
    "sebene": [.8, .5, .7, .5, .8, .5, .7, .5, .8, .5, .7, .5, .8, .5, .7, .5],
}
ACCOMP = ["pad", "stabs", "offbeat", "arp", "broken", "comp", "pump", "picking", "pulse8", "rumba", "strum", "log"]

def S(name, family, tempo, scales, progs, rhythm, accomp, density=0.55, swing=0.0, triplet=0.0, reg=0, leap=0.25,
      rest=0.25, legato=0.5, ext=0.3, repeat=0.55, orn=0.1, chords_per_bar=1, energy=0.5, palette=0):
    return dict(name=name, family=family, tmin=tempo[0], tmax=tempo[1], scales=scales, progs=progs, rhythm=rhythm, accomp=accomp,
                density=density, swing=swing, triplet=triplet, reg=reg, leap=leap, rest=rest, legato=legato, ext=ext, repeat=repeat,
                orn=orn, cpb=chords_per_bar, energy=energy, palette=palette)

MIN = [[1, 6, 3, 7], [1, 4, 5, 1], [1, 6, 7, 1], [1, 1, 6, 7], [1, 7, 6, 7], [6, 7, 1, 1], [1, 4, 6, 5]]
MAJ = [[1, 5, 6, 4], [6, 4, 1, 5], [1, 6, 4, 5], [1, 4, 6, 5], [4, 1, 5, 6], [1, 3, 4, 4]]
RNB = [[2, 5, 1, 6], [4, 3, 2, 1], [1, 6, 2, 5], [4, 5, 3, 6], [2, 3, 4, 5], [1, 7, 6, 5]]
AFRO = [[1, 4, 5, 4], [1, 5, 6, 4], [6, 4, 1, 5], [1, 4, 1, 5], [2, 5, 1, 1]]
CARIB = [[1, 4, 5, 4], [1, 5, 4, 5], [6, 4, 5, 1], [2, 5, 1, 6], [1, 6, 2, 5]]
JAZZ = [[2, 5, 1, 6], [3, 6, 2, 5], [1, 6, 2, 5], [4, 7, 3, 6]]
H, R, A, C, L, E, P, J, W = "Hip-hop & Trap", "R&B & Soul", "Afro", "Caraïbes", "Latino", "Club & électro", "Pop & Rock", "Jazz, classique & ambiances", "Monde"
STYLES = [
    ("trap", S("Trap", H, (130, 160), ["minor", "harmonic", "phrygian", "minpenta"], MIN, "trap", "pad", .45, 0, .2, -2, .3, .4, .5, .2, .65, .15, 1, .7, 1)),
    ("rap", S("Rap", H, (82, 98), ["minor", "dorian", "minpenta"], MIN, "sparse", "stabs", .45, .12, .05, -1, .25, .35, .45, .35, .6, .1, 1, .55, 2)),
    ("drill", S("Drill", H, (138, 146), ["harmonic", "phrygian", "minor"], MIN, "trap", "pad", .5, 0, .35, -1, .35, .35, .4, .2, .6, .2, 1, .8, 1)),
    ("afrotrap", S("Afrotrap", H, (140, 150), ["minor", "dorian", "minpenta"], MIN + AFRO, "afro", "broken", .6, .08, .1, 0, .3, .3, .4, .25, .6, .15, 1, .75, 3)),
    ("detroit", S("Detroit", H, (90, 104), ["minor", "harmonic", "phrygian"], MIN, "trap", "stabs", .5, .1, .15, -2, .35, .35, .35, .25, .6, .1, 1, .7, 1)),
    ("clubnl", S("Club hollandais", H, (96, 108), ["minor", "dorian", "mixolydian"], MIN + MAJ, "dembow", "stabs", .6, .08, .05, 1, .3, .3, .4, .3, .6, .15, 1, .75, 4)),
    ("luxe", S("Luxe", H, (70, 86), ["minor", "melodic", "harmonic", "dorian"], MIN + RNB, "ballad", "pad", .45, .1, .05, 0, .3, .35, .75, .75, .5, .2, 1, .55, 5)),
    ("intello", S("Intello", H, (90, 110), ["dorian", "mixolydian", "lydian", "majpenta"], RNB + JAZZ, "garage", "comp", .55, .18, .1, 2, .35, .3, .45, .8, .55, .2, 2, .6, 6)),
    ("emo", S("Emo", H, (130, 160), ["minor", "major", "dorian"], MIN + MAJ, "straight8", "strum", .55, 0, .05, 2, .25, .3, .55, .3, .6, .05, 1, .6, 7)),
    ("rnb", S("R&B", R, (65, 92), ["dorian", "minor", "major", "melodic"], RNB, "ballad", "pad", .5, .15, .15, 1, .3, .3, .7, .8, .5, .3, 1, .45, 8)),
    ("soul", S("Soul", R, (70, 100), ["major", "dorian", "mixolydian", "minpenta"], RNB + MAJ, "ballad", "comp", .55, .18, .1, 0, .3, .3, .65, .65, .55, .3, 1, .5, 9)),
    ("neosoul", S("Neo soul", R, (70, 92), ["dorian", "minor", "melodic", "lydian"], RNB + JAZZ, "garage", "comp", .55, .22, .2, 1, .35, .3, .6, .9, .5, .35, 2, .5, 8)),
    ("gospel", S("Gospel", R, (70, 120), ["major", "mixolydian", "majpenta", "blues"], RNB + MAJ, "ballad", "comp", .6, .15, .2, 0, .3, .25, .65, .85, .5, .35, 2, .65, 9)),
    ("afrolove", S("Afro love", A, (95, 112), ["major", "majpenta", "mixolydian", "dorian"], AFRO + MAJ, "afro", "broken", .55, .08, .05, 1, .25, .3, .55, .45, .6, .2, 1, .55, 10)),
    ("afrobeat", S("Afrobeat", A, (100, 120), ["dorian", "mixolydian", "minpenta", "major"], AFRO, "afro", "picking", .65, .1, .05, 1, .3, .25, .4, .4, .65, .2, 1, .7, 10)),
    ("amapiano", S("Amapiano", A, (110, 116), ["dorian", "minor", "major", "melodic"], RNB + AFRO, "amapiano", "log", .55, .12, .05, 0, .3, .3, .55, .7, .6, .15, 1, .6, 11)),
    ("afrorumba", S("Afro rumba", A, (110, 130), ["major", "majpenta", "mixolydian"], AFRO + CARIB, "rumba" if False else "clave", "picking", .65, .05, .05, 2, .3, .25, .45, .4, .6, .2, 1, .65, 12)),
    ("rumbacongo", S("Rumba congolaise", A, (100, 125), ["major", "mixolydian", "majpenta"], AFRO + MAJ, "clave", "picking", .65, .05, .05, 2, .25, .25, .45, .35, .6, .2, 1, .6, 12)),
    ("sebene", S("Sébène", A, (125, 145), ["major", "majpenta", "mixolydian"], [[1, 4, 5, 4], [1, 5, 1, 4]], "sebene", "picking", .8, 0, .05, 3, .3, .15, .3, .3, .7, .25, 1, .9, 12)),
    ("coupedecale", S("Coupé-décalé", A, (125, 135), ["major", "mixolydian", "minor"], AFRO, "afro", "stabs", .7, 0, .05, 2, .3, .2, .35, .3, .65, .15, 1, .9, 13)),
    ("zouk", S("Zouk", C, (85, 110), ["major", "minor", "dorian"], CARIB + RNB, "kompa", "broken", .55, .05, .05, 1, .25, .3, .6, .55, .55, .2, 1, .55, 14)),
    ("kompa", S("Kompa", C, (95, 120), ["major", "mixolydian", "minor"], CARIB, "kompa", "picking", .6, .05, .05, 1, .25, .3, .5, .45, .6, .2, 1, .6, 14)),
    ("reggae", S("Reggae", C, (68, 82), ["major", "minor", "dorian", "mixolydian"], CARIB, "offbeat", "offbeat", .5, .15, .1, 0, .25, .35, .45, .3, .6, .15, 1, .45, 15)),
    ("ragga", S("Ragga", C, (88, 100), ["minor", "phrygian", "minpenta"], MIN + CARIB, "offbeat", "stabs", .6, .1, .1, 0, .3, .3, .35, .25, .6, .15, 1, .7, 15)),
    ("dancehall", S("Dancehall", C, (90, 105), ["minor", "dorian", "phrygian"], MIN + CARIB, "dembow", "stabs", .55, .08, .05, 0, .3, .3, .4, .3, .6, .15, 1, .7, 16)),
    ("shatta", S("Shatta", C, (95, 108), ["minor", "phrygian", "harmonic"], MIN, "dembow", "pad", .55, .05, .1, -1, .35, .3, .45, .25, .6, .2, 1, .75, 16)),
    ("bouyon", S("Bouyon", C, (150, 165), ["major", "mixolydian", "minor"], CARIB + AFRO, "busy16", "pulse8", .75, 0, 0, 2, .3, .2, .3, .25, .7, .15, 1, .95, 17)),
    ("bombaton", S("Bombaton", C, (100, 110), ["minor", "dorian"], MIN + CARIB, "dembow", "stabs", .6, .05, .05, 0, .3, .3, .4, .3, .6, .15, 1, .8, 16)),
    ("reggaeton", S("Reggaeton", L, (88, 100), ["minor", "dorian", "harmonic"], MIN + CARIB, "dembow", "stabs", .55, 0, .05, 0, .3, .3, .45, .35, .6, .15, 1, .7, 18)),
    ("bailefunk", S("Baile funk", L, (125, 135), ["minor", "phrygian", "minpenta"], MIN, "tresillo", "stabs", .6, 0, .05, 0, .3, .3, .35, .2, .65, .15, 1, .9, 18)),
    ("latino", S("Latino", L, (95, 125), ["major", "minor", "harmonic", "phrygdom"], CARIB + MAJ, "clave", "picking", .6, .05, .1, 1, .3, .25, .45, .4, .6, .25, 1, .7, 19)),
    ("bossa", S("Bossa nova", L, (120, 140), ["major", "dorian", "lydian"], JAZZ + RNB, "clave", "comp", .55, .05, .05, 1, .3, .25, .55, .9, .55, .2, 2, .45, 19)),
    ("ukg", S("UK garage", E, (130, 136), ["minor", "dorian"], RNB + MIN, "garage", "stabs", .6, .2, .05, 1, .3, .3, .4, .7, .6, .15, 1, .75, 20)),
    ("twostep", S("2-step", E, (128, 136), ["minor", "dorian", "melodic"], RNB, "garage", "comp", .55, .25, .05, 1, .3, .3, .45, .75, .6, .15, 1, .7, 20)),
    ("electro", S("Electro", E, (120, 130), ["minor", "phrygian", "dorian"], MIN, "straight8", "arp", .65, 0, 0, 0, .35, .25, .3, .2, .7, .1, 1, .8, 21)),
    ("electrohouse", S("Electro house", E, (124, 130), ["minor", "dorian", "major"], MIN + MAJ, "fourfloor", "pump", .6, 0, 0, 1, .3, .25, .35, .3, .7, .1, 1, .85, 21)),
    ("techno", S("Techno", E, (128, 140), ["minor", "phrygian", "locrian"], [[1, 1, 1, 1], [1, 1, 7, 7], [1, 6, 1, 6]], "busy16", "arp", .6, 0, 0, -1, .2, .3, .3, .15, .8, .05, 1, .85, 22)),
    ("minimal", S("Minimal", E, (120, 128), ["minor", "dorian", "phrygian"], [[1, 1, 1, 1], [1, 1, 4, 4]], "sparse", "pulse8", .35, .05, 0, 0, .2, .5, .3, .2, .8, .05, 1, .5, 22)),
    ("dubstep", S("Dubstep", E, (138, 142), ["minor", "phrygian", "harmonic"], MIN, "sparse", "pad", .45, 0, .15, -3, .35, .4, .4, .2, .6, .1, 1, .85, 23)),
    ("club", S("Club", E, (118, 128), ["minor", "dorian", "major"], MIN + MAJ, "fourfloor", "pump", .6, .05, 0, 1, .3, .25, .4, .3, .65, .1, 1, .8, 21)),
    ("clubenergy", S("Énergie club", E, (126, 132), ["minor", "dorian"], MIN, "fourfloor", "arp", .7, 0, 0, 1, .3, .2, .3, .25, .7, .1, 1, .95, 23)),
    ("pop", S("Pop", P, (95, 125), ["major", "minor", "mixolydian"], MAJ + MIN, "straight8", "pulse8", .55, 0, .05, 1, .25, .3, .5, .3, .6, .1, 1, .6, 24)),
    ("poprock", S("Pop rock", P, (110, 140), ["major", "mixolydian", "minor"], MAJ, "rock", "strum", .6, 0, 0, 1, .25, .25, .45, .25, .6, .1, 1, .7, 25)),
    ("rock", S("Rock", P, (110, 150), ["minpenta", "mixolydian", "minor", "blues"], MAJ + MIN, "rock", "strum", .6, .05, .05, 0, .3, .25, .4, .15, .6, .1, 1, .8, 25)),
    ("alternative", S("Alternative", P, (90, 140), ["minor", "dorian", "lydian", "mixolydian"], MIN + MAJ, "straight8", "picking", .55, 0, .05, 1, .35, .3, .5, .35, .55, .1, 1, .6, 26)),
    ("variete", S("Variété française", P, (85, 120), ["major", "minor", "harmonic"], MAJ + MIN, "ballad", "broken", .55, .05, .1, 1, .3, .3, .55, .4, .55, .15, 1, .5, 26)),
    ("seventies", S("Années 70", P, (95, 118), ["dorian", "mixolydian", "major", "minpenta"], RNB + MAJ, "garage", "comp", .6, .15, .05, 1, .3, .3, .45, .55, .6, .15, 1, .65, 27)),
    ("eighties", S("Années 80", P, (110, 125), ["major", "minor", "lydian"], MAJ + MIN, "straight8", "arp", .55, 0, 0, 1, .3, .3, .45, .35, .65, .1, 1, .7, 28)),
    ("jazz", S("Jazz", J, (90, 160), ["dorian", "mixolydian", "melodic", "blues", "lydian"], JAZZ, "garage", "comp", .6, .35, .25, 1, .4, .25, .5, 1.0, .45, .35, 2, .55, 29)),
    ("classique", S("Musique classique", J, (60, 120), ["major", "minor", "harmonic", "melodic"], MAJ + MIN, "classical", "broken", .6, 0, .1, 1, .3, .2, .6, .35, .55, .3, 1, .5, 30)),
    ("lofi", S("Lofi", J, (70, 90), ["dorian", "major", "minor", "lydian"], RNB + JAZZ, "ballad", "comp", .45, .2, .1, 0, .3, .35, .6, .85, .55, .2, 1, .35, 31)),
    ("planante", S("Planante", J, (70, 100), ["lydian", "dorian", "majpenta", "major"], MAJ + RNB, "sparse", "pad", .35, 0, .05, 2, .3, .45, .85, .6, .55, .15, 1, .3, 32)),
    ("triste", S("Musique triste", J, (60, 85), ["minor", "harmonic", "dorian"], MIN, "ballad", "pad", .4, 0, .05, 0, .25, .4, .8, .45, .55, .2, 1, .3, 33)),
    ("joyeuse", S("Musique joyeuse", J, (110, 135), ["major", "majpenta", "mixolydian"], MAJ, "straight8", "pulse8", .65, .05, .05, 2, .3, .2, .45, .3, .6, .15, 1, .75, 34)),
    ("oriental", S("Musique orientale", W, (90, 115), ["phrygdom", "double", "harmonic", "hungarian"], [[1, 2, 1, 2], [1, 7, 6, 5], [1, 1, 2, 1]], "tresillo", "picking", .6, .05, .1, 1, .2, .25, .5, .2, .6, .45, 1, .6, 35)),
    ("asiatique", S("Musique asiatique", W, (70, 110), ["hirajoshi", "insen", "majpenta", "minpenta"], [[1, 1, 4, 5], [1, 6, 4, 1], [6, 1, 6, 5]], "sparse", "arp", .45, 0, .05, 2, .35, .35, .6, .2, .55, .3, 1, .4, 36)),
    ("hindoue", S("Musique hindoue", W, (80, 120), ["bhairav", "kafi", "phrygdom"], [[1, 1, 1, 1], [1, 2, 1, 7], [1, 4, 1, 5]], "tresillo", "pad", .6, .05, .2, 1, .2, .25, .55, .15, .6, .5, 1, .6, 37)),
    ("world", S("World", W, (90, 120), ["dorian", "majpenta", "mixolydian", "phrygdom"], AFRO + CARIB, "afro", "picking", .55, .05, .1, 1, .3, .25, .5, .3, .6, .25, 1, .6, 38)),
    ("guerre", S("Musique de guerre", W, (80, 110), ["harmonic", "minor", "phrygian"], [[1, 6, 7, 1], [1, 1, 6, 5], [1, 7, 6, 5]], "rock", "pulse8", .6, 0, .2, -1, .3, .25, .45, .25, .65, .15, 1, .9, 39)),
]
def main():
    root = pathlib.Path(__file__).resolve().parent.parent
    sid = {s[0]: i for i, s in enumerate(SCALES)}; rid = {k: i for i, k in enumerate(RHYTHMS)}
    fams = []
    for _, s in STYLES:
        if s["family"] not in fams: fams.append(s["family"])
    L = ["// Généré par tools/styles.py — ne pas modifier à la main (inclus dans namespace kal::data)"]
    L.append("static const ScaleDef kScales[] = {")
    for k, n, iv in SCALES: L.append('  { "%s", %d, { %s } },' % (k, len(iv), ", ".join(map(str, iv + [0] * (8 - len(iv))))))
    L.append("};\nstatic const int kNumScales = %d;" % len(SCALES))
    L.append("static const float kRhythms[][16] = {")
    for k, w in RHYTHMS.items(): L.append("  { %s }," % ", ".join("%.2ff" % v for v in w))
    L.append("};")
    L.append("static const MoodDef kMoods[] = {")
    for m in MOODS:
        sc = [sid[x] for x in m[2]] + [-1] * (5 - len(m[2]))
        L.append('  { "%s", { %s }, %.2ff, %d, %.2ff, %.2ff, %.2ff, %.2ff },' % (m[0], ", ".join(map(str, sc)), m[3], m[4], m[5], m[6], m[7], m[8]))
    L.append("};\nstatic const int kNumMoods = %d;" % len(MOODS))
    L.append("static const StyleDef kStyles[] = {")
    for k, s in STYLES:
        sc = [sid[x] for x in s["scales"]][:6]; sc += [-1] * (6 - len(sc))
        pr = s["progs"][:8]; prs = ", ".join("{ %s }" % ", ".join(map(str, p)) for p in pr) + "".join(", { 0, 0, 0, 0 }" for _ in range(8 - len(pr)))
        L.append('  { "%s", %d, %d, %d, { %s }, %d, { %s }, %d, %d, %.2ff, %.2ff, %.2ff, %d, %.2ff, %.2ff, %.2ff, %.2ff, %.2ff, %.2ff, %d, %.2ff, %d },' % (
            k, fams.index(s["family"]), s["tmin"], s["tmax"], ", ".join(map(str, sc)), len(pr), prs, rid[s["rhythm"]], ACCOMP.index(s["accomp"]),
            s["density"], s["swing"], s["triplet"], s["reg"], s["leap"], s["rest"], s["legato"], s["ext"], s["repeat"], s["orn"], s["cpb"], s["energy"], s["palette"]))
    L.append("};\nstatic const int kNumStyles = %d;" % len(STYLES))
    (root / "source" / "gen").mkdir(parents=True, exist_ok=True)
    (root / "source" / "gen" / "Styles.inc").write_text("\n".join(L) + "\n", encoding="utf-8")
    ui = {"families": fams, "styles": [{"id": k, "name": s["name"], "family": fams.index(s["family"]), "tempo": [s["tmin"], s["tmax"]], "energy": s["energy"], "palette": s["palette"]} for k, s in STYLES],
          "moods": [{"id": m[0], "name": m[1]} for m in MOODS], "scales": [{"id": k, "name": n} for k, n, _ in SCALES]}
    N = ["// Généré par tools/styles.py — noms affichés (paramètres Style et Humeur), même ordre que Styles.inc"]
    N.append("inline const std::vector<const char*>& styleNames() { static const std::vector<const char*> n { %s }; return n; }" % ", ".join('"%s"' % s["name"] for _, s in STYLES))
    N.append("inline const std::vector<const char*>& moodNames() { static const std::vector<const char*> n { %s }; return n; }" % ", ".join('"%s"' % m[1] for m in MOODS))
    (root / "source" / "gen" / "Names.inc").write_text("\n".join(N) + "\n", encoding="utf-8")
    (root / "ui").mkdir(exist_ok=True)
    (root / "ui" / "styles.json").write_text(json.dumps(ui, ensure_ascii=False, indent=1), encoding="utf-8")
    print(len(STYLES), "styles,", len(fams), "familles,", len(MOODS), "humeurs,", len(SCALES), "gammes")
if __name__ == "__main__": main()
