#!/usr/bin/env python3
"""Kaleidoscope — les 100 « fonds » (instruments échantillonnés) de la v0.1, source de vérité.

Sources, toutes en CC0 (domaine public, licence lue dans le fichier LICENSE de chaque dépôt) :
  VCSL        https://github.com/sgossner/VCSL         (Versilian Community Sample Library)
  VSCO-2-CE   https://github.com/sgossner/VSCO-2-CE    (Versilian Studios Chamber Orchestra 2 CE)
Exclus volontairement : Karoryfer, Philharmonia (cahier des charges).

Ce script écrit ui/banks.json (catalogue embarqué dans le plugin : noms visibles même si la banque
n'est pas installée) et licences/inventaire.csv. tools/build_banks.py fabrique les fichiers audio (CI).
Chaque mise à jour ajoute 25 fonds : les ajouter dans UPDATES (la RESERVE contient déjà les suivants).
"""
import csv, json, os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VC, VS = "vcsl", "vsco-2-ce"
REPOS = {VC: "https://github.com/sgossner/VCSL", VS: "https://github.com/sgossner/VSCO-2-CE"}

# (id, nom, catégorie, dépôt, dossier, tenu ?, couches de vélocité max, style conseillé, humeur conseillée)
B = [
 # --- Pianos & claviers
 ("piano-kawai", "Grand Piano Kawai", "Pianos", VC, "Chordophones/Zithers/Grand Piano, Kawai/Sustains", False, 2, "neosoul", "romantique"),
 ("piano-steinway", "Grand Piano Steinway B", "Pianos", VC, "Chordophones/Zithers/Grand Piano, Steinway B/Sus", False, 2, "classique", "triste"),
 ("piano-steinway-sec", "Steinway Étouffé", "Pianos", VC, "Chordophones/Zithers/Grand Piano, Steinway B/NoSus", False, 2, "lofi", "nostalgique"),
 ("piano-knight", "Piano Droit Knight", "Pianos", VC, "Chordophones/Zithers/Upright Piano, Knight/Sustains", False, 2, "lofi", "nostalgique"),
 ("piano-yamaha", "Piano Droit Yamaha", "Pianos", VC, "Chordophones/Zithers/Upright Piano, Yamaha/Sustains", False, 2, "variete", "romantique"),
 ("piano-ur1", "Piano Droit de Salon", "Pianos", VS, "Keys/Upright Nr1", False, 2, "jazz", "nostalgique"),
 ("clavecin-anglais", "Clavecin Anglais", "Clavecins", VC, "Chordophones/Zithers/Harpsichord, English/Sustains/Normal", False, 1, "classique", "luxueuse"),
 ("clavecin-luth", "Clavecin Jeu de Luth", "Clavecins", VC, "Chordophones/Zithers/Harpsichord, English/Sustains/Lute", False, 1, "classique", "mysterieuse"),
 ("clavecin-flamand", "Clavecin Flamand", "Clavecins", VC, "Chordophones/Zithers/Harpsichord, Flemish/Sustains/High", False, 1, "classique", "epique"),
 ("clavecin-francais", "Clavecin Français", "Clavecins", VC, "Chordophones/Zithers/Harpsichord, French/Sustains", False, 1, "classique", "luxueuse"),
 ("clavecin-italien", "Clavecin Italien", "Clavecins", VC, "Chordophones/Zithers/Harpsichord, Italian/Sustains/stop1", False, 1, "trap", "sombre"),
 ("fm-clavisynth", "Clavisynth FM", "Claviers FM", VC, "Electrophones/TX81Z/Clavisynth", False, 1, "eighties", "energique"),
 ("fm-piano", "Piano FM", "Claviers FM", VC, "Electrophones/TX81Z/FM Piano", False, 1, "rnb", "romantique"),
 ("fm-piano1", "Piano Numérique 1987", "Claviers FM", VC, "Electrophones/TX81Z/Piano 1", False, 1, "eighties", "nostalgique"),
 ("orgue-plein", "Grand Orgue", "Orgues", VC, "Aerophones/Edge-blown Aerophones/Pipe Organ/Loud", True, 1, "gospel", "epique"),
 ("orgue-doux", "Orgue Doux", "Orgues", VC, "Aerophones/Edge-blown Aerophones/Pipe Organ/Quiet", True, 1, "gospel", "planante"),
 ("orgue-pedale", "Pédalier d'Orgue", "Orgues", VC, "Aerophones/Edge-blown Aerophones/Pipe Organ/Loud Pedal", True, 1, "guerre", "sombre"),
 ("orgue-pedale-doux", "Pédalier Doux", "Orgues", VC, "Aerophones/Edge-blown Aerophones/Pipe Organ/Quiet Pedal", True, 1, "planante", "mysterieuse"),
 ("orgue-renaissance", "Orgue Renaissance 8'", "Orgues", VC, "Aerophones/Edge-blown Aerophones/Renaissance Organ/8'", True, 1, "classique", "planante"),
 ("orgue-renaissance-plein", "Orgue Renaissance Plein Jeu", "Orgues", VC, "Aerophones/Edge-blown Aerophones/Renaissance Organ/Full", True, 1, "gospel", "epique"),
 # --- Percussions mélodiques
 ("vibra-doux", "Vibraphone Doux", "Mailloches", VC, "Idiophones/Struck Idiophones/Vibraphone/Soft Mallets", False, 2, "jazz", "romantique"),
 ("vibra-dur", "Vibraphone Brillant", "Mailloches", VC, "Idiophones/Struck Idiophones/Vibraphone/Hard Mallets", False, 2, "ukg", "luxueuse"),
 ("vibra-archet", "Vibraphone à l'Archet", "Mailloches", VC, "Idiophones/Struck Idiophones/Vibraphone/Bowed", True, 1, "planante", "mysterieuse"),
 ("marimba", "Marimba", "Mailloches", VC, "Idiophones/Struck Idiophones/Marimba", False, 2, "amapiano", "joyeuse"),
 ("marimba-chambre", "Marimba de Chambre", "Mailloches", VS, "Percussion/Marimba", False, 1, "afrobeat", "joyeuse"),
 ("xylo-dur", "Xylophone Sec", "Mailloches", VC, "Idiophones/Struck Idiophones/Xylophone/Hard Mallets", False, 1, "joyeuse", "joyeuse"),
 ("xylo-moyen", "Xylophone", "Mailloches", VC, "Idiophones/Struck Idiophones/Xylophone/Medium Mallets", False, 1, "afrorumba", "joyeuse"),
 ("xylo-doux", "Xylophone Feutré", "Mailloches", VC, "Idiophones/Struck Idiophones/Xylophone/Soft Mallets", False, 1, "lofi", "planante"),
 ("glockenspiel", "Glockenspiel", "Cloches", VC, "Idiophones/Struck Idiophones/Glockenspiel", False, 1, "trap", "mysterieuse"),
 ("cloches-tubes", "Cloches Tubulaires", "Cloches", VC, "Idiophones/Struck Idiophones/Tubular Bells 1", False, 1, "guerre", "epique"),
 ("cloches-tubes-2", "Carillon d'Église", "Cloches", VC, "Idiophones/Struck Idiophones/Tubular Bells 2", False, 1, "gospel", "triste"),
 ("glock-tubulaire", "Glockenspiel Tubulaire", "Cloches", VC, "Idiophones/Struck Idiophones/Tubular Glockenspiel", False, 1, "asiatique", "mysterieuse"),
 ("hand-chimes", "Cloches à Main", "Cloches", VC, "Idiophones/Struck Idiophones/Hand Chimes", False, 1, "planante", "planante"),
 ("balafon-dur", "Balafon Sec", "Afrique", VC, "Idiophones/Struck Idiophones/Balafon/Hard Mallet", False, 1, "coupedecale", "energique"),
 ("balafon-doux", "Balafon Doux", "Afrique", VC, "Idiophones/Struck Idiophones/Balafon/Soft Mallet", False, 1, "afrolove", "romantique"),
 ("balafon", "Balafon Traditionnel", "Afrique", VC, "Idiophones/Struck Idiophones/Balafon/Traditional Mallet", False, 1, "sebene", "joyeuse"),
 ("kalimba-kenya", "Kalimba du Kenya", "Afrique", VC, "Idiophones/Plucked Idiophones/Kalimba, Kenya", False, 1, "afrobeat", "joyeuse"),
 ("kalimba-tanzanie", "Kalimba de Tanzanie", "Afrique", VC, "Idiophones/Plucked Idiophones/Kalimba, Tanzania", False, 1, "amapiano", "planante"),
 ("mbira-mavembe", "Mbira Mavembe", "Afrique", VC, "Idiophones/Plucked Idiophones/Mbira Mavembe (Gandanga), Zimbabwe, Low G", False, 1, "afrorumba", "nostalgique"),
 ("mbira-nyamaropa", "Mbira Nyamaropa", "Afrique", VC, "Idiophones/Plucked Idiophones/Mbira dzaVadzimu Nyamaropa, Zimbabwe, Low B", False, 1, "world", "mysterieuse"),
 ("nyunga", "Nyunga Nyunga", "Afrique", VC, "Idiophones/Plucked Idiophones/Nyunga Nyunga, Mozambique, Low F", False, 1, "rumbacongo", "joyeuse"),
 ("verres-lents", "Verres Chantants", "Cloches", VC, "Idiophones/Friction Idiophones/Wine Glasses/Sustains/Slow", True, 1, "planante", "mysterieuse"),
 ("verres-rapides", "Verres Vibrants", "Cloches", VC, "Idiophones/Friction Idiophones/Wine Glasses/Sustains/Fast", True, 1, "planante", "planante"),
 # --- Cordes pincées
 ("harpe-concert", "Harpe de Concert", "Harpes", VC, "Chordophones/Composite Chordophones/Concert Harp", False, 1, "classique", "romantique"),
 ("harpe-folk", "Harpe Celtique", "Harpes", VC, "Chordophones/Composite Chordophones/Folk Harp", False, 1, "world", "nostalgique"),
 ("harpe-chambre", "Harpe de Chambre", "Harpes", VS, "Strings/Harp", False, 1, "rnb", "romantique"),
 ("strumstick", "Strumstick", "Cordes pincées", VC, "Chordophones/Composite Chordophones/Strumstick/Finger", False, 1, "bossa", "joyeuse"),
 ("dan-tranh", "Đàn Tranh", "Cordes pincées", VC, "Chordophones/Zithers/Dan Tranh/Normal", False, 1, "asiatique", "mysterieuse"),
 ("dan-tranh-tremolo", "Đàn Tranh Trémolo", "Cordes pincées", VC, "Chordophones/Zithers/Dan Tranh/Tremolo", True, 1, "asiatique", "planante"),
 ("dan-tranh-vibrato", "Đàn Tranh Vibrato", "Cordes pincées", VC, "Chordophones/Zithers/Dan Tranh/Vibrato", False, 1, "asiatique", "triste"),
 ("psalterion", "Psaltérion Pincé", "Cordes pincées", VC, "Chordophones/Zithers/Psaltery, Bowed and Plucked/Pluck", False, 1, "oriental", "mysterieuse"),
 ("psalterion-archet", "Psaltérion à l'Archet", "Cordes pincées", VC, "Chordophones/Zithers/Psaltery, Bowed and Plucked/LongBow", True, 1, "oriental", "planante"),
 # --- Cordes frottées
 ("violons", "Violons", "Cordes", VS, "Strings/Violin Section/susVib", True, 1, "classique", "epique"),
 ("violons-pizz", "Violons Pizzicato", "Cordes", VS, "Strings/Violin Section/Pizz", False, 1, "drill", "sombre"),
 ("violons-spic", "Violons Spiccato", "Cordes", VS, "Strings/Violin Section/Spic", False, 1, "drill", "epique"),
 ("violons-trem", "Violons Trémolo", "Cordes", VS, "Strings/Violin Section/Trem", True, 1, "guerre", "sombre"),
 ("altos", "Altos", "Cordes", VS, "Strings/Viola Section/susvib", True, 1, "classique", "triste"),
 ("altos-pizz", "Altos Pizzicato", "Cordes", VS, "Strings/Viola Section/pizz", False, 1, "rap", "mysterieuse"),
 ("altos-trem", "Altos Trémolo", "Cordes", VS, "Strings/Viola Section/trem", True, 1, "guerre", "mysterieuse"),
 ("violoncelles", "Violoncelles", "Cordes", VS, "Strings/Cello Section/susvib", True, 1, "triste", "triste"),
 ("violoncelles-pizz", "Violoncelles Pizzicato", "Cordes", VS, "Strings/Cello Section/pizzT", False, 1, "jazz", "mysterieuse"),
 ("violoncelles-spic", "Violoncelles Spiccato", "Cordes", VS, "Strings/Cello Section/spic", False, 1, "drill", "energique"),
 ("violoncelles-trem", "Violoncelles Trémolo", "Cordes", VS, "Strings/Cello Section/trem", True, 1, "guerre", "sombre"),
 ("violon-solo", "Violon Solo", "Cordes", VS, "Strings/Solo Violin/Arco Vib", True, 1, "oriental", "romantique"),
 ("violon-solo-pizz", "Violon Solo Pizzicato", "Cordes", VS, "Strings/Solo Violin/Pizz", False, 1, "trap", "mysterieuse"),
 ("violon-solo-spic", "Violon Solo Spiccato", "Cordes", VS, "Strings/Solo Violin/spic", False, 1, "rap", "energique"),
 ("violon-solo-trem", "Violon Solo Trémolo", "Cordes", VS, "Strings/Solo Violin/Trem", True, 1, "guerre", "sombre"),
 ("contrebasse", "Contrebasse Vibrato", "Cordes", VS, "Strings/Solo Contrabass/SusVib", True, 1, "classique", "sombre"),
 ("contrebasse-droite", "Contrebasse Droite", "Cordes", VS, "Strings/Solo Contrabass/SusNV", True, 1, "guerre", "sombre"),
 ("contrebasse-pizz", "Contrebasse Pizzicato", "Cordes", VS, "Strings/Solo Contrabass/Pizz", False, 1, "jazz", "nostalgique"),
 ("contrebasse-spic", "Contrebasse Spiccato", "Cordes", VS, "Strings/Solo Contrabass/Spic", False, 1, "drill", "sombre"),
 # --- Bois
 ("flute", "Flûte Traversière", "Bois", VS, "Woodwinds/Flute/susvib", True, 1, "classique", "planante"),
 ("flute-expressive", "Flûte Expressive", "Bois", VS, "Woodwinds/Flute/expvib", True, 1, "trap", "triste"),
 ("flute-stac", "Flûte Staccato", "Bois", VS, "Woodwinds/Flute/stac", False, 1, "afrotrap", "joyeuse"),
 ("hautbois", "Hautbois", "Bois", VS, "Woodwinds/Oboe/Vib", True, 1, "oriental", "triste"),
 ("clarinette", "Clarinette", "Bois", VS, "Woodwinds/Clarinet/susLong", True, 1, "jazz", "nostalgique"),
 ("clarinette-stac", "Clarinette Staccato", "Bois", VS, "Woodwinds/Clarinet/stac", False, 1, "joyeuse", "joyeuse"),
 ("basson", "Basson", "Bois", VS, "Woodwinds/Bassoon/vib", True, 1, "classique", "mysterieuse"),
 ("piccolo", "Piccolo", "Bois", VS, "Woodwinds/Piccolo/Sus", True, 1, "guerre", "epique"),
 ("flute-a-bec-alto", "Flûte à Bec Alto", "Flûtes", VC, "Aerophones/Edge-blown Aerophones/Baroque Alto Recorder/Sustain", True, 1, "world", "nostalgique"),
 ("flute-a-bec-soprano", "Flûte à Bec Soprano", "Flûtes", VC, "Aerophones/Edge-blown Aerophones/Baroque Soprano Recorder/Sustain", True, 1, "joyeuse", "joyeuse"),
 ("flute-a-bec-tenor", "Flûte à Bec Ténor", "Flûtes", VC, "Aerophones/Edge-blown Aerophones/Baroque Tenor Recorder/SusVib", True, 1, "asiatique", "planante"),
 ("flute-a-bec-basse", "Flûte à Bec Basse", "Flûtes", VC, "Aerophones/Edge-blown Aerophones/Baroque Bass Recorder/Sustain", True, 1, "lofi", "mysterieuse"),
 ("ocarina", "Ocarina", "Flûtes", VC, "Aerophones/Edge-blown Aerophones/Ocarina, Typical/Sustains/Sus", True, 1, "afrotrap", "nostalgique"),
 ("ocarina-petit", "Petit Ocarina", "Flûtes", VC, "Aerophones/Edge-blown Aerophones/Ocarina, Small/Sustain", True, 1, "trap", "mysterieuse"),
 ("harmonica", "Harmonica", "Anches", VC, "Aerophones/Free Aerophones/Harmonica-Hohner-Special20-C/Sustains/Normal", True, 1, "rock", "nostalgique"),
 ("harmonica-vibrato", "Harmonica Vibrato Main", "Anches", VC, "Aerophones/Free Aerophones/Harmonica-Hohner-Special20-F/Sustains/HandVib", True, 1, "soul", "triste"),
 ("harmonica-chromatique", "Harmonica Chromatique", "Anches", VC, "Aerophones/Free Aerophones/Harmonica-Hohner-Super64/Sustains/Vib", True, 1, "jazz", "romantique"),
 ("sax-tenor", "Saxophone Ténor", "Anches", VC, "Aerophones/Reed Aerophones/Tenor Saxophone/Vibrato", True, 1, "rnb", "romantique"),
 ("sax-tenor-droit", "Saxophone Ténor Droit", "Anches", VC, "Aerophones/Reed Aerophones/Tenor Saxophone/Non-Vibrato", True, 1, "jazz", "nostalgique"),
 ("saxello", "Saxello", "Anches", VC, "Aerophones/Reed Aerophones/Saxello/Vibrato", True, 1, "zouk", "romantique"),
 # --- Cuivres
 ("trompette", "Trompette", "Cuivres", VS, "Brass/Trumpet/sus", True, 1, "kompa", "energique"),
 ("trompette-harmon", "Trompette Sourdine Harmon", "Cuivres", VS, "Brass/Trumpet/harmonM-sus", True, 1, "jazz", "nostalgique"),
 ("trompette-stac", "Trompette Staccato", "Cuivres", VS, "Brass/Trumpet/stac", False, 1, "latino", "energique"),
 ("cor", "Cor d'Harmonie", "Cuivres", VS, "Brass/F Horn/sus", True, 1, "guerre", "epique"),
 ("cor-sourdine", "Cor en Sourdine", "Cuivres", VS, "Brass/F Horn/mute", True, 1, "classique", "mysterieuse"),
 ("trombone", "Trombone", "Cuivres", VS, "Brass/Tenor Trombone/sus", True, 1, "reggae", "energique"),
 ("trombone-vibrato", "Trombone Vibrato", "Cuivres", VS, "Brass/Tenor Trombone/vib", True, 1, "soul", "romantique"),
 ("tuba", "Tuba", "Cuivres", VS, "Brass/Tuba/sus", True, 1, "guerre", "sombre"),
 ("trombone-ancien", "Vieux Trombone", "Cuivres", VS, "Brass/OldTrombone/Sustain", True, 1, "jazz", "nostalgique"),
]

# Fonds déjà repérés pour les prochaines mises à jour (25 par version)
RESERVE = [
 ("piano-kawai-ancien", "Grand Piano d'Époque", "Pianos", VC, "Chordophones/Zithers/Grand Piano, Kawai - Legacy/Sustains", False, 2, "classique", "nostalgique"),
 ("clavecin-flamand-grave", "Clavecin Flamand Grave", "Clavecins", VC, "Chordophones/Zithers/Harpsichord, Flemish/Sustains/Low", False, 1, "classique", "sombre"),
 ("clavecin-inconnu", "Clavecin Mystère", "Clavecins", VC, "Chordophones/Zithers/Harpsichord, Unk/Sustains", False, 1, "trap", "mysterieuse"),
 ("orgue-renaissance-4", "Orgue Renaissance 4'", "Orgues", VC, "Aerophones/Edge-blown Aerophones/Renaissance Organ/4'", True, 1, "classique", "planante"),
 ("xylo-chambre", "Xylophone de Chambre", "Mailloches", VS, "Percussion/Xylo", False, 1, "joyeuse", "joyeuse"),
 ("glock-chambre", "Glockenspiel de Chambre", "Cloches", VS, "Percussion/Glock", False, 1, "trap", "mysterieuse"),
 ("psalterion-spic", "Psaltérion Sautillé", "Cordes pincées", VC, "Chordophones/Zithers/Psaltery, Bowed and Plucked/Spiccato", False, 1, "oriental", "energique"),
 ("altos-spic", "Altos Spiccato", "Cordes", VS, "Strings/Viola Section/spic", False, 1, "drill", "energique"),
 ("hautbois-droit", "Hautbois Droit", "Bois", VS, "Woodwinds/Oboe/Sus", True, 1, "classique", "planante"),
 ("basson-stac", "Basson Staccato", "Bois", VS, "Woodwinds/Bassoon/stac", False, 1, "joyeuse", "joyeuse"),
 ("cor-stac", "Cor Staccato", "Cuivres", VS, "Brass/F Horn/stac", False, 1, "guerre", "epique"),
 ("tuba-stac", "Tuba Staccato", "Cuivres", VS, "Brass/Tuba/stac", False, 1, "bouyon", "energique"),
 ("trombone-stac", "Trombone Staccato", "Cuivres", VS, "Brass/Tenor Trombone/stac", False, 1, "ragga", "energique"),
 ("trompette-droite", "Trompette Sourdine Sèche", "Cuivres", VS, "Brass/Trumpet/straightM-sus", True, 1, "jazz", "mysterieuse"),
 ("trompette-vibrato", "Trompette Vibrato", "Cuivres", VS, "Brass/Trumpet/susvib", True, 1, "kompa", "romantique"),
 ("flute-droite", "Flûte sans Vibrato", "Bois", VS, "Woodwinds/Flute/susNV", True, 1, "planante", "planante"),
 ("flute-a-bec-stac", "Flûte à Bec Piquée", "Flûtes", VC, "Aerophones/Edge-blown Aerophones/Baroque Alto Recorder/Staccato", False, 1, "joyeuse", "joyeuse"),
 ("ocarina-vibrato", "Ocarina Vibrato", "Flûtes", VC, "Aerophones/Edge-blown Aerophones/Ocarina, Typical/Sustains/SusVib", True, 1, "afrotrap", "triste"),
 ("harmonica-doux", "Harmonica Doux", "Anches", VC, "Aerophones/Free Aerophones/Harmonica-Hohner-Special20-C/Sustains/Soft", True, 1, "lofi", "nostalgique"),
 ("harmonica-accent", "Harmonica Accentué", "Anches", VC, "Aerophones/Free Aerophones/Harmonica-Hohner-Special20-F/Sustains/Accented", True, 1, "rock", "energique"),
 ("sax-tenor-stac", "Saxophone Ténor Staccato", "Anches", VC, "Aerophones/Reed Aerophones/Tenor Saxophone/Staccato", False, 1, "afrobeat", "energique"),
 ("saxello-droit", "Saxello Droit", "Anches", VC, "Aerophones/Reed Aerophones/Saxello/Non-Vibrato", True, 1, "jazz", "planante"),
 ("flute-a-bec-basse-vib", "Flûte à Bec Basse Vibrato", "Flûtes", VC, "Aerophones/Edge-blown Aerophones/Baroque Bass Recorder/SusVib", True, 1, "lofi", "triste"),
 ("flute-a-bec-alto-vib", "Flûte à Bec Alto Vibrato", "Flûtes", VC, "Aerophones/Edge-blown Aerophones/Baroque Alto Recorder/SusVib", True, 1, "world", "romantique"),
 ("ocarina-petit-stac", "Petit Ocarina Piqué", "Flûtes", VC, "Aerophones/Edge-blown Aerophones/Ocarina, Small/Staccato", False, 1, "afrotrap", "joyeuse"),
]

# 25 fonds par mise à jour, pris dans RESERVE
UPDATES = {"0.2.0": [r[0] for r in RESERVE[:25]]}


def entries(include_reserve=False):
    """fonds livrés (v0.1 + mises à jour) ; avec include_reserve, aussi ceux qui attendent une prochaine version"""
    ver = {i: v for v, ids in UPDATES.items() for i in ids}
    rows = [(r, "0.1.0") for r in B] + [(r, ver[r[0]]) for r in RESERVE if r[0] in ver]
    if include_reserve: rows += [(r, "réserve") for r in RESERVE if r[0] not in ver]
    out = []
    for (bid, name, cat, repo, folder, sus, layers, style, mood), v in rows:
        out.append(dict(id=bid, name=name, cat=cat, source="VCSL" if repo == VC else "VSCO 2 CE", repo=repo, folder=folder,
                        sustain=sus, layers=layers, style=style, mood=mood, license="CC0 1.0", version=v))
    return out


def main():
    rows = entries()
    n = len(B) + sum(len(v) for v in UPDATES.values())
    assert len(rows) == n, (len(rows), n)
    assert len({r["id"] for r in rows}) == n and len({r["name"] for r in rows}) == n
    json.dump(dict(version=1, folder="Banques", banks=[{k: r[k] for k in ("id", "name", "cat", "source", "sustain", "style", "mood", "license", "version")} for r in rows]),
              open(os.path.join(ROOT, "ui", "banks.json"), "w", encoding="utf-8"), ensure_ascii=False, indent=0)
    os.makedirs(os.path.join(ROOT, "licences"), exist_ok=True)
    with open(os.path.join(ROOT, "licences", "inventaire.csv"), "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["id", "nom", "catégorie", "source", "dépôt", "dossier", "licence", "preuve", "statut"])
        for r in entries(True):
            w.writerow([r["id"], r["name"], r["cat"], r["source"], REPOS[r["repo"]], r["folder"], "CC0 1.0",
                        "LICENSE du dépôt (CC0 1.0 Universal)", r["version"]])
    cats = {}
    for r in rows: cats[r["cat"]] = cats.get(r["cat"], 0) + 1
    print(len(rows), "fonds :", cats)


if __name__ == "__main__":
    main()
