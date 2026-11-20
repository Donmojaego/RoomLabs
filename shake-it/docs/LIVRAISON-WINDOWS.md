# Shake It {{VERSION}} — Windows

Instrument 808 et basses par Room Labs (VST3 + application autonome).

## Installer

- **Installeur** : lancer `ShakeIt-Setup-{{VERSION}}.exe`.
- **À la main** : copier `ShakeIt.vst3` dans `C:\Program Files\Common Files\VST3\`.

Dans FL Studio : Options > Manage plugins > Find installed plugins, puis Ajouter > Shake It (Generators).

## Emplacements

| Quoi | Où |
|---|---|
| Plugin | `C:\Program Files\Common Files\VST3\ShakeIt.vst3` |
| Application autonome | `C:\Program Files\ShakeIt\ShakeIt.exe` |
| Presets d'usine | intégrés au plugin (copie : `C:\Program Files\ShakeIt\Presets\factory.json`) |
| Préférences d'interface | `%APPDATA%\Room Labs\Shake It\Shake It.settings` |

## Contenu du dossier

- `ShakeIt.vst3`, `ShakeIt.exe`
- `RAPPORT-TESTS.md` : résultats des tests automatiques de cette version
- `A-FAIRE.md` : ce qui reste à vérifier

## Tester

1. Charger Shake It sur une piste, jouer la note de référence affichée en haut (ex. F1).
2. Jouer deux notes liées : la seconde glisse (Glide), sans nouvelle attaque.
3. Touche **F2** dans l'interface : affiche la cadence de l'animation et le temps de rendu.
4. Ouvrir 5 instances et vérifier que le son ne décroche pas (essayer aussi le mode Réduit).
