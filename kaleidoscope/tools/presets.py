#!/usr/bin/env python3
"""Kaleidoscope — génère les 150 presets natifs (source de vérité).

Sorties :
  ui/presets/factory.json   (chargé par le plugin via BinaryData)
  tests/presets.inc         (utilisé par les tests moteur et la calibration)
Les niveaux viennent de tools/levels.json (écrit par tests/calibrate_levels.cpp) : chaque preset
est ramené à environ -16 dB RMS / -6 dBFS crête sur la même phrase de test.
Chaque mise à jour ajoute 5 presets : les ajouter à la fin des listes UPDATES (version, catégorie, nom, réglages).
"""
import json, os, random, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import banks  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
T = {n: i for i, n in enumerate(["Analogique", "Impulsion", "Formants", "Verre", "Orgue", "Numérique", "Chœur", "Spectral"])}

BASE = dict(wtOn=True, tableA=0, tableB=1, posA=0.3, posB=0.5, mixB=0.4, semiB=0, fineB=7, unison=2, detune=0.2, spread=0.6,
            sub=0.2, noise=0.0, wtEnv=0.0, wtLfo=0.0, fMode=0, f24=False, cutoff=0.7, reso=0.2, fEnv=0.3, keytrack=0.5, drive=0.1,
            aA=5, aD=600, aS=0.7, aR=400, fA=2, fD=500, fS=0.3, fR=400, lfoRate=4.0, lfoVib=0.0, lfoCut=0.0, lfoAmp=0.0,
            granOn=False, granSize=80, granDensity=30, granSpray=0.3, granPitch=0.0, granPos=0.3, granMix=0.5,
            smpOn=False, smpLevel=0.8, smpTune=0.0, mono=False, glideMs=0, bendRange=2,
            fxDrive=0.0, chorus=0.0, delay=0.0, delayFb=0.35, delayDiv=2, reverb=0.2, revSize=0.6, width=0.8, tilt=0.0, levelDb=-6.0)

# --- catégorie : (gabarit, plages de variation, style conseillé, humeur conseillée, noms)
CATS = [
 ("Keys", dict(tableA=T["Analogique"], posA=0.15, tableB=T["Verre"], posB=0.3, mixB=0.3, unison=1, sub=0.1, cutoff=0.55, fEnv=0.35,
               aA=2, aD=1400, aS=0.25, aR=500, fD=700, fS=0.15, chorus=0.25, reverb=0.25),
  dict(posA=(0.0, 0.45), mixB=(0.1, 0.5), cutoff=(0.4, 0.7), aD=(700, 2400), chorus=(0.0, 0.45), tableB=[T["Verre"], T["Orgue"], T["Numérique"]]),
  ["neosoul", "lofi", "rnb", "jazz", "soul"], ["romantique", "nostalgique", "planante"],
  ["Rhodes de Minuit", "Velours Électrique", "Tine Doré", "Salon Bleu", "Wurli Poussière", "Clavier de Pluie", "Café Crème", "Satin 73",
   "Marteau Feutré", "Studio Seventh", "Néon Tiède", "Mallette Vintage", "Touches Fumées", "Ballade Lente", "Porte Entrouverte",
   "Ivoire Chaud", "Lumière d'Ambre", "Dimanche Soir", "Électrique Doux", "Piano Kaléido"]),
 ("Pads", dict(tableA=T["Chœur"], posA=0.4, tableB=T["Spectral"], posB=0.5, mixB=0.45, unison=4, detune=0.3, spread=0.9, sub=0.05,
               cutoff=0.5, fEnv=0.1, aA=900, aD=2000, aS=0.85, aR=2200, wtLfo=0.25, lfoRate=0.15, chorus=0.35, reverb=0.55, revSize=0.85),
  dict(posA=(0.1, 0.9), posB=(0.1, 0.9), cutoff=(0.35, 0.7), aA=(400, 2500), wtLfo=(0.1, 0.5), tableA=[T["Chœur"], T["Formants"], T["Analogique"], T["Spectral"]]),
  ["planante", "lofi", "classique", "triste"], ["planante", "mysterieuse", "triste"],
  ["Nuage de Verre", "Aube Lente", "Cathédrale Douce", "Brume Violette", "Horizon Infini", "Velours Cosmique", "Mer Intérieure",
   "Lumière Boréale", "Silence Habité", "Planète Lointaine", "Soie Prismatique", "Sommeil Paradoxal", "Halo", "Nébuleuse",
   "Champ de Lavande", "Respiration", "Givre Chaud", "Kaléido Nuit"]),
 ("Leads", dict(tableA=T["Analogique"], posA=0.7, tableB=T["Impulsion"], posB=0.4, mixB=0.4, unison=2, detune=0.12, sub=0.1,
                cutoff=0.68, reso=0.3, fEnv=0.4, aA=6, aD=500, aS=0.8, aR=250, mono=True, glideMs=45, lfoVib=0.25, lfoRate=5.5,
                delay=0.22, delayDiv=2, reverb=0.25, drive=0.25),
  dict(posA=(0.5, 1.0), cutoff=(0.55, 0.8), reso=(0.1, 0.5), glideMs=(0, 110), lfoVib=(0.0, 0.35), tableB=[T["Impulsion"], T["Numérique"], T["Formants"]]),
  ["eighties", "trap", "electro", "afrobeat", "pop"], ["energique", "nostalgique", "epique"],
  ["Laser Doré", "Siffleur Analogique", "Fil d'Argent", "Sirène Néon", "Comète", "Lead 1987", "Flûte Électrique", "Ruban Lumineux",
   "Chant Carré", "Mirage", "Voix de Pixel", "Saphir", "Éclair", "Trompette Synthé", "Lame Douce", "Rêve Lucide", "Prisme Lead",
   "Oiseau Mécanique", "Saturne", "Kaléido Solo"]),
 ("Plucks", dict(tableA=T["Analogique"], posA=0.6, tableB=T["Verre"], posB=0.5, mixB=0.3, unison=2, detune=0.15, sub=0.05, cutoff=0.35,
                 reso=0.25, fEnv=0.6, fA=1, fD=220, fS=0.0, aA=1, aD=420, aS=0.0, aR=300, delay=0.28, delayDiv=1, reverb=0.3),
  dict(posA=(0.3, 0.9), fEnv=(0.4, 0.8), fD=(120, 400), aD=(250, 900), delay=(0.1, 0.4), tableA=[T["Analogique"], T["Impulsion"], T["Numérique"]]),
  ["amapiano", "afrobeat", "ukg", "pop", "zouk"], ["joyeuse", "energique", "luxueuse"],
  ["Goutte de Lumière", "Pizz Prisme", "Marimba Synthé", "Kalimba de Verre", "Pluie de Néon", "Harpe Pixel", "Pincement Doré",
   "Bulle", "Cristal Court", "Guitare de Rêve", "Koto Moderne", "Arpège Clair", "Corde Pincée", "Gouttelette", "Tropical",
   "Perle", "Zéphyr", "Plume", "Écaille", "Kaléido Pluck"]),
 ("Bells", dict(tableA=T["Verre"], posA=0.5, tableB=T["Numérique"], posB=0.35, mixB=0.5, semiB=12, fineB=0, unison=1, sub=0.0,
                cutoff=0.8, fEnv=0.15, aA=1, aD=2600, aS=0.0, aR=1800, reverb=0.45, revSize=0.75, delay=0.15),
  dict(posA=(0.2, 1.0), posB=(0.1, 0.8), semiB=[7, 12, 19, 24], aD=(1500, 4000), reverb=(0.3, 0.6)),
  ["asiatique", "planante", "joyeuse", "lofi"], ["planante", "mysterieuse", "joyeuse"],
  ["Cloche de Verre", "Carillon d'Hiver", "Clochette Bleue", "Boîte à Musique", "Glockenspiel", "Cloche Lunaire", "Timbre de Cristal",
   "Gamelan Doux", "Sanza Céleste", "Tintement", "Étoile Filante", "Celesta", "Bol Chantant", "Clochettes de Vent", "Phare", "Kaléido Cloche"]),
 ("Bass", dict(tableA=T["Analogique"], posA=0.65, tableB=T["Impulsion"], posB=0.5, mixB=0.25, semiB=-12, fineB=0, unison=1, sub=0.5,
               cutoff=0.38, reso=0.2, fEnv=0.35, fD=260, fS=0.1, aA=2, aD=500, aS=0.75, aR=120, mono=True, glideMs=20, drive=0.35, reverb=0.05, width=0.2),
  dict(posA=(0.3, 0.9), cutoff=(0.25, 0.5), reso=(0.05, 0.45), sub=(0.3, 0.7), drive=(0.1, 0.6), glideMs=(0, 60)),
  ["trap", "drill", "dancehall", "techno", "reggaeton"], ["sombre", "energique"],
  ["Basse Profonde", "Sub Velours", "Moog Imaginaire", "Rebond", "Grondement", "Acide Doux", "Basse Ronde", "Tapis Grave",
   "Synthé 303 Tiède", "Fondation", "Basse Funk", "Pulse Grave", "Marée Basse", "Graviton", "Cuir", "Kaléido Basse"]),
 ("Strings", dict(tableA=T["Analogique"], posA=0.68, tableB=T["Chœur"], posB=0.3, mixB=0.3, unison=4, detune=0.22, spread=0.85, sub=0.0,
                  cutoff=0.55, fEnv=0.1, aA=320, aD=1500, aS=0.85, aR=900, lfoVib=0.1, lfoRate=5, chorus=0.4, reverb=0.45, revSize=0.8),
  dict(posA=(0.5, 0.8), cutoff=(0.4, 0.65), aA=(120, 700), chorus=(0.2, 0.6)),
  ["classique", "guerre", "triste", "gospel"], ["epique", "triste", "romantique"],
  ["Ensemble Soyeux", "Cordes de 1979", "Violons Synthé", "Quatuor Rêvé", "Orchestre de Poche", "Archet Lent", "Solina Douce",
   "Cordes d'Or", "Lyrique", "Sérénade", "Grand Écran", "Kaléido Cordes"]),
 ("Textures", dict(tableA=T["Spectral"], posA=0.5, tableB=T["Formants"], posB=0.5, mixB=0.4, unison=3, detune=0.35, spread=1.0,
                   noise=0.1, cutoff=0.6, aA=1200, aD=3000, aS=0.8, aR=3000, wtLfo=0.4, lfoRate=0.1, granOn=True, granSize=120,
                   granDensity=28, granSpray=0.45, granMix=0.7, reverb=0.6, revSize=0.9, delay=0.2),
  dict(posA=(0.0, 1.0), granSize=(40, 250), granDensity=(10, 50), granSpray=(0.1, 0.8), granPitch=(0.0, 0.3), noise=(0.0, 0.25)),
  ["planante", "minimal", "oriental", "world"], ["mysterieuse", "sombre", "planante"],
  ["Poussière d'Étoiles", "Grains de Sable", "Fourmillement", "Vitrail Brisé", "Courant d'Air", "Forêt Numérique", "Ruche Lumineuse",
   "Mémoire Floue", "Écho Fossile", "Brouillard", "Particules", "Kaléidoscope Lent", "Orage Lointain", "Spirale", "Fractale", "Mosaïque"]),
 ("Voices", dict(tableA=T["Formants"], posA=0.3, tableB=T["Chœur"], posB=0.5, mixB=0.5, unison=3, detune=0.18, sub=0.0, cutoff=0.6,
                 fEnv=0.1, aA=180, aD=1500, aS=0.8, aR=900, wtLfo=0.2, lfoRate=0.3, lfoVib=0.12, chorus=0.3, reverb=0.5, revSize=0.8),
  dict(posA=(0.0, 1.0), posB=(0.2, 0.8), wtLfo=(0.1, 0.4)),
  ["gospel", "planante", "soul"], ["planante", "triste", "romantique"],
  ["Chœur de Verre", "Aaah Céleste", "Voix d'Ange", "Ooh Soul", "Murmure", "Kaléido Chœur"]),
 ("FX", dict(tableA=T["Numérique"], posA=0.7, tableB=T["Spectral"], posB=0.5, mixB=0.5, unison=2, detune=0.5, noise=0.2, cutoff=0.6,
             reso=0.5, fEnv=0.6, fA=800, fD=2000, fS=0.5, aA=200, aD=2000, aS=0.6, aR=2500, lfoCut=0.4, lfoRate=0.3, wtLfo=0.5,
             delay=0.35, delayFb=0.55, reverb=0.6, revSize=0.9),
  dict(posA=(0.0, 1.0), lfoRate=(0.05, 4.0), reso=(0.3, 0.7), noise=(0.05, 0.4)),
  ["dubstep", "techno", "guerre"], ["mysterieuse", "sombre", "epique"],
  ["Montée Prismatique", "Sonar", "Signal Perdu", "Vortex", "Tempête Solaire", "Rayon Kaléido"]),
]

# Mises à jour : 5 presets de plus à chaque version (jusqu'à 300 natifs au total avec les banques).
UPDATES = []   # ex. ("0.2.0", "Keys", "Nouveau nom", {"posA": 0.2})


def load_levels():
    p = os.path.join(ROOT, "tools", "levels.json")
    return json.load(open(p, encoding="utf-8")) if os.path.exists(p) else {}


def build():
    levels = load_levels()
    out = []
    for cat, base, var, styles, moods, names in CATS:
        for i, name in enumerate(names):
            rnd = random.Random(sum(map(ord, cat + name)) * 7919 + i)
            p = dict(BASE); p.update(base)
            if i > 0:   # le premier de chaque catégorie reste le gabarit pur
                for k, r in var.items():
                    if isinstance(r, list): p[k] = rnd.choice(r)
                    else:
                        v = rnd.uniform(*r)
                        p[k] = round(v, 3) if isinstance(BASE.get(k, 0.0), float) else int(round(v))
            for k in ("aA", "aD", "aR", "fA", "fD", "fR", "glideMs", "granSize", "granDensity"):
                p[k] = round(float(p[k]), 1)
            pid = f"{cat.lower()}-{i + 1:02d}"
            p["levelDb"] = round(levels.get(pid, -6.0), 2)
            out.append(dict(id=pid, name=name, cat=cat, style=styles[i % len(styles)], mood=moods[i % len(moods)], version="0.1.0", params=p))
    for ver, cat, name, delta in UPDATES:
        base = next(c for c in CATS if c[0] == cat)
        p = dict(BASE); p.update(base[1]); p.update(delta)
        n = sum(1 for x in out if x["cat"] == cat) + 1
        pid = f"{cat.lower()}-{n:02d}"; p["levelDb"] = round(levels.get(pid, -6.0), 2)
        out.append(dict(id=pid, name=name, cat=cat, style=base[3][0], mood=base[4][0], version=ver, params=p))
    return out


# Fonds : instrument échantillonné + réglages de jeu adaptés (le son vient de la banque installée à part)
FOND = dict(wtOn=False, smpOn=True, smpLevel=1.0, cutoff=1.0, reso=0.0, fEnv=0.0, keytrack=0.0, drive=0.0, sub=0.0, unison=1,
            aA=1.5, aD=4000, aS=1.0, aR=450, chorus=0.0, delay=0.0, reverb=0.22, revSize=0.65, width=1.0, levelDb=-3.0)


def fonds():
    out = []
    for b in banks.entries():
        p = dict(BASE); p.update(FOND)
        if b["sustain"]: p.update(aA=25, aR=380)
        else: p.update(aR=900)
        if b["cat"] in ("Orgues", "Cordes", "Bois", "Cuivres", "Flûtes", "Anches"): p["reverb"] = 0.3
        out.append(dict(id="fond-" + b["id"], name=b["name"], cat=b["cat"], fond=True, bank=b["id"], style=b["style"], mood=b["mood"],
                        version=b["version"], params=p))
    return out


def cval(v):
    if isinstance(v, bool): return "true" if v else "false"
    if isinstance(v, int): return str(v)
    return f"{float(v)}f"


def main():
    presets = build()
    assert len(presets) >= 150, len(presets)
    assert len({p["name"] for p in presets}) == len(presets), "noms en double"
    os.makedirs(os.path.join(ROOT, "ui", "presets"), exist_ok=True)
    allp = presets + fonds()
    assert len(allp) >= 250, len(allp)
    json.dump(dict(version=1, presets=allp), open(os.path.join(ROOT, "ui", "presets", "factory.json"), "w", encoding="utf-8"), ensure_ascii=False, indent=0)
    with open(os.path.join(ROOT, "tests", "presets.inc"), "w", encoding="utf-8") as f:
        f.write("// généré par tools/presets.py — ne pas éditer\n")
        for p in presets:
            body = " ".join(f"p.{k} = {cval(v)};" for k, v in p["params"].items() if k != "levelDb")
            f.write(f'{{ "{p["id"]}", "{p["name"]}", [] (kal::SynthParams& p) {{ {body} }}, {cval(p["params"]["levelDb"])} }},\n')
    cats = {}
    for p in presets: cats[p["cat"]] = cats.get(p["cat"], 0) + 1
    print(len(presets), "presets natifs :", cats, "+", len(allp) - len(presets), "fonds =", len(allp))


if __name__ == "__main__":
    main()
