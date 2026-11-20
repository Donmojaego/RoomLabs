# Shake It — état du projet (20/11/2026, v0.1.0)

Fait et vérifié :
- Cahier des charges v1 (doc « Shake It · Cahier des charges v1 »).
- Moteur 808 / Sub / Moog-style / Pincée, mono legato, glide, 95 presets : tous les tests OK (docs/RAPPORT-TESTS.md).
- Plugin complet compilé sous Linux (VST3 + application autonome) : interface ouverte, presets reçus, images servies.
- Interface animée (ui/index.html) : grosse caisse, danseur 3 poses (neutre / impact / rebond), potards, presets,
  thèmes bronze / émeraude / bleu clair, modes Animé / Réduit / Statique, zoom 50–200 %, clavier jouable, F2 = mesures.
- Animation mesurée dans un navigateur : 60 i/s, rendu < 0,4 ms, 5 instances OK, arrêt au silence, Statique sans redessin.
- Workflow GitHub Actions (Windows + macOS, tests, pluginval, auval), installeurs Inno Setup et DMG.

En attente :
1. Ghislain crée le dépôt GitHub vide `shake-it` → Claude pousse, GitHub compile Windows et Mac.
2. Test dans FL Studio et sur Mac M1 (son, glide, animation, 5 instances, F2).
3. Tenues : 3 poses par tenue supplémentaire (même cadrage, même échelle).
