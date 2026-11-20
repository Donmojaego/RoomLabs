# Candy Shop — Room Labs Audio

Instrument pour les hooks, le club et la fête. La vitrine de confiserie sert de navigateur : chaque bonbon ouvre une famille de sons et s'illumine quand on joue.
VST3 (Windows, macOS), AU (macOS) et application autonome. JUCE 8.0.15, interface web intégrée.

| Bonbon | Famille | Moteur |
|---|---|---|
| Fraise | Plug Seed Pack | FM : cloches plugg, accents de fête |
| Banane | Marimbas | Maillet (synthèse modale) |
| Cola | Synths | Synthé : lead pétillant (unisson) |
| Violet | Pads | Synthé : attaque lente, chorus, Space |
| Cerise | Percussion | Synthé / FM courts et brillants |
| Sucette | Sucette | Lead mélodique |
| Carrés fruités | Accords | Un accord par touche |
| Lacets acidulés | Arpèges | Arpégiateur calé sur le tempo de l'hôte |

Boîte à rythmes : notes 24 à 35 (C0 à B0) ou canal MIDI 10 — tam-tams, tambour, claves, claquement, tics d'horloge, bruits de machine, shaker. 5 kits.

## Dossiers

| Dossier | Contenu |
|---|---|
| `source/engine/` | Moteur en C++ pur (16 voix, maillet, synthé, FM, accords, arpège, percussions, chorus, réverbération) |
| `source/` | Processeur, paramètres (identifiants figés), éditeur web |
| `ui/` | Interface (même page que l'aperçu en ligne) et images converties |
| `presets/` | 80 presets (`tools/generate_factory_presets.py`), niveaux calibrés (`tools/calibrate_levels.cpp`) |
| `tests/` | Test du moteur (sans JUCE) et du plugin complet |
| `docs/reference/` | Planches, manifest et note du pack d'images |

## Tester

```bash
cmake -B build-test -DCDY_ENGINE_TEST_ONLY=ON && cmake --build build-test && ./build-test/CandyShopEngineTest RAPPORT.md
python3 tools/build_preview.py   # aperçu jouable dans build/preview/index.html
```

Règles : aucun rendu, verrou ni allocation dans le callback audio ; le thème ne change jamais le son ; identifiants figés (fabricant `Rmlb`, plugin `CdyS`).
