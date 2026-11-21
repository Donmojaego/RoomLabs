# Ma petite soupe ouselem {{VERSION}} — Windows

Instrument et effet calé sur le tempo, par Room Labs (VST3 + application autonome).

## Installer

- **Installeur** : lancer `SoupeOuselem-Setup-{{VERSION}}.exe`.
- **À la main** : copier `SoupeOuselem.vst3` dans `C:\Program Files\Common Files\VST3\`.

Dans FL Studio : Options > Manage plugins > Find installed plugins, puis Ajouter > Ma petite soupe ouselem (Generators).

## Emplacements

| Quoi | Où |
|---|---|
| Plugin | `C:\Program Files\Common Files\VST3\SoupeOuselem.vst3` |
| Application autonome | `C:\Program Files\SoupeOuselem\SoupeOuselem.exe` |
| Presets d'usine | intégrés au plugin (copie : `C:\Program Files\SoupeOuselem\Presets\factory.json`) |
| Préférences d'interface | `%APPDATA%\Room Labs\Ma petite soupe ouselem\Ma petite soupe ouselem.settings` |

## Contenu du dossier

- `SoupeOuselem.vst3`, `SoupeOuselem.exe`
- `RAPPORT-TESTS.md` : résultats des tests automatiques de cette version
- `A-FAIRE.md` : ce qui reste à vérifier

## Tester

1. Charger le plugin sur une piste et lancer la lecture de FL Studio : les effets suivent le tempo et la mesure.
2. Cliquer un sachet, jouer : la soupe tourne plus vite quand l'effet travaille, la vapeur monte avec le son.
3. Essayer REVERSE et STUTTER, puis les menus INSTRUMENT et FX (SYNC).
4. Cliquer sur le titre pour le mode « Mon petit ramen ». F2 affiche la fluidité de l'animation.
