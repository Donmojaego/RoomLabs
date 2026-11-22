# Room Labs Audio — Cahier des charges de production

## Mission et ordre de livraison

Construis la suite par unités vérifiables. Commence par Noise Tools comme prototype esthétique et audio. Intègre ensuite Noise, Reverb, Loop Creator, Loop Remix Lab, Audio to MIDI, 7th Surgical Labs et Stem Splitter, puis assemble l’application standalone et le 7 Room Rack. Consigne les décisions, dépendances, licences, limitations et résultats dans chaque livraison. Compare les parcours au code et aux fonctionnalités web existants quand ils sont disponibles. La cible est la qualité professionnelle native. Fais approuver toute réduction de périmètre ou modification d’un parcours validé.

## Les huit plugins

| Plugin | Type | Fonction et validation spécifique |
|---|---|---|
| Noise Tools | Effet VST3 | Denoise pour souffle et ronflement 50/60 Hz, De-click pour clics et craquements, De-clip pour reconstruire autant que possible les crêtes écrêtées. Trois traitements indépendants, bypass global, A/B à niveau comparable. Tester chaque traitement seul et combiné. Utiliser des prises propres puis dégradées de manière contrôlée, et des prises réelles. Vérifier sibilantes, consonnes, transitoires, souffle résiduel et artefacts de reconstruction. Mesurer la latence. |
| Noise | Générateur VST3 | Bruit blanc, rose ou brun, couleur et niveau, sortie initialement muette, activation sans clic. Vérifier pente spectrale, stabilité, comportement mono/stéréo et niveaux sur une capture suffisamment longue. |
| Stem Splitter | VST3 de traitement de fichiers | Import ou capture, séparation locale voix/instrumental, préécoute et export de pistes alignées. Valider précision, fuites, artefacts, temps de rendu et mémoire sur plusieurs styles. Documenter la licence et la redistribution des poids. Confirmer la faisabilité avec un prototype avant d’annoncer la sortie. |
| 7th Surgical Labs | Effet VST3 et standalone | Chaîne réordonnable jusqu’à quatre blocs de restauration, exemple De-click, Denoise, EQ, Loudness. A/B et traitement par lots dans le standalone. Documenter les blocs et le comportement d’une permutation. Vérifier cohérence des moteurs entre formats et reprise après un échec d’export. |
| Reverb | Effet VST3 | Pièce, salle et plaque, paramètres adaptés, mix et écoute en insert. Vérifier impulse, décroissance, stabilité et queues pendant bypass, changement de preset et rendu hors ligne. Évaluer séparément la réduction de réverbération et valider sa faisabilité avant intégration. |
| Loop Creator | Instrument VST3 | Séquenceur synchronisé au DAW, batterie, basse, accords et mélodie. Vérifier la précision du déclenchement, les transitions de tempo, les boucles, le rappel de presets et le comportement du transport. |
| Loop Remix Lab | Instrument VST3 | Import ou capture de boucle, création et écoute de trois variations, rappel des résultats dans la session. Vérifier raccords, tempo, longueur, transitoires et reproductibilité avec le même état de projet. |
| Audio to MIDI | Instrument VST3 et standalone | Glisser un fichier audio, détecter les notes, afficher le piano roll éditable et récupérer le fichier MIDI par glisser-déposer ou export. Tester hauteur, octave, attaques, durées et notes manquantes avec des mélodies de référence. Mesurer précision et rappel et vérifier l’import dans les DAW cibles. |

Mastering Pro, 7th Mix Lab, Musique IA et les analyseurs encore à confirmer ne font pas partie de cette livraison.

## Standalone — six outils dans une application

| Outil | Parcours et validation |
|---|---|
| Loop Synchro | Importer, vérifier BPM, mesure et longueur, recadrer, renseigner les métadonnées de synchronisation et exporter WAV. Time-stretch optionnel. Exemple CI à 96 BPM, quatre mesures de 4/4 durent 10 s. À 104 BPM les quatre mêmes mesures durent environ 9,231 s si le time-stretch est appliqué. Test d’import et de suivi du tempo sur les DAW cibles. |
| OneShot | Détecter les transitoires d’un lot de drum loops, corriger et préécouter les découpes, classer les hits, exporter ZIP et rapport. Vérifier kick, snare, clap, hi-hat, tom et percussions, fades si utiles et absence de découpe destructive. Cible à confirmer jusqu’à 100 boucles par lot. |
| Sample Rate Converter | Rééchantillonner les lots de WAV vers 44,1, 48, 88,2, 96 ou 192 kHz. ZIP et rapport. Vérifier fréquence réelle, durée à l’arrondi de l’échantillon près, canaux, aliasing et comportement aux niveaux élevés. Cible à confirmer jusqu’à 150 fichiers. |
| Chord Finder | Détecter les accords sur une timeline éditable avec écoute. Évaluer des progressions annotées, avec inversions, accords mineurs et passages ambigus. |
| Audio to MIDI | Employer le moteur du plugin et rendre disponible le même export depuis l’application locale. |
| 7th Surgical Labs | Restaurer et comparer avant/après, traiter jusqu’à 50 fichiers par lot comme cible fonctionnelle. Conserver les originaux et distinguer les erreurs fichier par fichier. |

## 7 Room Rack

Châssis sombre à modules d’accents variés. Charger les huit modules Room Labs lorsque leurs moteurs respectifs sont disponibles. Intégrer les mêmes moteurs que les VST3 individuels pour éviter des différences de rendu et gérer les traitements longs sur une tâche hors audio temps réel. Autoriser ajout, réordonnancement, bypass et sauvegarde des chaînes et presets. Tester automation, compensation de latence, MIDI et audio selon le type du slot. Prévoir le rappel d’un preset lorsqu’un module manque et afficher son nom sans casser la session.

## Critères audio et recette

- Établir un corpus documenté et reproductible avant le réglage des algorithmes. Ajouter des voix chantées et parlées, des percussions, des mix denses, des prises bruitées et des tests synthétiques. Comparer des sorties alignées et à niveau comparable, au casque et sur moniteurs.
- Vérifier à 44,1, 48 et 96 kHz, avec blocs courants de 64, 128, 256, 512 et 1024 échantillons. Tester mono/stéréo selon le plugin, silence, impulsion, sinus, bruit et fichiers longs. Observer CPU, mémoire, stabilité, rendu hors ligne et reprise du transport. Mesurer et déclarer latence et queues d’effets.
- Le traitement temps réel évite allocations, accès disque, opérations réseau et attente de verrous dans le thread audio. Les traitements lourds disposent d’un flux d’analyse ou de rendu avec progression, annulation et message d’échec. La lecture d’un projet doit rester stable si un fichier source manque.
- Contrôler absence de valeurs NaN ou infinies, clics introduits par les transitions, écrêtage involontaire, inversion de canaux et variations de gain inattendues. Mesurer selon la fonction, notamment true peak, bruit et artefacts, précision des notes, alignement des transitoires ou aliasing. Versionner corpus, paramètres, méthode de mesure et seuils propres à chaque moteur. Présenter résultats et écarts avant validation.
- Tester dans des DAW de production Windows et macOS sur machines cibles. Couvrir installation propre, scan, ouverture, automation, rappel de preset, réouverture de session, rendu, mise à niveau et désinstallation. Faire une écoute comparative de chaque fonction avant d’apposer la mention « qualité professionnelle ».

## Deux installateurs par plateforme et par livraison

| Produit | Windows 10/11 x64 | macOS Apple Silicon |
|---|---|---|
| Suite VST | RoomLabsAudio_Plugins_<version>_Win_x64.exe | RoomLabsAudio_Plugins_<version>_macOS_arm64.dmg |
| Standalone | RoomLabsAudio_Standalone_<version>_Win_x64.exe | RoomLabsAudio_Standalone_<version>_macOS_arm64.dmg |

La suite installe les huit VST3 disponibles et le 7 Room Rack lorsqu’il est livré. L’application standalone installe les six outils en une fois. Le DMG VST peut contenir un PKG qui place automatiquement les bundles. Le DMG standalone peut contenir un PKG ou l’application. Produire ces quatre artefacts pour toute compilation livrée de la suite complète, depuis le même commit et avec numéros de version cohérents. Pour les livraisons progressives, les deux paquets de la suite contiennent les VST réellement disponibles et indiquent les modules restants. Documenter signatures et notarisation et tester sur une installation propre. Le programme d’installation et de mise à niveau doit conserver presets, projets et données utilisateur.

Maintenir un manifeste versionné de modules avec identifiant stable, produit, plateformes, formats, chemin, version, dépendances et contrôles d’intégrité. Ajouter un VST par le manifeste et le pipeline de build sans réécrire l’installateur. L’installation de cette phase fonctionne hors ligne après téléchargement. Room Central et la sélection de téléchargements par module constituent une phase ultérieure.

## Direction pour Claude

Lire la charte et l’inventaire avant de coder. Présenter l’implémentation Noise Tools dans le DAW et un rapport comparant les trois thèmes aux références. Itérer sur les écarts visuels et audio. Implémenter chaque plugin et chaque écran avec les états et contrôles réels. Ne déduire aucun réglage DSP des chiffres décoratifs des maquettes.
