# Licence Room Labs Audio (Moonbase)

Couche commune à tous les plugins. Chaque plugin a son `licensing.json` (réglages propres au plugin) :

```json
{ "enabled": false, "endpoint": "", "productId": "", "publicKeyFile": "moonbase_public_key.pem",
  "maxDevices": 3, "graceDays": 14, "checkIntervalHours": 24, "offline": true, "trialDays": 0 }
```

Quand le compte Moonbase est prêt, pour chaque produit :
1. Créer le produit dans Moonbase et y régler **3 ordinateurs par licence** (la limite est appliquée par le serveur).
2. Recopier l'adresse du compte (`endpoint`) et l'identifiant du produit (`productId`).
3. Déposer la **clé publique** du produit (PEM) à côté de `licensing.json`. Jamais de clé API secrète dans le dépôt ni dans le plugin.
4. Passer `enabled` à `true` et recompiler.

Fonctions : activation en ligne, activation hors ligne par fichier (demande → réponse), statut affiché dans Paramètres,
désactivation de l'ordinateur (écran « Détails de la licence »), utilisation hors ligne pendant `graceDays` après la dernière
vérification en ligne réussie. Module : `moonbase_licensing` (dépôt MIT Moonbase-sh/moonbase-cpp), récupéré à la compilation.
