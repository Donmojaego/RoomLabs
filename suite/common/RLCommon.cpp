#include "RLCommon.h"
#include <BinaryData.h>

namespace rl
{
juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this, RL_FACE_W, RL_FACE_H); }

static const char* mimeFor (const juce::String& p)
{
    if (p.endsWith (".html")) return "text/html; charset=utf-8";
    if (p.endsWith (".js")) return "text/javascript; charset=utf-8";
    if (p.endsWith (".css")) return "text/css; charset=utf-8";
    if (p.endsWith (".webp")) return "image/webp";
    if (p.endsWith (".png")) return "image/png";
    if (p.endsWith (".svg")) return "image/svg+xml";
    if (p.endsWith (".json")) return "application/json";
    return "application/octet-stream";
}

std::optional<juce::WebBrowserComponent::Resource> Editor::serve (const juce::String& url)
{
    auto path = url.upToFirstOccurrenceOf ("?", false, false);
    if (path == "/" || path.isEmpty()) path = "/index.html";
    // les fichiers sont rangés à plat dans BinaryData : on cherche le nom « dossier_fichier » puis « fichier »
    auto flat = path.substring (1).replace ("/", "__");
    const auto name = path.fromLastOccurrenceOf ("/", false, false);
    const bool text = path.endsWith (".html") || path.endsWith (".js") || path.endsWith (".css");
    for (const auto& want : { flat, name })
        for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
        {
            if (want != BinaryData::originalFilenames[i]) continue;
            int size = 0; auto* data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], size);
            const size_t off = text ? 3 : 0;
            std::vector<std::byte> bytes ((size_t) size + off);
            if (text) { bytes[0] = std::byte { 0xEF }; bytes[1] = std::byte { 0xBB }; bytes[2] = std::byte { 0xBF }; }
            std::memcpy (bytes.data() + off, data, (size_t) size);
            return juce::WebBrowserComponent::Resource { std::move (bytes), mimeFor (path) };
        }
    return std::nullopt;
}

Editor::Editor (Processor& p, int w, int h) : AudioProcessorEditor (p), proc (p), faceW (w), faceH (h)
{
    using WBC = juce::WebBrowserComponent;
    using Args = const juce::Array<juce::var>&;
    using Done = WBC::NativeFunctionCompletion;
    auto prefsVar = [this]
    {
        auto* o = new juce::DynamicObject();
        auto ui = proc.getUi();
        o->setProperty ("theme", ui.hasProperty ("theme") ? ui["theme"] : juce::var (prefs->file().getValue ("theme", "vert")));
        o->setProperty ("zoom", prefs->file().getIntValue ("zoom_" + proc.product.replace ("/", "_"), 120));
        return juce::var (o);
    };
    auto opts = WBC::Options{}
        .withBackend (WBC::Options::Backend::webview2)
        .withWinWebView2Options (WBC::Options::WinWebView2{}
            .withUserDataFolder (juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("RoomLabs_Suite"))
            .withStatusBarDisabled())
        .withNativeIntegrationEnabled()
        .withInitialisationData ("prefs", prefsVar())
        .withInitialisationData ("product", proc.product)
        .withInitialisationData ("version", JucePlugin_VersionString)
        .withResourceProvider ([this] (const juce::String& u) { return serve (u); })
        .withNativeFunction ("getState", [this] (Args, Done done) { done (stateForUi()); })
        .withNativeFunction ("setParam", [this] (Args a, Done done)
        {
            if (a.size() >= 2)
                if (auto* prm = dynamic_cast<juce::RangedAudioParameter*> (proc.state.getParameter (a[0].toString())))
                {
                    const auto id = a[0].toString(); const bool own = ! inGesture.count (id);
                    if (own) prm->beginChangeGesture();
                    prm->setValueNotifyingHost (prm->convertTo0to1 ((float) (double) a[1]));
                    if (own) prm->endChangeGesture();
                    sent[id] = prm->convertFrom0to1 (prm->getValue());
                }
            done ({});
        })
        .withNativeFunction ("gesture", [this] (Args a, Done done)
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
        .withNativeFunction ("setPref", [this] (Args a, Done done)
        {
            const auto key = a[0].toString();
            if (key == "theme") { prefs->file().setValue ("theme", a[1]); proc.setUi ("theme", a[1]); }
            else if (key == "zoom") { prefs->file().setValue ("zoom_" + proc.product.replace ("/", "_"), a[1]); applyZoom ((int) a[1]); }
            else proc.setUi (key, a[1]);
            prefs->file().saveIfNeeded();
            done ({});
        })
        .withNativeFunction ("call", [this] (Args a, Done done)
        {
            juce::Array<juce::var> rest; for (int i = 1; i < a.size(); ++i) rest.add (a[i]);
            done (proc.call (a[0].toString(), rest));
        });
    web = std::make_unique<WBC> (opts);
    addAndMakeVisible (*web);
    setResizable (true, true);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) faceW / faceH);
    setResizeLimits (faceW * 6 / 10, faceH * 6 / 10, faceW * 2, faceH * 2);
    applyZoom (prefs->file().getIntValue ("zoom_" + proc.product.replace ("/", "_"), 120));
    web->goToURL (WBC::getResourceProviderRoot());
    startTimerHz (30);
}

Editor::~Editor()
{
    stopTimer();
    for (auto& id : inGesture) if (auto* prm = proc.state.getParameter (id)) prm->endChangeGesture();
}

void Editor::applyZoom (int pct) { pct = juce::jlimit (60, 200, pct); setSize (faceW * pct / 100, faceH * pct / 100); }

juce::var Editor::stateForUi() const
{
    auto* o = new juce::DynamicObject(); auto* params = new juce::DynamicObject(); auto* ranges = new juce::DynamicObject();
    for (auto* p : proc.getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            params->setProperty (r->getParameterID(), r->convertFrom0to1 (r->getValue()));
            auto rg = r->getNormalisableRange();
            juce::Array<juce::var> a { rg.start, rg.end, r->convertFrom0to1 (r->getDefaultValue()), rg.skew };
            if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (r)) { juce::Array<juce::var> ch; for (auto& s : c->choices) ch.add (s); a.add (ch); }
            ranges->setProperty (r->getParameterID(), a);
        }
    o->setProperty ("params", juce::var (params)); o->setProperty ("ranges", juce::var (ranges));
    o->setProperty ("ui", proc.getUi());
    o->setProperty ("latency", proc.getLatencySamples());
    o->setProperty ("sampleRate", proc.getSampleRate());
    return juce::var (o);
}

void Editor::timerCallback()
{
    ++tick;
    auto* changed = new juce::DynamicObject();
    for (auto* p : proc.getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            const float v = r->convertFrom0to1 (r->getValue());
            auto it = sent.find (r->getParameterID());
            if (it == sent.end() || std::abs (it->second - v) > 1e-5f) { sent[r->getParameterID()] = v; changed->setProperty (r->getParameterID(), v); }
        }
    juce::var cv (changed);
    if (! changed->getProperties().isEmpty()) web->emitEventIfBrowserIsVisible ("params", cv);
    auto fb = proc.feedback();
    if (! fb.isVoid()) web->emitEventIfBrowserIsVisible ("fb", fb);
}
} // namespace rl
