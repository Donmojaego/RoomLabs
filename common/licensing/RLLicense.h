#pragma once
// =============================================================================
// Room Labs Audio — licence (Moonbase), commune à tous les plugins.
//  - activation en ligne, activation hors ligne par fichier, statut, désactivation de l'ordinateur
//    (écrans du module officiel moonbase_licensing) ;
//  - période de tolérance hors ligne après la dernière vérification en ligne (graceDays) ;
//  - réglages propres au plugin dans licensing.json → rl_license_config.h (aucun secret).
// Tant que licensing.json n'est pas rempli (enabled=false), le plugin fonctionne librement.
// =============================================================================
#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>
#include <memory>
#include "rl_license_config.h"

namespace rl
{
class License
{
public:
    License();
    ~License();
    void start();                               // charge la licence enregistrée (fil des messages)
    bool allowed() const noexcept;              // fil audio, sans verrou
    juce::var status() const;                   // pour l'interface : état, e-mail, expiration, réglages
    // affiche l'écran Moonbase (activation / hors ligne / détails & désactivation) par-dessus l'éditeur
    void showPanel (juce::Component& parent);
    void closePanel();
    std::function<void()> onChanged;            // licence activée / désactivée
    static juce::String copyright();            // « © 2026 Room Labs Audio · ℗ 2026 Room Labs Audio »
private:
    struct Impl; std::unique_ptr<Impl> impl;
};
} // namespace rl
