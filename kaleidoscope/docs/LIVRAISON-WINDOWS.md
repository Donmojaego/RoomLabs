# Kaleidoscope {{VERSION}} — Windows

Générateur de mélodies infinies par Room Labs Audio (VST3 + application autonome) : 59 styles, 10 humeurs, 20 gammes,
Solo ou Accompagnement, Keys ou Drop, Déclinaison, Personal, mode Infini, export MIDI.
Son : moteur hybride (wavetable + soustractif, granulaire, fonds échantillonnés). 150 presets natifs + 100 fonds.

## Installer

- **Plugin** : lancer `Kaleidoscope-Setup-{{VERSION}}.exe`.
- **Fonds (100 instruments)** : lancer `Kaleidoscope-Fonds-Setup-0.1.0.exe` (installeur séparé, publié dans les Releases « kaleidoscope-fonds »).
  Sans lui, les 150 presets natifs marchent ; les 100 fonds s'affichent « à installer ».

Dans FL Studio : Options > Manage plugins > Find installed plugins, puis Ajouter > Kaleidoscope (Generators).
Pour récupérer les notes dans une piste : le plugin envoie aussi les notes en MIDI (réglage ⚙ « Envoyer les notes en MIDI »),
ou glissez le mini-rouleau sous le kaléidoscope vers une piste, ou bouton « Export MIDI ».

## Emplacements

| Quoi | Où |
|---|---|
| Plugin | `C:\Program Files\Common Files\VST3\Kaleidoscope.vst3` |
| Application autonome | `C:\Program Files\Kaleidoscope\Kaleidoscope.exe` |
| Fonds | `C:\ProgramData\Room Labs\Kaleidoscope\Banques\` (ou `Documents\Room Labs\Kaleidoscope\Banques\`) |
| Mélodies exportées | `Documents\Room Labs\Kaleidoscope\MIDI\` |
| Profil Personal | `%APPDATA%\Room Labs\Kaleidoscope\personal.txt` |
| Essai d'assets de layout | `Documents\Room Labs\Kaleidoscope\Assets\` (ex. `retro_fond.webp`), sans recompiler |

## Tester

1. GÉNÉRER (ou Espace) : une mélodie dans le style choisi, la lecture démarre ; le kaléidoscope réagit à chaque note.
2. Changer Style, Humeur, Tonique, Gamme : une nouvelle proposition arrive aussitôt.
3. DÉCLINAISON (touche D) : variante de la même idée. RETOUR : la précédente.
4. J'aime / Garder / À refaire : Personal apprend vos goûts (curseur Personal = son poids). ↺ remet à zéro.
5. Drop : déposer un son (wav, mp3, flac) ; la tonalité et le tempo sont détectés, la mélodie se construit autour.
6. Infini : la mélodie évolue toute seule pendant la lecture.
7. LAYOUT : Rétro → Télescope → Holographique → Laiton & Verre. Clic droit : la liste. Rien d'autre ne change.
