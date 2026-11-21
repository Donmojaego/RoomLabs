# Smoke — Room Labs Audio

Claviers, nappes, risers et basses pour le R&B, le hip-hop et la house. La fumée qui monte du joint suit le son, les yeux rougissent avec l'intensité des effets.
VST3 (Windows, macOS), AU (macOS) et application autonome. JUCE 8.0.15, interface web intégrée.

| Banque | Contenu | Modèles |
|---|---|---|
| KEYS | pianos feutrés, accords R&B, pianos qui dérivent | Piano feutré (8 partiels légèrement inharmoniques) |
| RHODES | Rhodes chauds, trémolo, accords de nuit | Rhodes (FM + tine) |
| RISERS | nappes mouvantes, montées, longues queues | Nappe, Riser (3 dents de scie, montée d'une quinte) |
| SUB | subs ronds, basses planantes, basses house | Sub, Basse house (mono, glisse) |

Modules : MOD (dérive + chorus), TONE (filtre), SPACE (réverbération), DREAM (pré-délai, queue longue), CLEAN (saturation douce). Accords : m9, maj9, m11, 9, maj7. 12 voix, 40 presets (10 par banque), niveaux calibrés vers -6 dBFS.

## Images

Pack `SMOKE_Assets` → `tools/prepare_assets.py <dossier du pack> ui/assets` : recadrage, WebP, repère des molettes effacé (l'interface dessine le sien). Thèmes Prune (joint papier), Bleu (Woodback), Vert (Woodback vert).

## Tester

```bash
cmake -B build-test -DSMK_ENGINE_TEST_ONLY=ON && cmake --build build-test && ./build-test/SmokeEngineTest RAPPORT.md
python3 tools/build_preview.py && python3 tests/ui/measure_animation.py "$PWD/build/preview/index.html"
```

Identifiants figés : fabricant `Rmlb`, plugin `Smok`.
