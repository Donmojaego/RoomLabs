#include "PluginEditor.h"
#include <BinaryData.h>

// ---------------------------------------------------------------------------
UiPrefs::UiPrefs()
{
    juce::PropertiesFile::Options o;
    o.applicationName = "Kaleidoscope";
    o.folderName = "Room Labs" + juce::File::getSeparatorString() + "Kaleidoscope";
    o.filenameSuffix = "settings";
    o.osxLibrarySubFolder = "Application Support";
    props = std::make_unique<juce::PropertiesFile> (o);
}

juce::var UiPrefs::toVar() const
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("layout", props->getValue ("layout", "retro"));      // retro | telescope | holo | laiton
    o->setProperty ("zoom", props->getIntValue ("zoom", 100));
    o->setProperty ("visual", props->getValue ("visual", "anime"));      // anime | reduit | statique
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
    if (path.endsWith (".jpg")) return "image/jpeg";
    if (path.endsWith (".svg")) return "image/svg+xml";
    if (path.endsWith (".json")) return "application/json";
    if (path.endsWith (".woff2")) return "font/woff2";
    return "application/octet-stream";
}

juce::File KaleidoEditor::assetsOverride() const
{
    // les assets d'un layout peuvent être essayés sans recompiler : Documents/Room Labs/Kaleidoscope/Assets/<fichier>
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Room Labs/Kaleidoscope/Assets");
}

std::optional<juce::WebBrowserComponent::Resource> KaleidoEditor::serve (const juce::String& url)
{
    auto path = url.upToFirstOccurrenceOf ("?", false, false);
    if (path == "/" || path.isEmpty()) path = "/index.html";
    const auto fileName = path.fromLastOccurrenceOf ("/", false, false);
    const bool text = path.endsWith (".html") || path.endsWith (".js") || path.endsWith (".css");
    auto wrap = [&] (const void* data, size_t size)
    {
        const size_t off = text ? 3 : 0;          // BOM UTF-8 : accents corrects avec tous les moteurs web
        std::vector<std::byte> bytes (size + off);
        if (text) { bytes[0] = std::byte { 0xEF }; bytes[1] = std::byte { 0xBB }; bytes[2] = std::byte { 0xBF }; }
        std::memcpy (bytes.data() + off, data, size);
        return juce::WebBrowserComponent::Resource { std::move (bytes), mimeFor (path) };
    };
    if (path.startsWith ("/assets/"))
    {
        auto f = assetsOverride().getChildFile (fileName);
        if (f.existsAsFile()) { juce::MemoryBlock mb; f.loadFileAsData (mb); return wrap (mb.getData(), mb.getSize()); }
    }
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        if (fileName != BinaryData::originalFilenames[i]) continue;
        int size = 0;
        auto* data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], size);
        return wrap (data, (size_t) size);
    }
    return std::nullopt;
}

// ---------------------------------------------------------------------------
KaleidoEditor::KaleidoEditor (KaleidoProcessor& p)
    : AudioProcessorEditor (p), proc (p)
{
    using WBC = juce::WebBrowserComponent;
    using Args = const juce::Array<juce::var>&;
    using Done = WBC::NativeFunctionCompletion;
    auto opts = WBC::Options{}
        .withBackend (WBC::Options::Backend::webview2)
        .withWinWebView2Options (WBC::Options::WinWebView2{}
            .withUserDataFolder (juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("RoomLabs_Kaleidoscope"))
            .withStatusBarDisabled())
        .withNativeIntegrationEnabled()
        .withInitialisationData ("prefs", mergedPrefs())
        .withInitialisationData ("version", KAL_VERSION_STRING)
        .withResourceProvider ([this] (const juce::String& url) { return serve (url); })
        .withNativeFunction ("getState", [this] (Args, Done done) { done (stateForUi (true)); })
        .withNativeFunction ("setParam", [this] (Args a, Done done) { if (a.size() >= 2) setParamFromUi (a[0].toString(), (float) (double) a[1]); done ({}); })
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
        .withNativeFunction ("noteOn", [this] (Args a, Done done)
        { proc.keyboard.noteOn (1, juce::jlimit (0, 127, (int) a[0]), juce::jlimit (0.05f, 1.f, (float) (double) a[1])); done ({}); })
        .withNativeFunction ("noteOff", [this] (Args a, Done done) { proc.keyboard.noteOff (1, juce::jlimit (0, 127, (int) a[0]), 0.f); done ({}); })
        .withNativeFunction ("loadPreset", [this] (Args a, Done done)
        { proc.loadPreset ((int) a[0]); lastPreset = proc.currentPreset.load(); done (stateForUi (false)); })
        .withNativeFunction ("generate", [this] (Args, Done done) { auto v = proc.generate(); lastPattern = proc.patternSerial.load(); done (v); })
        .withNativeFunction ("decline", [this] (Args, Done done) { auto v = proc.decline(); lastPattern = proc.patternSerial.load(); done (v); })
        .withNativeFunction ("previous", [this] (Args, Done done) { auto v = proc.previous(); lastPattern = proc.patternSerial.load(); done (v); })
        .withNativeFunction ("feedback", [this] (Args a, Done done) { auto v = proc.feedback (a[0].toString()); lastPattern = proc.patternSerial.load(); done (v); })
        .withNativeFunction ("resetPersonal", [this] (Args, Done done) { proc.resetPersonal(); done (proc.patternInfo()); })
        .withNativeFunction ("setDrop", [this] (Args a, Done done) { auto v = proc.setDrop (a[0]); lastPattern = proc.patternSerial.load(); done (v); })
        .withNativeFunction ("exportMidi", [this] (Args, Done done)
        {
            auto f = proc.exportMidi();
            done (f.existsAsFile() ? juce::var (f.getFullPathName()) : juce::var());
        })
        .withNativeFunction ("dragMidi", [this] (Args, Done done)
        {
            // glisser la mélodie vers une piste de l'hôte (fichier .mid temporaire)
            auto f = proc.exportMidi (juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("Kaleidoscope_melodie.mid"));
            if (f.existsAsFile()) performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
            done (f.existsAsFile());
        })
        .withNativeFunction ("openFolder", [this] (Args a, Done done)
        {
            auto which = a.size() > 0 ? a[0].toString() : juce::String ("midi");
            auto dir = which == "assets" ? assetsOverride()
                     : which == "banks" ? juce::File (proc.bankInfo().getProperty ("folder", "").toString())
                     : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Room Labs/Kaleidoscope/MIDI");
            dir.createDirectory();
            dir.startAsProcess();
            done ({});
        })
        .withNativeFunction ("bankInfo", [this] (Args, Done done) { done (proc.bankInfo()); })
        .withNativeFunction ("setPref", [this] (Args a, Done done)
        {
            const auto key = a[0].toString();
            prefs->file().setValue (key, a[1]);
            prefs->file().saveIfNeeded();
            static const juce::StringArray perProject { "layout", "zoom", "visual" };
            if (perProject.contains (key)) proc.setProjectUi (key, a[1]);
            if (key == "zoom") applyZoom ((int) a[1]);
            done ({});
        });

    web = std::make_unique<WBC> (opts);
    addAndMakeVisible (*web);
    setResizable (true, true);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) baseW / baseH);
    setResizeLimits (baseW / 2, baseH / 2, baseW * 2, baseH * 2);
    applyZoom ((int) mergedPrefs().getProperty ("zoom", 100));
    web->goToURL (WBC::getResourceProviderRoot());
    lastPattern = proc.patternSerial.load();
    startTimerHz (60);
}

KaleidoEditor::~KaleidoEditor()
{
    stopTimer();
    for (auto& id : inGesture)
        if (auto* prm = proc.state.getParameter (id)) prm->endChangeGesture();
}

void KaleidoEditor::visibilityChanged()
{
    if (isShowing()) { if (! isTimerRunning()) startTimerHz (60); }
    else stopTimer();
}

void KaleidoEditor::applyZoom (int percent)
{
    percent = juce::jlimit (50, 200, percent);
    setSize (baseW * percent / 100, baseH * percent / 100);
}

void KaleidoEditor::resized() { if (web) web->setBounds (getLocalBounds()); }

void KaleidoEditor::setParamFromUi (const juce::String& id, float value)
{
    auto* prm = dynamic_cast<juce::RangedAudioParameter*> (proc.state.getParameter (id));
    if (prm == nullptr) return;
    const bool own = ! inGesture.count (id);
    if (own) prm->beginChangeGesture();
    prm->setValueNotifyingHost (prm->convertTo0to1 (value));
    if (own) prm->endChangeGesture();
    sent[id] = prm->convertFrom0to1 (prm->getValue());
}

juce::var KaleidoEditor::mergedPrefs() const
{
    auto v = prefs->toVar();
    auto proj = proc.getProjectUi();
    if (auto* o = proj.getDynamicObject())
        for (auto& kv : o->getProperties()) v.getDynamicObject()->setProperty (kv.name, kv.value);
    return v;
}

juce::var KaleidoEditor::stateForUi (bool full) const
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
            e->setProperty ("id", fp.id); e->setProperty ("name", fp.name); e->setProperty ("cat", fp.cat);
            e->setProperty ("fond", fp.fond); e->setProperty ("bank", fp.bank); e->setProperty ("style", fp.style); e->setProperty ("mood", fp.mood);
            e->setProperty ("version", fp.version);
            list.add (juce::var (e));
        }
        o->setProperty ("presets", list);
        // bornes des paramètres (molettes)
        auto* ranges = new juce::DynamicObject();
        for (auto* p : proc.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (p))
            {
                auto rg = r->getNormalisableRange();
                juce::Array<juce::var> a { rg.start, rg.end, r->getDefaultValue() >= 0 ? r->convertFrom0to1 (r->getDefaultValue()) : 0.f, rg.skew };
                ranges->setProperty (r->getParameterID(), a);
            }
        o->setProperty ("ranges", juce::var (ranges));
        o->setProperty ("pattern", proc.patternInfo());
        o->setProperty ("banks", proc.bankInfo());
        o->setProperty ("version", KAL_VERSION_STRING);
        o->setProperty ("native", true);
    }
    return juce::var (o);
}

// ---------------------------------------------------------------------------
void KaleidoEditor::timerCallback()
{
    ++tick;
    if ((tick & 1) == 0)
    {
        auto* changed = new juce::DynamicObject();
        for (auto* p : proc.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (p))
            {
                const float v = r->convertFrom0to1 (r->getValue());
                auto it = sent.find (r->getParameterID());
                if (it == sent.end() || std::abs (it->second - v) > 1e-5f) { sent[r->getParameterID()] = v; changed->setProperty (r->getParameterID(), v); }
            }
        juce::var changedVar (changed);
        if (! changed->getProperties().isEmpty()) web->emitEventIfBrowserIsVisible ("params", changedVar);
        if (proc.currentPreset.load() != lastPreset) { lastPreset = proc.currentPreset.load(); web->emitEventIfBrowserIsVisible ("preset", lastPreset); }
        if (proc.patternSerial.load() != lastPattern) { lastPattern = proc.patternSerial.load(); web->emitEventIfBrowserIsVisible ("pattern", proc.patternInfo()); }
    }
    if (tick % 15 == 0)
    {
        auto b = proc.bankInfo();
        auto s = juce::JSON::toString (b, true);
        if (s != lastBankState) { lastBankState = s; web->emitEventIfBrowserIsVisible ("banks", b); }
    }
    auto& f = proc.feedback();
    const float energy = f.energy.load (std::memory_order_relaxed);
    const int hits = f.noteHits.load (std::memory_order_relaxed), step = proc.seqStep.load();
    const bool active = energy > 0.002f || f.voices.load() > 0 || hits != lastHits || step != lastStep;
    idleTicks = active ? 0 : idleTicks + 1;
    if (idleTicks < 3)
    {
        lastHits = hits; lastStep = step;
        auto* o = new juce::DynamicObject();
        o->setProperty ("e", energy);
        o->setProperty ("h", hits);
        o->setProperty ("n", f.lastNote.load (std::memory_order_relaxed));
        o->setProperty ("vel", f.lastVel.load (std::memory_order_relaxed));
        o->setProperty ("v", f.voices.load (std::memory_order_relaxed));
        o->setProperty ("l", f.level.load (std::memory_order_relaxed));
        o->setProperty ("b", proc.beatPos.load());
        o->setProperty ("s", step);
        o->setProperty ("bpm", proc.hostBpm.load());
        web->emitEventIfBrowserIsVisible ("fb", juce::var (o));
    }
}
