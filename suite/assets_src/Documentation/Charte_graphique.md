# Room Labs Audio — Charte graphique

Vert est le thème par défaut. Noir graphite, émeraude ou bleu-vert, signal vert lumineux et contraste élevé. Studio utilise du graphite, du métal brossé et des marquages monochromes. Pocket utilise une coque claire, des écrans sombres et des commandes noir et blanc. Un bouton de thème stable dans l’en-tête change l’apparence en conservant les réglages et le son.

Les formes suivent l’usage. Noise est compact et arrondi. Stem Splitter a une façade capsule pour ses deux pistes. 7th Surgical Labs a des angles biseautés. Noise Tools est carré compact. Reverb a un sommet arrondi. Loop Creator suit la forme d’une groovebox. Loop Remix Lab est asymétrique. Audio to MIDI utilise une façade large. Le standalone garde une navigation commune avec les icônes incluses. Le Rack garde un châssis horizontal sombre et des accents différents pour les modules.

Le relief vient de la lumière diffusée depuis le haut gauche, du métal brossé, des biseaux et des reflets des molettes. Assembler fond, contour puis cadres. Les cotes de calques sont dans Inventaire_PNG.csv. Le contour et le fond partagent leurs coordonnées d’origine. Les feuilles de commandes sont des sprites à découper selon leurs cases. Les LEDs n’entrent pas dans le nombre de boutons. Les pieds de molettes et les bagues sont dessinés sur les sprites, alors que les indicateurs d’angle, états, étiquettes et zones de clic sont rendus dans l’interface. Textes, courbes, vumètres et piano rolls restent dynamiques. Réutiliser la même peau de commandes et la même grille de navigation dans les écrans du standalone.

## Format et intégration

PNG RGBA 1020 × 1200, interprétation du format dicté. Les faces utiles sont cadrées dans des zones adaptées au produit. Recadrer selon l’inventaire en préservant le ratio. Les feuilles de boutons, molettes et pads ont quatre éléments au maximum et ne contiennent aucune étiquette. Les panneaux et contours sont séparés. Le dossier References contient les planches approuvées servant à régler la finition et les proportions. Pour les écrans à haute densité, générer des rendus 2× à partir des masters ou redessiner les primitives nécessaires, puis contrôler au zoom natif.

## Recette graphique

Commencer par Noise Tools. Construire une composition cliquable avec les trois traitements sur les trois thèmes. Comparer la capture à l’échelle de travail dans les DAW cibles. Mesurer tailles, alignements et zones cliquables, vérifier contraste et rendre correctement les états focus, survol et désactivé. Les planches sont des références d’art, le pack PNG fournit des calques de départ. Vérifier la correspondance et affiner les finitions avant de déclarer les compositions définitives.

## Découpe des feuilles de commandes

Les coordonnées partent du coin supérieur gauche, en pixels à la résolution native du PNG. Utiliser les centres des molettes comme pivots de rotation du pointeur.

| Feuille | Éléments et ordre de gauche à droite |
|---|---|
| Molettes | Quatre centres (130,220), (380,220), (630,220), (880,220). Rayons extérieurs 80, 71, 62, 53 px. Prévoir 14 px de marge au recadrage pour l’ombre. |
| Touches | Les coins supérieurs gauches sont (60,160), (300,160), (540,160) et (780,160). Les pièces occupent chacune 175 × 87 px avec l’ombre. Les pièces claires avec voyant illustrent un état actif. |
| LEDs | Les coins supérieurs gauches sont (120,120), (360,120), (600,120), (840,120). Chaque pièce occupe 33 × 33 px. |
| Pads | Les coins supérieurs gauches sont (86,160), (306,160), (526,160), (746,160). Chaque pièce occupe 181 × 161 px. |
