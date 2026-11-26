#include "PluginEditor.h"
#include <BinaryData.h>

// ---------------------------------------------------------------------------
UiPrefs::UiPrefs()
{
    juce::PropertiesFile::Options o;
    o.applicationName = "Ma petite soupe ouselem";
    o.folderName = "Room Labs" + juce::File::getSeparatorString() + "Ma petite soupe ouselem";
    o.filenameSuffix = "settings";
    o.osxLibrarySubFolder = "Application Support";
    props = std::make_unique<juce::PropertiesFile> (o);
}

juce::var UiPrefs::toVar() const
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("theme", props->getValue ("theme", "soupe"));     // soupe | ramen (Mon petit ramen)
    o->setProperty ("zoom", props->getIntValue ("zoom", 100));
    o->setProperty ("visual", props->getValue ("visual", "anime"));   // anime | reduit | statique
    return juce::var (o);
}

// ---------------------------------------------------------------------------
static const char* mimeFor (const juce::String& path)
{
    if (path.endsWith (".html")) return "text/html; charset=utf-8";
    if (path.endsWith (".js")) return "text/javascript; charset=utf-8";
    if (path.endsWith (".css")) return "text/css";
    if (path.endsWith (".webp")) return "image/webp";
    if (path.endsWith (".png")) return "image/png";
    if (path.endsWith (".svg")) return "image/svg+xml";
    if (path.endsWith (".json")) return "application/json";
    return "application/octet-stream";
}

std::optional<juce::WebBrowserComponent::Resource> SoupEditor::serve (const juce::String& url)
{
    auto path = url.upToFirstOccurrenceOf ("?", false, false);
    if (path == "/" || path.isEmpty()) path = "/index.html";
    const auto fileName = path.fromLastOccurrenceOf ("/", false, false);
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        if (fileName != BinaryData::originalFilenames[i]) continue;
        int size = 0;
        auto* data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], size);
        // BOM UTF-8 devant le texte : les accents s'affichent bien avec tous les moteurs web
        const bool text = path.endsWith (".html") || path.endsWith (".js") || path.endsWith (".css");
        const size_t off = text ? 3 : 0;
        std::vector<std::byte> bytes ((size_t) size + off);
        if (text) { bytes[0] = std::byte { 0xEF }; bytes[1] = std::byte { 0xBB }; bytes[2] = std::byte { 0xBF }; }
        std::memcpy (bytes.data() + off, data, (size_t) size);
        return juce::WebBrowserComponent::Resource { std::move (bytes), mimeFor (path) };
    }
    return std::nullopt;
}

// ---------------------------------------------------------------------------
SoupEditor::SoupEditor (SoupProcessor& p)
    : AudioProcessorEditor (p), proc (p)
{
    using WBC = juce::WebBrowserComponent;
    auto opts = WBC::Options{}
        .withBackend (WBC::Options::Backend::webview2)
        .withWinWebView2Options (WBC::Options::WinWebView2{}
            .withUserDataFolder (juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("RoomLabs_SoupeOuselem"))
            .withStatusBarDisabled())
        .withNativeIntegrationEnabled()
        .withInitialisationData ("prefs", mergedPrefs())
        .withInitialisationData ("version", SOUP_VERSION_STRING)
        .withResourceProvider ([this] (const juce::String& url) { return serve (url); })
        .withNativeFunction ("getState", [this] (const juce::Array<juce::var>&, WBC::NativeFunctionCompletion done)
        {
            done (stateForUi (true));
        })
        .withNativeFunction ("setParam", [this] (const juce::Array<juce::var>& a, WBC::NativeFunctionCompletion done)
        {
            if (a.size() >= 2) setParamFromUi (a[0].toString(), (float) (double) a[1]);
            done ({});
        })
        .withNativeFunction ("gesture", [this] (const juce::Array<juce::var>& a, WBC::NativeFunctionCompletion done)
        {
            if (a.size() >= 2)
                if (auto* prm = proc.state.getParameter (a[0].toString()))
                {
                    const auto id = a[0].toString();
                    if ((bool) a[1] && ! inGesture.count (id)) { inGesture.insert (id); prm->beginChangeGesture(); }
                    else if (! (bool) a[1] && inGesture.count (id)) { inGesture.erase (id); prm->endChangeGesture(); }
                }
            done ({});
        })
        .withNativeFunction ("noteOn", [this] (const juce::Array<juce::var>& a, WBC::NativeFunctionCompletion done)
        {
            proc.keyboard.noteOn (a.size() > 2 ? juce::jlimit (1, 16, (int) a[2]) : 1, juce::jlimit (0, 127, (int) a[0]), juce::jlimit (0.05f, 1.f, (float) (double) a[1]));
            done ({});
        })
        .withNativeFunction ("noteOff", [this] (const juce::Array<juce::var>& a, WBC::NativeFunctionCompletion done)
        {
            proc.keyboard.noteOff (a.size() > 1 ? juce::jlimit (1, 16, (int) a[1]) : 1, juce::jlimit (0, 127, (int) a[0]), 0.f);
            done ({});
        })
        .withNativeFunction ("loadPreset", [this] (const juce::Array<juce::var>& a, WBC::NativeFunctionCompletion done)
        {
            proc.loadPreset ((int) a[0]);
            lastPreset = proc.currentPreset.load();
            done (stateForUi (false));
        })
        .withNativeFunction ("license", [this] (const juce::Array<juce::var>&, WBC::NativeFunctionCompletion done)
        {
            done (proc.license.status());
        })
        .withNativeFunction ("licensePanel", [this] (const juce::Array<juce::var>&, WBC::NativeFunctionCompletion done)
        {
            proc.license.showPanel (*this);     // activation en ligne / hors ligne, détails, désactivation de l'ordinateur
            done ({});
        })
        .withNativeFunction ("openUrl", [this] (const juce::Array<juce::var>& a, WBC::NativeFunctionCompletion done)
        {
            const auto u = a[0].toString();
            if (u.startsWith ("https://") || u.startsWith ("http://")) juce::URL (u).launchInDefaultBrowser();
            done ({});
        })
        .withNativeFunction ("setPref", [this] (const juce::Array<juce::var>& a, WBC::NativeFunctionCompletion done)
        {
            const auto key = a[0].toString();
            prefs->file().setValue (key, a[1]);                 // préférence globale (nouveaux projets)
            prefs->file().saveIfNeeded();
            static const juce::StringArray perProject { "theme", "zoom", "visual" };
            if (perProject.contains (key)) proc.setProjectUi (key, a[1]);   // et mémorisée dans le projet
            if (key == "zoom") applyZoom ((int) a[1]);
            done ({});
        });

    web = std::make_unique<WBC> (opts);
    addAndMakeVisible (*web);
    proc.license.onChanged = [this] { if (web) web->emitEventIfBrowserIsVisible ("license", proc.license.status()); };

    setResizable (true, true);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) baseW / baseH);
    setResizeLimits (baseW / 2, baseH / 2, baseW * 2, baseH * 2);
    applyZoom ((int) mergedPrefs().getProperty ("zoom", 100));

    web->goToURL (WBC::getResourceProviderRoot());
    startTimerHz (60);
}

SoupEditor::~SoupEditor()
{
    stopTimer();
    proc.license.onChanged = nullptr;
    proc.license.closePanel();
    for (auto& id : inGesture)
        if (auto* prm = proc.state.getParameter (id)) prm->endChangeGesture();
}

void SoupEditor::visibilityChanged()
{
    // rendu suspendu quand la fenêtre est cachée
    if (isShowing()) { if (! isTimerRunning()) startTimerHz (60); }
    else stopTimer();
}

void SoupEditor::applyZoom (int percent)
{
    percent = juce::jlimit (50, 200, percent);
    setSize (baseW * percent / 100, baseH * percent / 100);
}

void SoupEditor::resized() { if (web) web->setBounds (getLocalBounds()); }

void SoupEditor::setParamFromUi (const juce::String& id, float value)
{
    auto* prm = dynamic_cast<juce::RangedAudioParameter*> (proc.state.getParameter (id));
    if (prm == nullptr) return;
    const float norm = prm->convertTo0to1 (value);
    const bool own = ! inGesture.count (id);
    if (own) prm->beginChangeGesture();
    prm->setValueNotifyingHost (norm);
    if (own) prm->endChangeGesture();
    sent[id] = prm->convertFrom0to1 (prm->getValue());
}

juce::var SoupEditor::mergedPrefs() const
{
    auto v = prefs->toVar();
    auto proj = proc.getProjectUi();
    if (auto* o = proj.getDynamicObject())
        for (auto& kv : o->getProperties())
            v.getDynamicObject()->setProperty (kv.name, kv.value);
    return v;
}

juce::var SoupEditor::stateForUi (bool full) const
{
    auto* o = new juce::DynamicObject();
    auto* params = new juce::DynamicObject();
    for (auto* p : proc.getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (p))
            params->setProperty (r->getParameterID(), r->convertFrom0to1 (r->getValue()));
    o->setProperty ("params", juce::var (params));
    o->setProperty ("preset", proc.currentPreset.load());
    if (full)
    {
        juce::Array<juce::var> list;
        for (auto& fp : proc.factory())
        {
            auto* e = new juce::DynamicObject();
            e->setProperty ("name", fp.name);
            e->setProperty ("sachet", fp.sachet);
            e->setProperty ("icon", fp.icon);
            e->setProperty ("section", fp.section);
            e->setProperty ("root", fp.root);
            e->setProperty ("rootName", fp.rootName);
            e->setProperty ("mode", fp.mode);
            list.add (juce::var (e));
        }
        o->setProperty ("presets", list);
        o->setProperty ("sachets", proc.factoryInfo().getProperty ("sachets", {}));
        o->setProperty ("ramenSachets", proc.factoryInfo().getProperty ("ramenSachets", {}));
        o->setProperty ("instruments", proc.factoryInfo().getProperty ("instruments", {}));
        o->setProperty ("fxList", proc.factoryInfo().getProperty ("fx", {}));
        o->setProperty ("divisions", proc.factoryInfo().getProperty ("divisions", {}));
        o->setProperty ("version", SOUP_VERSION_STRING);
        o->setProperty ("native", true);
    }
    return juce::var (o);
}

// ---------------------------------------------------------------------------
// 60 Hz : mouvement (seulement quand il se passe quelque chose), paramètres à 30 Hz.
void SoupEditor::timerCallback()
{
    ++tick;

    // 1) valeurs changées par l'hôte (automation, preset de projet…) → interface
    if ((tick & 1) == 0)
    {
        auto* changed = new juce::DynamicObject();
        for (auto* p : proc.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (p))
            {
                const float v = r->convertFrom0to1 (r->getValue());
                auto it = sent.find (r->getParameterID());
                if (it == sent.end() || std::abs (it->second - v) > 1e-5f)
                {
                    sent[r->getParameterID()] = v;
                    changed->setProperty (r->getParameterID(), v);
                }
            }
        juce::var changedVar (changed);
        if (! changed->getProperties().isEmpty())
            web->emitEventIfBrowserIsVisible ("params", changedVar);

        if (proc.currentPreset.load() != lastPreset)
        {
            lastPreset = proc.currentPreset.load();
            web->emitEventIfBrowserIsVisible ("preset", lastPreset);
        }
    }

    // 2) retour du moteur : surface du bol, vapeur, tourbillon
    auto& f = proc.feedback();
    const float energy = f.energy.load (std::memory_order_relaxed), wet = f.wet.load (std::memory_order_relaxed);
    const int hits = f.noteHits.load (std::memory_order_relaxed);
    const bool active = energy > 0.002f || wet > 0.002f || f.voices.load() > 0 || hits != lastOnsets;
    idleTicks = active ? 0 : idleTicks + 1;
    if (idleTicks < 3)
    {
        lastOnsets = hits;
        auto* o = new juce::DynamicObject();
        o->setProperty ("e", energy);
        o->setProperty ("w", wet);
        o->setProperty ("h", hits);
        o->setProperty ("b", f.beat.load (std::memory_order_relaxed));
        o->setProperty ("v", f.voices.load (std::memory_order_relaxed));
        o->setProperty ("l", f.level.load (std::memory_order_relaxed));
        o->setProperty ("bpm", proc.hostBpm.load());
        web->emitEventIfBrowserIsVisible ("fb", juce::var (o));
    }
}
