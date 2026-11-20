# Shake It — Room Labs Audio

Instrument 808 et basses, mono legato, avec une scène animée qui danse sur le grave.
VST3 (Windows, macOS), AU (macOS) et application autonome. JUCE 8.0.15, interface web intégrée.

## Ce qu'il y a dedans

| Dossier | Contenu |
|---|---|
| `source/engine/` | Moteur de son en C++ pur (808, Sub, Moog-style, corde pincée), testable sans JUCE |
| `source/` | Processeur du plugin, paramètres (identifiants figés), éditeur web et pont « motion » |
| `ui/index.html` | Toute l'interface (la même page sert d'aperçu en ligne) |
| `ui/assets/` | Danseur (3 poses détourées), logo, icône |
| `presets/factory.json` | 95 presets d'usine (générés par `tools/generate_factory_presets.py`) |
| `tests/` | Test du moteur, test du plugin complet, mesure de l'animation |
| `installer/` | Installeur Windows (Inno Setup) et DMG macOS |
| `docs/` | Cahier de livraison, état du projet, à faire, maquette de référence |

## Compiler

```bash
cmake -B build -DSHK_BUILD_TESTS=ON
cmake --build build --config Release
```

GitHub Actions compile Windows et macOS à chaque envoi et publie les installeurs dans Releases.

## Tester

```bash
# moteur seul, sans JUCE (quelques secondes)
cmake -B build-test -DSHK_ENGINE_TEST_ONLY=ON && cmake --build build-test && ./build-test/ShakeItEngineTest RAPPORT.md
# plugin complet hors DAW
./build/ShakeItRenderTest_artefacts/Release/ShakeItRenderTest RAPPORT.md
# animation (navigateur) : fabrique l'aperçu puis mesure
python3 tools/build_preview.py && python3 tests/ui/measure_animation.py "$PWD/build/preview/index.html"
```

## Règles

- Aucun rendu, verrou ni allocation dans le callback audio. Le moteur écrit l'énergie du grave, un compteur d'attaques, le glide et la note dans des variables atomiques ; l'éditeur les lit à 60 Hz.
- Thème, tenue, taille et mode visuel sont des préférences d'interface : jamais de paramètre, jamais d'effet sur le son.
- Identifiants figés : fabricant `Rmlb`, plugin `ShkI`, identifiants des paramètres dans `source/Params.h`.
- Chaque nouvelle version ajoute au moins 20 presets.
