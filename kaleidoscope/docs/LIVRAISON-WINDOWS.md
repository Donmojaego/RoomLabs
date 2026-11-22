# Kaleidoscope {{VERSION}} — Windows

Générateur de mélodies infinies par Room Labs Audio (VST3 + application autonome) : 59 styles, 10 humeurs, 20 gammes,
Solo ou Accompagnement, Keys ou Drop, Déclinaison, Personal, mode Infini, export MIDI.
Son : moteur hybride (wavetable + soustractif, granulaire, instruments enregistrés). 155 presets de synthèse + 125 instruments enregistrés = 280 sons.
Interface : les 4 habillages peints de Room Labs (Rétro par défaut, Labo, Holographique, Télescope), MASTER de -36 à +12 dB.

## Installer

Un seul fichier : `Kaleidoscope-Setup-{{VERSION}}.exe`. Il installe le plugin, l'application autonome, les 280 sons
(dont les 125 instruments enregistrés), les gabarits et la documentation. Rien d'autre à télécharger.

Dans FL Studio : Options > Manage plugins > Find installed plugins, puis Ajouter > Kaleidoscope (Generators).
Pour récupérer les notes dans une piste : le plugin envoie aussi les notes en MIDI (réglage ⚙ « Envoyer les notes en MIDI »),
ou glissez le mini-rouleau sous le kaléidoscope vers une piste, ou bouton « MIDI ».

## Emplacements

| Quoi | Où |
|---|---|
| Plugin | `C:\Program Files\Common Files\VST3\Kaleidoscope.vst3` |
| Application autonome | `C:\Program Files\Kaleidoscope\Kaleidoscope.exe` |
| Instruments enregistrés | `C:\ProgramData\Room Labs\Kaleidoscope\Banques\` |
| Gabarits | `C:\Program Files\Kaleidoscope\Documentation\Gabarits\` |
| Mélodies exportées | `Documents\Room Labs\Kaleidoscope\MIDI\` |
| Profil Personal | `%APPDATA%\Room Labs\Kaleidoscope\personal.txt` |
| Essai d'assets de layout | `Documents\Room Labs\Kaleidoscope\Assets\` (ex. `retro_chassis.webp`), sans recompiler |

## Tester

1. GÉNÉRER (ou Espace) : une mélodie dans le style choisi, la lecture démarre ; le kaléidoscope réagit à chaque note.
2. Changer Style, Humeur, Tonique, Gamme : une nouvelle proposition arrive aussitôt.
3. DÉCLINAISON (touche D) : variante de la mélodie de référence (la 1re de la session ; clic droit ou bouton Référence pour en choisir une autre). RETOUR : la précédente.
4. J'aime / Garder / À refaire : Personal apprend vos goûts (curseur Personal = son poids). ↺ remet à zéro.
5. Drop : déposer un son (wav, mp3, flac) ; la tonalité et le tempo sont détectés, la mélodie se construit autour.
6. Infini : la mélodie évolue toute seule pendant la lecture.
7. LAYOUT (en bas à gauche) : Rétro → Labo → Holographique → Télescope. Clic droit : la liste. Rien d'autre ne change.
8. Navigateur : ▶ pour écouter un preset, ★ pour le mettre en favori.
9. MASTER : volume de sortie modéré au départ (-4 dB), montez jusqu'à +12 dB sans écrêtage.

## Gabarits

Le dossier `Gabarits` montre, pour chaque layout, où va chaque élément peint (2000 × 2166 px).
