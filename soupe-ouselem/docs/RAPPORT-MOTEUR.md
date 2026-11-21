# Ma petite soupe ouselem — rapport des tests du moteur

## Recettes d'usine (note de référence, 120 BPM)

| Recette | Note | Justesse à sec (cents) | Crête (dBFS) | Résultat |
|---|---|---|---|---|
| Violon Renversé | 67 | +0.0 | -5.7 | OK |
| Archet Miroir | 64 | +0.0 | -6.3 | OK |
| Retour Vif | 72 | -0.0 | -5.8 | OK |
| Grain d'Archet | 67 | +0.0 | -6.0 | OK |
| Violon Brumeux | 62 | -0.0 | -5.2 | OK |
| Pad Mijoté | 60 | +0.1 | -5.9 | OK |
| Frémissement | 60 | +0.1 | -5.3 | OK |
| Bouillon Lent | 57 | +0.3 | -6.0 | OK |
| Piano Mijoté | 60 | -0.0 | -6.0 | OK |
| Pad Granité | 64 | -0.5 | -5.6 | OK |
| Gouttes au Sésame | 72 | — | -6.2 | OK |
| Cloc Touc | 60 | — | -6.2 | OK |
| Pluie de Graines | 79 | — | -6.3 | OK |
| Goutte Lointaine | 76 | — | -6.1 | OK |
| Touc Grave | 48 | — | -6.2 | OK |
| Nouilles Longues | 60 | -0.0 | -6.0 | OK |
| Fil Étiré | 67 | -0.1 | -6.0 | OK |
| Pâte Souple | 55 | — | -4.9 | OK |
| Cordes Tirées | 64 | +0.1 | -6.0 | OK |
| Ramen Infini | 60 | — | -3.2 | OK |
| Bouillon d'Arpèges | 60 | — | -6.0 | OK |
| Bulles Montantes | 64 | — | -6.0 | OK |
| Mijotage Haché | 57 | — | -6.0 | OK |
| Arpège Descendant | 67 | — | -6.0 | OK |
| Clavier Découpé | 60 | — | -6.0 | OK |
| Fond de Bol | 36 | -0.0 | -6.0 | OK |
| Bouillon Grave | 33 | +0.0 | -6.0 | OK |
| Sous la Cuillère | 38 | -0.0 | -6.0 | OK |
| Touc Profond | 36 | — | -6.6 | OK |
| Basse Feutrée | 40 | +0.0 | -6.1 | OK |

## Effets calés sur le tempo (120 BPM)

- [OK] toutes les recettes : son, pas de NaN, crête ≤ 0 dBFS, justesse ±12 cents, voix libérées
- [OK] Reverse 1/4 : le coup revient à l'envers dans la noire suivante (début 0.0000 < fin 0.0388)
- [OK] Stutter 1/16 : la 3e tranche du temps répète la 1re (corrélation 1.000)
- [OK] Répétitions 1/8 (aller-retour gauche/droite) : échos à gauche à 250 et 750 ms (0.0199, 0.0071), silence entre (0.0000)
- [OK] Découpage 1/16 : pas allumé 0.2802, pas éteint 0.00000
- [OK] Étirement : après la note, le son étiré continue (0.1712) quand le son sec s'est tu (0.00000)
- [OK] Filtre modulé 1/4 : le niveau varie de 6.2 dB sur un cycle de 4 noires
- [OK] Grain Delay 1/4 : grains audibles une noire plus tard (0.0223)
- [OK] interrupteurs Reverse + Stutter ensemble : son présent, sans NaN
- [OK] arpège du Bouillon (1/16, 120 BPM) : 16 notes en 2 s (attendu 16 à 17)
- [OK] suit la position de l'hôte (noires) pour caler les effets et l'arpège

## CPU (48 kHz, blocs de 512)

- [OK] 12 voix de cordes + étirement + Space : 3.5 % d'un cœur
- [OK] 8 voix de violon + Grain Delay + Reverse + Stutter : 2.0 % d'un cœur

**Résultat : tout est OK.**
## Animation (navigateur sans carte graphique : ordre de grandeur)

| Taille | Bol | Mode | Cas | i/s | Rendu moyen | Rendu 95 % |
|---|---|---|---|---|---|---|
| 50 % | soupe | anime | note courte | 60 | 0.21 ms | 0.30 ms |
| 50 % | soupe | anime | tenue + effet | 60 | 0.70 ms | 2.00 ms |
| 50 % | ramen | anime | note courte | 58 | 0.62 ms | 1.70 ms |
| 50 % | ramen | anime | tenue + effet | 60 | 0.27 ms | 0.40 ms |
| 100 % | soupe | anime | note courte | 60 | 0.31 ms | 0.40 ms |
| 100 % | soupe | anime | tenue + effet | 60 | 0.25 ms | 0.40 ms |
| 100 % | soupe | anime | silence | 25 | 0.59 ms | 0.30 ms |
| 100 % | soupe | reduit | note courte | 29 | 0.23 ms | 0.30 ms |
| 100 % | soupe | reduit | tenue + effet | 30 | 0.25 ms | 0.40 ms |
| 100 % | soupe | reduit | silence | 19 | 0.19 ms | 0.30 ms |
| 100 % | soupe | statique | note courte | 0 | 0.00 ms | 0.00 ms |
| 100 % | soupe | statique | tenue + effet | 0 | 0.00 ms | 0.00 ms |
| 100 % | soupe | statique | silence | 0 | 0.00 ms | 0.00 ms |
| 100 % | ramen | anime | note courte | 58 | 0.52 ms | 2.70 ms |
| 100 % | ramen | anime | tenue + effet | 60 | 0.24 ms | 0.40 ms |
| 200 % | soupe | anime | note courte | 60 | 0.26 ms | 0.40 ms |
| 200 % | soupe | anime | tenue + effet | 60 | 0.28 ms | 0.40 ms |
| 200 % | ramen | anime | note courte | 58 | 0.30 ms | 0.40 ms |
| 200 % | ramen | anime | tenue + effet | 60 | 0.32 ms | 0.40 ms |

5 instances ouvertes (100 %, tenue + effet) : 62 à 64 i/s, rendu moyen 0.33 ms.
