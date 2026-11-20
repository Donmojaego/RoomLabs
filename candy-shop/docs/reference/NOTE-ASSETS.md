# Candy Shop · Room Labs Audio

Intègre trois habillages au même plugin. Ouvre sur Néon, puis rends Vert jour et Orange jour accessibles via les réglages. Garde le preset, les paramètres et l'état de jeu quand l'habillage change.

- `*_vitrine@2x.png` est le fond vide. `*_planche@2x.png` est l'aperçu de placement. Les libellés, potards, pads et sélecteurs qui figurent sur les planches sont à créer dans l'interface.
- Pose les sprites et les libellés avec les coordonnées de `manifest.json`, exprimées sur le canevas 1536 × 1024, origine en haut à gauche. Mets à l'échelle fond et calques ensemble. Préserve deux rangées de quatre emplacements.
- Les sprites carrés 1024 × 1024 sont en PNG RGBA. Chaque bonbon partage un état OFF commun. Son état ON est décliné pour les trois thèmes, avec le même cadrage. Remplace l'un par l'autre dans le même rectangle lors d'une sélection. Les trois percussions sont communes.
- Le premier emplacement reprend le libellé **Plug Seed Pack**. Les trois derniers libellés sont des propositions pour les bonbons ajoutés dans la note de production. La planche montre également une proposition de placement des contrôles.
- La V1 de la boîte à rythmes est un kit jouable au clavier MIDI, conformément à la note. Les interactions, la navigation, les animations et le moteur audio sont à intégrer côté plugin.
- Vitrines et planches 1536 × 1024. Le suffixe `@2x` suit le nommage de la note. Utilise les dimensions réelles du manifeste pour le placement.
