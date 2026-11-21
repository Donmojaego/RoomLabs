# Windup Dogs — rapport des tests du moteur

## Presets d'usine (note de référence, vélocité 0,9)

| Preset | Note | Crête (dBFS) | Silence après relâche | Résultat |
|---|---|---|---|---|
| Berceuse Remontée | 72 | -9.4 | -240 dB | OK |
| Tiroir de Grand-Mère | 72 | -9.5 | -240 dB | OK |
| Ballerine | 76 | -7.6 | -240 dB | OK |
| Peigne d'Acier | 72 | -9.7 | -240 dB | OK |
| Boîte Fatiguée | 69 | -8.7 | -240 dB | OK |
| Neige sur le Carrousel | 79 | -6.0 | -240 dB | OK |
| Tic-Tac Mélodie | 72 | -5.0 | -240 dB | OK |
| Rêve Mécanique | 72 | -6.0 | -239 dB | OK |
| Piano de Poche | 72 | -9.9 | -240 dB | OK |
| Salle de Jeux | 67 | -6.0 | -240 dB | OK |
| Petit Concert | 72 | -9.4 | -240 dB | OK |
| Piano Grinçant | 64 | -7.7 | -240 dB | OK |
| Marteaux de Feutre | 67 | -6.7 | -240 dB | OK |
| Cour de Récré | 72 | -4.1 | -240 dB | OK |
| Piano Hanté | 60 | -6.0 | -240 dB | OK |
| Jouet Neuf | 76 | -10.7 | -240 dB | OK |
| Clochettes d'Or | 79 | -8.5 | -240 dB | OK |
| Grelots du Traîneau | 84 | -13.4 | -240 dB | OK |
| Cloche de Comptoir | 72 | -9.3 | -240 dB | OK |
| Carillon du Manège | 76 | -8.4 | -240 dB | OK |
| Clochette Rouillée | 74 | -9.9 | -240 dB | OK |
| Étoile Filante | 84 | -10.1 | -237 dB | OK |
| Petite Église | 67 | -5.9 | -240 dB | OK |
| Cloches au Tempo | 79 | -9.5 | -240 dB | OK |
| Trompette en Fer | 67 | -7.7 | -240 dB | OK |
| Clairon du Chien | 72 | -9.4 | -240 dB | OK |
| Fanfare de Poche | 64 | -6.2 | -240 dB | OK |
| Kazoo Mécanique | 67 | -7.7 | -240 dB | OK |
| Parade Fatiguée | 62 | -6.3 | -240 dB | OK |
| Cor de Chasse Jouet | 60 | -6.0 | -240 dB | OK |
| Marche au Tic-Tac | 67 | -7.3 | -240 dB | OK |
| Trompette dans la Brume | 64 | -6.8 | -240 dB | OK |
| Tambour du Chien | 60 | -11.0 | -240 dB | OK |
| Boîte à Jouets | 60 | -11.0 | -240 dB | OK |
| Percussions de Bois | 61 | -12.0 | -240 dB | OK |
| Fanfare Miniature | 60 | -11.0 | -240 dB | OK |
| Grenier Poussiéreux | 60 | -10.6 | -240 dB | OK |
| Rythme Remonté | 60 | -11.3 | -240 dB | OK |
| Petit Cirque | 60 | -10.9 | -240 dB | OK |
| Jouets en Échos | 60 | -10.9 | -239 dB | OK |

## Justesse (à sec)

- [OK] Boîte à musique (note 72) : -0.8 cents
- [OK] Piano jouet (note 67) : +0.5 cents
- [OK] Clochettes (note 72) : -1.0 cents
- [OK] Fanfare (note 64) : +0.3 cents

## Mécanisme

- [OK] ressort détendu : tension 0.00, hauteur -75 cents (attendu vers -70)
- [OK] clé : remontage en moins d'1 s (tension 1.00), hauteur revenue à +0.1 cents
- [OK] tic-tac au tempo de l'hôte (1/8, 120 BPM) : 8 en 2 s (attendu 8)
- [OK] swing 60 % : deuxième tic à 0.320 s (attendu ≈ 0,325 s)
- [OK] écho à la croche pointée : -42.1 dB vers 375 ms contre -180.0 dB avant
- [OK] tambour : 12 sons sur 12 jouent et s'arrêtent d'eux-mêmes
- [OK] Méca : l'attaque gagne un clic mécanique (énergie aiguë × 2.9)

## CPU (48 kHz, blocs de 512)

- [OK] 16 voix de clochettes + écho + espace + méca + tic-tac : 2.9 % d'un cœur
- [OK] 16 voix de fanfare + écho + espace : 2.8 % d'un cœur

**Résultat : tout est OK.**
