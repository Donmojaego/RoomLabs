# Ma petite soupe ouselem — Room Labs Audio

Un sachet = un instrument + un module d'effet calé sur le tempo. Le bol visualise le mélange ; « Mon petit ramen » est un mode d'affichage du même plugin.
VST3 (Windows, macOS), AU (macOS) et application autonome. JUCE 8.0.15, interface web intégrée.

| Sachet | Recette | Instrument | Effet |
|---|---|---|---|
| 01 | Violon renversé | Violon | Reverse synchronisé |
| 02 | Pad mijoté | Pad / piano | Stutter à divisions musicales |
| 03 | Gouttes au sésame | Cloche « cloc », tambour « touc » | Répétitions |
| 04 | Nouilles longues | Cordes / texture | Étirement + relâchement long |
| 05 | Bouillon d'arpèges | Texture en arpège | Découpage rythmique |
| 06 | Fond de bol | Basse sourde et douce | Filtre modulé |

Effets disponibles partout : Reverse, Stutter, Répétitions, Découpage, Étirement, Filtre modulé, Grain Delay — divisions 1/2 à 1/16T, calées sur la position de l'hôte. Interrupteurs Reverse et Stutter en plus. Tone, Damp, Space, Mix.

## Images

Les maquettes (`docs/reference`) sont des images entières. `tools/prepare_assets.py` en tire un fond nettoyé, un calque par sachet et la surface du bol (coordonnées : `tools/layout.json`).

## Tester

```bash
cmake -B build-test -DSOUP_ENGINE_TEST_ONLY=ON && cmake --build build-test && ./build-test/SoupeOuselemEngineTest RAPPORT.md
python3 tools/build_preview.py && python3 tests/ui/measure_animation.py "$PWD/build/preview/index.html"
```

Identifiants figés : fabricant `Rmlb`, plugin `SpOu`.
