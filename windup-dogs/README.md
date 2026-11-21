# Windup Dogs — Room Labs Audio

Instruments jouets mécaniques : une console de jouet remontable, avec ses chiens musiciens. Chaque pièce du mécanisme est animée sur son axe : train d'engrenages (sens et vitesses par le nombre de dents), cylindre à picots et peigne, ressort et clés, marteau et cloche.
VST3 (Windows, macOS), AU (macOS) et application autonome. JUCE 8.0.15, interface web intégrée.

| Musicien | Instrument | Synthèse |
|---|---|---|
| Chien qui court | Boîte à musique | lames d'acier (partiels + deux lames qui battent) |
| Chef d'orchestre | Piano jouet | tiges frappées (partiels inharmoniques) |
| Cloches | Clochettes | partiels de cloche, longues queues |
| Trompettiste | Fanfare | impulsion + formants nasillards, vibrato |
| Batteur | Tambour | 12 percussions jouets, une par touche |

Mécanisme : clé de remontage (cliquet), ressort qui se détend pendant le jeu (hauteur qui baisse jusqu'à -70 cents, tic-tac libre qui ralentit), tic-tac calé sur l'hôte (1/4, 1/8, 1/16, 1/8T, swing). Effets : Tone, Méca, Wobble, Écho au tempo, Espace. 16 voix, 40 presets calibrés vers -6 dBFS.

## Images

Pack `Windup_Dogs_Assets` → `tools/prepare_assets.py <dossier du pack> ui/assets` : pièces animées découpées et effacées de la planche, train d'engrenages séparé roue par roue (dents comptées), cylindre sans picots (redessinés en rotation), spirale et clés du ressort, levier du marteau ; mesures dans `ui/assets/layout.json`.

## Tester

```bash
cmake -B build-test -DWD_ENGINE_TEST_ONLY=ON && cmake --build build-test && ./build-test/WindupDogsEngineTest RAPPORT.md
python3 tools/build_preview.py && python3 tests/ui/measure_animation.py "$PWD/build/preview/index.html"
```

Identifiants figés : fabricant `Rmlb`, plugin `WdDg`.
