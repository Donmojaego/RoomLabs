# Candy Shop {{VERSION}} — Windows

Instrument pour les hooks, le club et la fête, par Room Labs (VST3 + application autonome).

## Installer

- **Installeur** : lancer `CandyShop-Setup-{{VERSION}}.exe`.
- **À la main** : copier `CandyShop.vst3` dans `C:\Program Files\Common Files\VST3\`.

Dans FL Studio : Options > Manage plugins > Find installed plugins, puis Ajouter > Candy Shop (Generators).

## Emplacements

| Quoi | Où |
|---|---|
| Plugin | `C:\Program Files\Common Files\VST3\CandyShop.vst3` |
| Application autonome | `C:\Program Files\CandyShop\CandyShop.exe` |
| Presets d'usine | intégrés au plugin (copie : `C:\Program Files\CandyShop\Presets\factory.json`) |
| Préférences d'interface | `%APPDATA%\Room Labs\Candy Shop\Candy Shop.settings` |

## Contenu du dossier

- `CandyShop.vst3`, `CandyShop.exe`
- `RAPPORT-TESTS.md` : résultats des tests automatiques de cette version
- `A-FAIRE.md` : ce qui reste à vérifier

## Tester

1. Charger Candy Shop sur une piste, cliquer sur un bonbon, puis sur un des pads du bas pour choisir un preset.
2. Jouer la note de référence affichée dans l'écran (ex. C4) : le bonbon choisi s'illumine.
3. Percussions : touches C0 à B0 (violettes sur le clavier de l'interface) ou canal MIDI 10. Les pads « Kit » changent le kit.
4. « Lacets acidulés » : arpèges calés sur le tempo de FL Studio. « Carrés fruités » : un accord par touche.
5. Le bouton ⚙ ouvre les réglages avancés (moteur, accords, arpège, kit, affichage). F2 affiche la fluidité de l'animation.
