# Windup Dogs {{VERSION}} — Windows

Instruments jouets mécaniques par Room Labs Audio (VST3 + application autonome) : boîte à musique, piano jouet, clochettes, fanfare, tambour.

## Installer

- **Installeur** : lancer `WindupDogs-Setup-{{VERSION}}.exe`.
- **À la main** : copier `WindupDogs.vst3` dans `C:\Program Files\Common Files\VST3\`.

Dans FL Studio : Options > Manage plugins > Find installed plugins, puis Ajouter > Windup Dogs (Generators).

## Emplacements

| Quoi | Où |
|---|---|
| Plugin | `C:\Program Files\Common Files\VST3\WindupDogs.vst3` |
| Application autonome | `C:\Program Files\WindupDogs\WindupDogs.exe` |
| Presets d'usine | intégrés au plugin (copie : `C:\Program Files\WindupDogs\Presets\factory.json`) |
| Préférences d'interface | `%APPDATA%\Room Labs\Windup Dogs\Windup Dogs.settings` |

## Contenu du dossier

- `WindupDogs.vst3`, `WindupDogs.exe`
- `RAPPORT-TESTS.md` : résultats des tests automatiques de cette version
- `A-FAIRE.md` : ce qui reste à vérifier

## Tester

1. Choisir un musicien à gauche (Boîte, Piano, Cloches, Fanfare, Tambour) et jouer : les engrenages tournent, le cylindre défile, le marteau frappe, les tuyaux s'allument.
2. Jouer longtemps : le ressort se détend, le son baisse un peu. Cliquer la clé (en haut), le bouton rouge ou le ressort pour remonter.
3. Bouton crème : tic-tac au tempo (lancer la lecture dans FL Studio). Cliquer « 1/8 » sur la plaque pour changer la division.
4. Médaillon en bas à droite : réglages (résonance, attaque, couleur, swing, durée du ressort, animation, taille). F2 : fluidité de l'animation.
