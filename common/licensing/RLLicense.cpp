#include "RLLicense.h"
#if RL_LICENSING
 #include <moonbase_licensing/moonbase_licensing.h>
#endif

namespace rl
{
#if RL_LICENSING
using namespace moonbase::juce_integration;
static ActivationConfig makeConfig()
{
    ActivationConfig cfg;
    cfg.endpoint = licensecfg::endpoint;
    cfg.productId = licensecfg::productId;
    cfg.publicKey = licensecfg::publicKey;
    cfg.enableOffline = licensecfg::offline;
    if (licensecfg::checkIntervalHours > 0) cfg.onlineCheckInterval = std::chrono::hours (licensecfg::checkIntervalHours);
    if (licensecfg::graceDays > 0) cfg.onlineGracePeriod = std::chrono::hours (24 * licensecfg::graceDays);
    if (licensecfg::trialDays > 0) cfg.trialLengthDays = licensecfg::trialDays;
    return cfg;
}
struct License::Impl
{
    ActivationController controller { makeConfig() };
    std::unique_ptr<ActivationComponent> panel;
};
#else
struct License::Impl {};
#endif

License::License() : impl (std::make_unique<Impl>()) {}
License::~License() { closePanel(); }

void License::start()
{
   #if RL_LICENSING
    impl->controller.start();
   #endif
}

bool License::allowed() const noexcept
{
   #if RL_LICENSING
    return impl->controller.licensedFlag().load (std::memory_order_relaxed);
   #else
    return true;                                // licence pas encore configurée : version libre
   #endif
}

juce::var License::status() const
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("configured", (bool) RL_LICENSING);
    o->setProperty ("licensed", allowed());
    o->setProperty ("maxDevices", licensecfg::maxDevices);
    o->setProperty ("graceDays", licensecfg::graceDays);
    o->setProperty ("offline", licensecfg::offline);
    o->setProperty ("copyright", copyright());
   #if RL_LICENSING
    o->setProperty ("state", allowed() ? "active" : "inactive");
   #else
    o->setProperty ("state", "unconfigured");
   #endif
    return juce::var (o);
}

void License::showPanel (juce::Component& parent)
{
   #if RL_LICENSING
    closePanel();
    impl->panel = std::make_unique<ActivationComponent> (impl->controller);
    impl->panel->onClose = [this] { juce::MessageManager::callAsync ([this] { closePanel(); }); };
    impl->panel->onActivationChanged = [this] (bool) { if (onChanged) onChanged(); };
    parent.addAndMakeVisible (*impl->panel);
    impl->panel->setBounds (parent.getLocalBounds());
   #else
    juce::ignoreUnused (parent);
   #endif
}

void License::closePanel()
{
   #if RL_LICENSING
    if (impl && impl->panel) { if (auto* p = impl->panel->getParentComponent()) p->removeChildComponent (impl->panel.get()); impl->panel.reset(); }
   #endif
}

juce::String License::copyright()
{
    return juce::String::fromUTF8 ("\xc2\xa9 2026 Room Labs Audio \xc2\xb7 \xe2\x84\x97 2026 Room Labs Audio");
}
} // namespace rl
