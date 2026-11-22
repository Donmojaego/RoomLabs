# KALEIDOSCOPE by Room Labs Audio
## Cahier des charges fonctionnel et artistique — version 1 pour Claude

**Statut :** design validé dans son principe ; prototype à démarrer.  
**Destinataire :** Claude, pour cadrage technique, prototype et demande précise des assets nécessaires.  
**Nom de travail :** *Kaleidoscope by Room Labs Audio*.

---

## 1. Vision du plugin

Kaleidoscope est un instrument virtuel créatif qui génère des mélodies et accompagnements MIDI dans de nombreux styles et humeurs, puis les joue avec un moteur sonore hybride. L’artiste choisit un son, un style et une manière de jouer, puis peut générer de nouvelles idées ou décliner une idée de départ.

L’identité visuelle doit être indissociable de la fonction musicale : un kaléidoscope complexe et animé se recompose en fonction de la mélodie et du style. Chaque proposition musicale doit donner naissance à une nouvelle composition visuelle.

Le plugin doit être créatif et riche, tout en restant fluide dans une session de production. La qualité sonore et l’animation centrale sont prioritaires ; les habillages de l’interface doivent rester nets en haute définition et légers à charger.

## 2. Choix de design validés

Quatre apparences sont retenues et doivent coexister dans le même plugin. **Le layout 5 rétro est l’apparence par défaut.**

1. **Layout 5 — Retro 1970s / Warm Vintage — défaut**
   - Habillage chaud, bois, laiton et accents orange/ambre.
   - Écran central encadré comme une lentille ou un hublot ; le kaléidoscope animé reste très visible.
   - Aspect d’instrument de musique vintage, commandes tactiles et lisibles.

2. **Layout 3 — Minimal Sound Laboratory / White Scientific**
   - Habillage blanc, clair et scientifique, avec repères fins inspirés des instruments optiques.
   - Conserver le kaléidoscope central en couleur vive et un contraste suffisant sur les commandes.

3. **Layout 4 — Futuristic Holographic / Spectral Tunnel**
   - Habillage sombre, futuriste, cyan, violet et lumière holographique.
   - Tunnel lumineux profond qui se transforme avec la musique.
   - Commandes modernes, avec contraste suffisant pour rester faciles à utiliser.

4. **Layout Télescope — référence visuelle dédiée**
   - Le plugin évoque un véritable tube optique de kaléidoscope, avec une vue profonde en tunnel et des miroirs colorés.
   - Grande ouverture circulaire et commandes intégrées autour du tube ; conserver une bonne lisibilité.

### Bouton unique de changement de layout

Un seul bouton visible en façade, en bas à gauche du plugin, nommé provisoirement **LAYOUT**, fait défiler les quatre apparences :

**Rétro 5 (défaut) → Labo minimal 3 → Holographique 4 → Télescope → Rétro 5.**

Le changement d’habillage ne doit pas réinitialiser le preset, la mélodie, les réglages du moteur, la tonalité ou l’état du mode Personal. Les quatre layouts pilotent le même moteur audio et le même système d’animation, sauf le mode Télescope dont l’image utilise un rendu optique miroir/tunnel dédié. Prévoir une transition visuelle douce au changement.

Pour rester compatible avec les DAW, conserver une fenêtre de plugin hébergée de dimensions rectangulaires stables ; l’interface dessinée à l’intérieur peut prendre une silhouette de tube, de cadran, de tunnel ou de panneau facetté.

## 3. Kaléidoscope animé — élément central

### Direction artistique

S’inspirer des références transmises par Ghislain : motifs radiaux très riches, symétrie en miroir, fragments multicolores, éclats brillants, profondeur optique, sensation de tunnel et compositions de type mandala. Créer une identité originale Room Labs Audio sans recopier une image de référence.

L’animation ne doit pas être un GIF ou une image fixe que l’on fait tourner. Elle doit être complexe, fluide et de haute qualité, comme un visualiseur kaléidoscopique dédié : plusieurs couches, symétrie radiale, fragments en mouvement, variations de géométrie, profondeur, lumière et transitions.

### Animation réactive à la musique

L’image doit évoluer avec le style, l’humeur et les caractéristiques de la mélodie produite. Correspondances proposées pour le prototype :

- tempo → pulsation, vitesse ou rotation ;
- densité rythmique → nombre de fragments et activité visuelle ;
- contour et intervalles mélodiques → géométrie des motifs ;
- registre → zones, taille ou couleur des fragments ;
- tension harmonique → contraste, saturation ou stabilité du motif ;
- style/humeur → palette et caractère du mouvement ;
- nouvelle mélodie → nouvelle graine visuelle et nouvelle composition.

Pour les déclinaisons, transformer progressivement le motif existant afin que l’image garde un lien avec la mélodie source. Le rendu Télescope conserve la logique de symétrie et ajoute un effet de kaléidoscope miroir et de profondeur optique. Le rendu visuel d’une mélodie peut être régénéré de façon cohérente à partir de cette mélodie, si le moteur conserve son identifiant/graine.

### Qualité et performances

- L’animation complexe du kaléidoscope sera réalisée par Claude dans JUCE. Prioriser un rendu procédural accéléré par GPU/shader pour la zone animée, plutôt que de livrer des centaines d’images HD préfabriquées.
- Garder les éléments d’interface statiques légers : formes vectorielles quand possible, textures comprimées et assets réutilisables.
- Proposer une qualité d’affichage adaptative : haute qualité et animation fluide lorsque l’interface est visible ; réduire le nombre d’images/seconde ou suspendre l’animation quand l’interface est masquée ou inactive.
- Ne jamais bloquer le fil audio pour calculer l’animation ; isoler le rendu graphique du traitement audio.
- Prévoir un réglage de qualité visuelle ou une option pour réduire les effets lourds si un ordinateur est moins puissant.
- Objectif : animation nette et fluide sur Mac mini M1 et PC Windows récents, sans clics audio, dropouts ni hausse CPU excessive. Claude doit valider les choix techniques sur les plateformes et DAW ciblés avant de promettre ce niveau.

## 4. Générateur de mélodies et MIDI

### Modes de jeu

- **Solo :** ligne mélodique monophonique, notes jouées l’une après l’autre ; utilisable comme lead, riff ou topline instrumentale.
- **Accompagnement :** accords ou voicings de plusieurs notes à la fois, adaptés au style pour soutenir la voix ou la mélodie.

### Commandes principales

- **Générer :** crée une proposition nouvelle et indépendante.
- **Déclinaison :** crée une variation liée à une mélodie de référence.
- **J’aime / Garder / À refaire :** retours explicites pouvant guider le mode Personal.
- **Export MIDI / glisser-déposer MIDI**, si le DAW et le format de plugin le permettent.

### Sources de déclinaison

Par défaut, Déclinaison repart de la première mélodie générée dans la session. L’utilisateur peut ensuite verrouiller une autre mélodie comme nouvelle référence. En mode Drop, l’audio de référence peut guider la tonalité, le tempo et certaines caractéristiques de la variation.

### Tonalité et harmonie

- **Mode Keys :** l’utilisateur choisit tonalité et gamme ; les notes générées restent dans cette gamme, sauf option explicite autorisant des notes de passage ou tensions.
- **Mode Drop :** glisser-déposer un fichier audio ; estimer tonalité/gamme et tempo, afficher les estimations et laisser l’utilisateur les corriger avant génération.
- Respecter les règles harmoniques du style sans produire une simple suite automatique d’accords clichés.

### Variété et anti-répétition

Le moteur ne se limite pas à un tirage aléatoire de notes. Il combine grammaires et choix variables : progression harmonique, contour, rythme, silences, registre, densité, motifs, répétitions musicales intentionnelles, tension et résolution.

À chaque clic, produire plusieurs candidates, comparer leurs empreintes musicales à l’historique récent, rejeter les doublons et les propositions trop proches, puis retenir une candidate à la fois originale et musicalement cohérente. Une génération reçoit une graine stable afin de pouvoir retrouver la même proposition. L’objectif est une variété quasi inépuisable à l’usage ; ne pas promettre mathématiquement une infinité de séquences distinctes dans un espace de règles fini.

## 5. Mode Personal

Personal adapte graduellement les probabilités de génération aux goûts de l’utilisateur. Utiliser surtout les retours explicites **J’aime**, **Garder**, **À refaire**, et éventuellement les mélodies exportées ou glissées dans la session. Le simple fait de laisser une mélodie affichée ne constitue pas une preuve fiable qu’elle plaît.

Le profil peut retenir des préférences de rythme, mouvement mélodique, densité, registre, accords, intervalles et tension. Prévoir une activation/désactivation claire, une remise à zéro, et privilégier un profil local si faisable. Personal doit guider les propositions sans bloquer l’exploration d’autres idées.

## 6. Styles et humeurs à couvrir

La bibliothèque doit organiser les choix en familles avec sous-styles et humeurs, pas afficher une longue liste indigeste sur l’écran principal. Recherche et filtres par style, époque, énergie et humeur recommandés.

Styles demandés : trap, R&B, rap, Detroit, UK garage, amapiano, afro love, Afrobeat, afro rumba, soul, electro, shatta, zouk, kompa, reggae, reggaeton, variété française, pop, emo, musique triste, musique joyeuse, planante, énergie club, années 70, années 80, musique classique, world, latino, bossa nova, jazz, neo soul, 2-step, techno, rock, pop rock, alternative, dubstep, drill, Afrotrap, club, Baile funk, gospel, rumba congolaise, sébène, coupé-décalé, musique orientale, musique asiatique, musique hindoue, musique de guerre, lofi, Ragga, bouyon, bombaton, electro house, minimal, dancehall, club hollandais (références de travail : Lil Kleine, Broederliefde, TopNotch), style luxe (référence : Maybach Music) et style intello (référence : Pharrell).

« Bombaton » est conservé comme terme de travail à préciser au moment de créer les presets. Les noms d’artistes servent de références d’ambiance, pas à cloner exactement le style d’un artiste.

## 7. Moteur sonore hybride

Architecture souhaitée :

- moteur **soustractif + wavetable** comme moteur principal ;
- moteur **granulaire** pour textures, nappes, transformations et couches expérimentales ;
- lecture de samples, SoundFonts et banques SFZ comme source complémentaire.

Les couches doivent pouvoir être activées/désactivées par preset et mélangées sans gaspiller le CPU. Prévoir une gestion efficace des voix, du streaming des samples et du chargement à la demande ; ne pas charger les 100 banques mémoire en même temps.

## 8. Presets, instruments et SoundFonts

- Cible de départ : environ **150 presets natifs** ; architecture qui peut en accueillir jusqu’à 200.
- Ajouter environ **100 SoundFonts, patchs SFZ ou presets issus de ces banques** sous licence compatible avec un produit commercial.
- Le total doit être compté après inventaire. 150 + 100 éléments signifie 250 entrées si une banque compte comme une entrée ; une SoundFont peut contenir plusieurs patchs, ce qui peut porter le nombre de sons à environ 300. Claude devra rapporter le compte réel par patch.
- Familles de sons : pianos, claviers, cordes, instruments orchestraux, voix, percussions, textures et instruments expérimentaux.
- Priorité à l’audio source haute fidélité ; conserver des fichiers sources et des versions distribuées optimisées, sans dégrader audiblement les presets.
- Prévoir le navigateur de presets avec catégories, favoris, recherche et aperçu/audition.

## 9. Sources d’échantillons et licences

Utiliser uniquement des échantillons dont la licence permet l’usage commercial et l’intégration/distribution dans le plugin. Conserver pour chaque son : nom du pack et du patch, auteur, URL, licence exacte, date de vérification, format, attribution éventuelle et preuve de la licence. Ne pas intégrer un son simplement parce qu’il est décrit comme « gratuit » ou « libre de droits ».

Sources à examiner :

- **VCSL (Versilian Community Sample Library)** — CC0 ; utilisable en logiciel commercial selon la page officielle ; contient notamment des patchs SFZ.
- **VSCO 2 Community Edition** — CC0. Utiliser l’édition Community Edition, pas les éditions commerciales Standard/Professional sans licence appropriée.
- **Karoryfer Samples : exclu de ce projet** à la demande de Ghislain, car il ne veut pas intégrer cette source sans certitude suffisante sur la licence CC0 de chaque banque.
- **Meta Open Sound** — Meta Piano, Meta Nylon Guitar et Meta Steel Guitar annoncés CC0 ; vérifier chaque sous-collection.
- **FreePats** — plusieurs banques SF2/SFZ sont annoncées CC0 ; contrôler chaque instrument/paquet.
- **Freesound** — vérifier chaque fichier ; sélectionner uniquement ceux dont la page indique clairement CC0, et archiver la preuve.
- **Ne pas utiliser Philharmonia Orchestra** pour cette version du plugin.

Sources officielles de départ : [VCSL](https://versilian-studios.com/vcsl/), [VSCO 2 CE](https://versilian-studios.com/vsco-community/), [Meta Open Sound](https://github.com/facebookincubator/Meta-open-sound), [FreePats](https://freepats.zenvoid.org/), [licences Freesound](https://freesound.org/help/faq/).

## 10. Interface générale

L’interface doit proposer, dans une hiérarchie claire :

- choix style/humeur ;
- preset sonore ;
- Solo / Accompagnement ;
- Keys / Drop et tonalité détectée/choisie ;
- grand kaléidoscope réactif ;
- boutons Générer et Déclinaison ;
- mode Personal et retours J’aime/Garder/À refaire ;
- sections et commandes des moteurs wavetable/soustractif/granulaire/samples ;
- bouton unique LAYOUT pour cycler entre les trois designs.

Le bouton LAYOUT doit être identifiable sans prendre la place du générateur. Les commandes conservent leur état pendant le changement de design.

## 11. Compatibilité et architecture à proposer

Cible : un instrument VST3 pour Windows et macOS, avec support AU sur macOS si la technologie retenue le permet. Claude doit confirmer le format, le framework, la sortie MIDI et la compatibilité avec les DAW visés avant de figer l’architecture. La sortie MIDI d’instruments/plugins dépend des capacités et du routage de chaque DAW ; prévoir un plan de validation de l’envoi MIDI dans les hôtes ciblés et un export MIDI de secours si nécessaire.

Le traitement audio temps réel, la génération MIDI, le rendu graphique et le chargement des banques doivent être séparés proprement afin que l’animation ne provoque pas de coupure audio.

## 12. Première étape demandée à Claude

Commencer à travailler pendant la préparation des assets. Première réponse attendue :

1. **Architecture technique proposée** pour VST3, sortie MIDI, moteur hybride, analyse Drop et kaléidoscope animé ; identifier les points risqués et proposer une validation progressive.
2. **Liste exacte des assets nécessaires**, classée par priorité, avec pour chacun : nom de fichier, format, dimensions/résolution, transparence ou non, état statique/animé, layout concerné et emplacement d’usage.
3. Distinguer les éléments qui doivent être fournis en image des éléments qui peuvent être dessinés ou générés par code/shader. Le kaléidoscope animé doit rester principalement procédural afin d’obtenir beaucoup de variations sans embarquer une énorme séquence d’images.
4. Créer le squelette du projet et un premier prototype de l’interface, avec le bouton unique de changement de layout et une preuve de concept du kaléidoscope qui réagit à une mélodie générée.
5. Signaler les banques candidates à examiner et produire un inventaire de licences/patchs, sans supposer qu’une bibliothèque entière est CC0 si seules certaines collections le sont.
6. Ne pas demander un pack d’assets générique : demander seulement les fichiers précis après avoir défini leur rôle, leurs dimensions et leur format.

## 13. Critères de validation de la première version

- Un clic sur Générer fournit une nouvelle proposition cohérente et met à jour l’animation.
- Solo et Accompagnement produisent bien des types de MIDI différents.
- Keys respecte la gamme ; Drop affiche et permet de corriger tonalité/tempo détectés.
- Déclinaison reste liée à la mélodie de référence ; la première mélodie est la référence initiale.
- Les quatre layouts sont accessibles avec un seul bouton en façade, en bas à gauche ; le rétro est celui qui apparaît au lancement et le changement ne réinitialise aucun réglage.
- Le kaléidoscope montre une animation multicouche et réactive ; il reste fluide et n’interrompt pas le son.
- Le profil Personal peut recevoir des retours explicites et être réinitialisé.
- Le navigateur montre le nombre réel de presets/patchs et les banques chargées sans saturer la mémoire.
- Les sons distribués ont une licence documentée et compatible avec la vente du plugin.
- Le volume général au lancement est modéré, mais reste assez présent et se règle sur une large plage par le contrôle MASTER. Ne pas réduire excessivement les graves ni écraser la dynamique ; permettre à l’utilisateur de monter le niveau selon son besoin sans écrêtage numérique.

## 14. Production des assets PNG

- Créer chaque asset progressivement, un PNG à la fois, puis regrouper les livrables finaux dans un ZIP par plugin.
- Cibles de taille demandées : panneau complet avec contour et détails autour de **2000 × 2166 px** ; planches de boutons et groupes d’éléments autour de **1060 × 1900 px**.
- Une planche peut regrouper au maximum **quatre éléments/boutons**. Ne jamais entasser plus de quatre boutons sur un PNG.
- Les éléments visuels principaux (châssis, lentille, pièces décoratives majeures) doivent aussi exister en PNG séparés avec fond transparent quand ils doivent se superposer à l’interface.
- Prévoir des groupes cohérents de boutons pour chacun des quatre layouts : rétro 5, labo minimal 3, holographique 4 et télescope. Le bouton LAYOUT est placé en bas à gauche de la façade.
- Les libellés et le texte fonctionnel des boutons doivent pouvoir être rendus proprement par le code ; éviter de dépendre d’un texte généré dans l’image si cela nuit à la lisibilité.
- Garder une haute définition visuelle sans poids excessif : transparence propre, textures réutilisées, compression adaptée et animation procédurale légère.
- L’inventaire final de ce lot est fourni en section 15 : nom de fichier, layout, rôle, dimensions, nombre d’éléments et transparence. Continuer ensuite la production asset par asset, puis livrer tous les PNG avec ce cahier des charges dans le ZIP final.

**Direction validée par Ghislain :** le concept Kaleidoscope, la visualisation kaléidoscopique réactive, l’architecture hybride, le rétro 5 par défaut, ainsi que les layouts labo minimal 3, holographique 4 et télescope. Un seul bouton en bas à gauche fait défiler les quatre apparences. Les assets sont à produire asset par asset puis à rassembler dans un ZIP final.


## 15. Inventaire final des assets PNG

Les 27 fichiers ci-dessous sont des PNG avec canal alpha. Les panneaux et cadres optiques sont en 2000 × 2166 px ; les planches de contrôles sont en 1060 × 1900 px. Les planches de contrôles regroupent quatre éléments au maximum.

| Fichier | Layout | Rôle | Dimensions | Éléments | Fond transparent |
|---|---|---|---:|---:|---|
| `KL_Holo04_Boutons_Modes_01-04.png` | Holographique_04 | Commandes Solo/Accomp, Keys/Drop et modes | 1060x1900 | 4 max. | Oui (alpha) |
| `KL_Holo04_Boutons_Principaux_01-04.png` | Holographique_04 | Commandes principales, dont génération et cycle de layout | 1060x1900 | 4 max. | Oui (alpha) |
| `KL_Holo04_Boutons_Rotatifs_01-04.png` | Holographique_04 | Boutons rotatifs de variation et paramètres | 1060x1900 | 4 max. | Oui (alpha) |
| `KL_Holo04_Cadre_Lentille_Optique.png` | Holographique_04 | Cadre optique / lentille séparée | 2000x2166 | 1 | Oui (alpha) |
| `KL_Holo04_Chassis.png` | Holographique_04 | Châssis / panneau principal | 2000x2166 | 1 | Oui (alpha) |
| `KL_Holo04_Selecteurs_Preset_Style_Humeur_Master_01-04.png` | Holographique_04 | Sélecteurs preset, style, humeur et master | 1060x1900 | 4 max. | Oui (alpha) |
| `KL_Holo04_Toggles_Moteurs_01-04.png` | Holographique_04 | Interrupteurs des moteurs sonores | 1060x1900 | 4 max. | Oui (alpha) |
| `KL_Labo03_Boutons_Modes_01-04.png` | Labo_Minimal_03 | Commandes Solo/Accomp, Keys/Drop et modes | 1060x1900 | 4 max. | Oui (alpha) |
| `KL_Labo03_Boutons_Principaux_01-04.png` | Labo_Minimal_03 | Commandes principales, dont génération et cycle de layout | 1060x1900 | 4 max. | Oui (alpha) |
| `KL_Labo03_Boutons_Rotatifs_01-04.png` | Labo_Minimal_03 | Boutons rotatifs de variation et paramètres | 1060x1900 | 4 max. | Oui (alpha) |
| `KL_Labo03_Chassis_Complet.png` | Labo_Minimal_03 | Châssis / panneau principal | 2000x2166 | 1 | Oui (alpha) |
| `KL_Labo03_Selecteurs_Preset_Style_Humeur_Master_01-04.png` | Labo_Minimal_03 | Sélecteurs preset, style, humeur et master | 1060x1900 | 4 max. | Oui (alpha) |
| `KL_Labo03_Toggles_Moteurs_01-04.png` | Labo_Minimal_03 | Interrupteurs des moteurs sonores | 1060x1900 | 4 max. | Oui (alpha) |
| `KL_Retro05_Boutons_Modes_01-04.png` | Retro_05 | Commandes Solo/Accomp, Keys/Drop et modes | 1060x1900 | 4 max. | Oui (alpha) |
| `KL_Retro05_Boutons_Principaux_01-04.png` | Retro_05 | Commandes principales, dont génération et cycle de layout | 1060x1900 | 4 max. | Oui (alpha) |
| `KL_Retro05_Boutons_Rotatifs_01-04.png` | Retro_05 | Boutons rotatifs de variation et paramètres | 1060x1900 | 4 max. | Oui (alpha) |
| `KL_Retro05_Cadre_Lentille_Centrale.png` | Retro_05 | Cadre optique / lentille séparée | 2000x2166 | 1 | Oui (alpha) |
| `KL_Retro05_Chassis_Complet.png` | Retro_05 | Châssis / panneau principal | 2000x2166 | 1 | Oui (alpha) |
| `KL_Retro05_Selecteurs_Preset_Style_Humeur_Master_01-04.png` | Retro_05 | Sélecteurs preset, style, humeur et master | 1060x1900 | 4 max. | Oui (alpha) |
| `KL_Retro05_Toggles_Moteurs_01-04.png` | Retro_05 | Interrupteurs des moteurs sonores | 1060x1900 | 4 max. | Oui (alpha) |
| `KL_Telescope_Boutons_Modes_01-04.png` | Telescope | Commandes Solo/Accomp, Keys/Drop et modes | 1060x1900 | 4 max. | Oui (alpha) |
| `KL_Telescope_Boutons_Principaux_01-04.png` | Telescope | Commandes principales, dont génération et cycle de layout | 1060x1900 | 4 max. | Oui (alpha) |
| `KL_Telescope_Boutons_Rotatifs_01-04.png` | Telescope | Boutons rotatifs de variation et paramètres | 1060x1900 | 4 max. | Oui (alpha) |
| `KL_Telescope_Cadre_Lentille_Optique.png` | Telescope | Cadre optique / lentille séparée | 2000x2166 | 1 | Oui (alpha) |
| `KL_Telescope_Chassis.png` | Telescope | Châssis / panneau principal | 2000x2166 | 1 | Oui (alpha) |
| `KL_Telescope_Selecteurs_Preset_Style_Humeur_Master_01-04.png` | Telescope | Sélecteurs preset, style, humeur et master | 1060x1900 | 4 max. | Oui (alpha) |
| `KL_Telescope_Toggles_Moteurs_01-04.png` | Telescope | Interrupteurs des moteurs sonores | 1060x1900 | 4 max. | Oui (alpha) |

**Ordre d’intégration conseillé :** utiliser le châssis comme base, placer l’animation procédurale dans l’ouverture du visualiseur, puis superposer le cadre optique et les commandes séparées. Les textes fonctionnels, valeurs et libellés définitifs sont à rendre dans JUCE afin qu’ils restent nets, localisables et accessibles.
