# Kaleidoscope — Room Labs Audio

Générateur de mélodies infinies (VST3 / AU / autonome), JUCE 8 + interface WebView.

| Dossier | Contenu |
|---|---|
| `source/engine/Generator.*` | grammaires de style, Solo / Accompagnement, Déclinaison, anti-répétition, Personal |
| `source/engine/SynthEngine.*` | moteur hybride : 8 tables d'ondes à morphing, filtre, grains, échantillons, effets |
| `source/Plugin*` | paramètres, séquenceur calé sur l'hôte, sortie MIDI, fonds, état, pont avec l'interface |
| `ui/` | interface (4 layouts), `kscope.js` (kaléidoscope WebGL2), `drop.js` (analyse Drop), `layouts.json` (assets attendus) |
| `tools/styles.py` | 59 styles, 10 humeurs, 20 gammes → `source/gen/*.inc`, `ui/styles.json` |
| `tools/presets.py` | 150 presets natifs + 100 fiches de fonds → `ui/presets/factory.json` |
| `tools/banks.py`, `tools/build_banks.py` | les 100 fonds CC0 (VCSL, VSCO 2 CE) et leur fabrication (workflow « Kaleidoscope - Fonds ») |
| `tests/` | générateur, moteur (niveaux calibrés), plugin complet |
| `docs/gabarits/` | gabarits des layouts pour dessiner les assets |

Assets : `ui/assets/<layout>_<clé>.webp` (voir `ui/layouts.json`) remplacent le dessin CSS dès qu'ils existent.
Pour essayer sans recompiler : `Documents/Room Labs/Kaleidoscope/Assets/`.

Ajouter des presets (chaque mise à jour : +5) : `tools/presets.py` → `UPDATES`, puis `tests/calibrate_levels.cpp` et relancer `presets.py`.
Ajouter des fonds (chaque mise à jour : +25) : `tools/banks.py` → déplacer de `RESERVE`, relancer le workflow des fonds.
