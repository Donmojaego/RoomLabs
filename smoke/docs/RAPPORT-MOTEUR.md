# Smoke — rapport des tests du moteur

## Presets d'usine (note de référence, vélocité 0,9)

| Preset | Note | Crête (dBFS) | Silence après relâche | Résultat |
|---|---|---|---|---|
| Feutre de Minuit | 60 | -6.0 | -240 dB | OK |
| Piano Velours | 60 | -6.1 | -240 dB | OK |
| Accord m9 Feutré | 57 | -6.1 | -240 dB | OK |
| Maj9 du Dimanche | 60 | -6.5 | -240 dB | OK |
| Piano qui Dérive | 60 | -6.0 | -240 dB | OK |
| Cassette Douce | 57 | -7.0 | -240 dB | OK |
| Onze Mineur | 55 | -7.4 | -240 dB | OK |
| Neuvième Brumeuse | 60 | -8.1 | -240 dB | OK |
| Feutre Sec | 62 | -6.1 | -240 dB | OK |
| Brouillard au Piano | 60 | -5.9 | -217 dB | OK |
| Rhodes Velours | 60 | -6.4 | -240 dB | OK |
| Néon Rhodes | 60 | -7.2 | -240 dB | OK |
| Rhodes m9 | 57 | -6.0 | -240 dB | OK |
| Slow Jam | 55 | -7.9 | -240 dB | OK |
| Tine Sucré | 64 | -6.5 | -240 dB | OK |
| Rhodes Enfumé | 60 | -8.3 | -240 dB | OK |
| Neuf Doux | 58 | -8.7 | -240 dB | OK |
| Clean Rhodes | 60 | -6.1 | -240 dB | OK |
| Rhodes Lointain | 60 | -5.7 | -211 dB | OK |
| Accords de Nuit | 57 | -7.0 | -240 dB | OK |
| Nappe Prune | 60 | -6.2 | -236 dB | OK |
| Montée Lente | 60 | -6.7 | -240 dB | OK |
| Brume Mouvante | 57 | -8.7 | -184 dB | OK |
| Riser Court | 60 | -6.0 | -240 dB | OK |
| Nuage Lumineux | 64 | -6.1 | -239 dB | OK |
| Ascension | 55 | -6.0 | -206 dB | OK |
| Plafond de Fumée | 60 | -7.3 | -99 dB | OK |
| Tension Douce | 60 | -6.1 | -240 dB | OK |
| Queue Infinie | 60 | -6.0 | -54 dB | OK |
| Lever de Rideau | 62 | -6.1 | -240 dB | OK |
| Sub Rond | 36 | -6.0 | -240 dB | OK |
| Sub Feutré | 33 | -6.1 | -240 dB | OK |
| Sub Audible | 36 | -6.0 | -240 dB | OK |
| Basse Planante | 38 | -6.6 | -240 dB | OK |
| Glisse Grave | 36 | -6.2 | -240 dB | OK |
| House Profonde | 36 | -6.2 | -240 dB | OK |
| Basse Électro | 38 | -6.1 | -240 dB | OK |
| Garage Rond | 36 | -6.4 | -240 dB | OK |
| Hip-Hop Moelleux | 33 | -6.0 | -240 dB | OK |
| Basse de Fumée | 36 | -6.8 | -240 dB | OK |

## Justesse (à sec, sans dérive)

- [OK] Rhodes (note 60) : +0.0 cents
- [OK] Piano feutré (note 60) : +0.5 cents
- [OK] Nappe (note 57) : +0.0 cents
- [OK] Sub (note 36) : +0.0 cents
- [OK] Basse house (note 36) : -0.0 cents
- [OK] Riser : monte de 263 Hz à 392 Hz en 2 s (une quinte attendue, 262 → 392)

## Modules

- [OK] Space + Dream allongent la queue : -240.0 dB → -25.6 dB, 1,7 s après la relâche
- [OK] Tone fermé assombrit le son (indice de brillance 0.071 → 0.026)
- [OK] Clean à 0 compresse la forme d'onde (facteur de crête 1.91 → 1.77)
- [OK] Mod fait dériver la hauteur lentement (263.28 Hz puis 261.40 Hz, ±14 cents max)

## Jeu

- [OK] accord m9 : une touche lance 5 voix (attendu 5)
- [OK] relâcher la touche libère toutes les voix de l'accord
- [OK] Sub mono legato : 1 voix, glisse de 75 Hz vers 98 Hz (cible 98)
- [OK] Sub : relâcher la note du haut revient à la note tenue (65 Hz, attendu 65)
- [OK] 20 touches d'un coup : 12 voix au maximum (vol de voix)

## Retour visuel (fumée, braise, yeux)

- [OK] note tenue : énergie 0.94, intensité 0.95, touches tenues 1, coups 1
- [OK] après la relâche et la queue : énergie 0.000, intensité 0.000, touches 0 (la fumée se dissipe)

## CPU (48 kHz, blocs de 512)

- [OK] 12 voix de piano feutré + Space/Dream/Mod : 7.2 % d'un cœur
- [OK] 12 voix de nappe + Space/Dream/Mod : 3.3 % d'un cœur

**Résultat : tout est OK.**
