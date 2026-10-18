#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Core/Randomizer.h"

using namespace monie;

// Analysis runs on its own thread: it drains the lock-free FIFO, runs FFT/beat detection and publishes atomics.
class MonieProcessor::AnalysisThread : public juce::Thread
{
public:
    AnalysisThread (MonieProcessor& p, double sr) : juce::Thread ("monie analysis"), proc (p) { analyzer.prepare (sr); }
    void run() override
    {
        std::vector<float> tmp (1024);
        while (! threadShouldExit())
        {
            const int ready = proc.fifo.getNumReady();
            if (ready <= 0) { wait (3); continue; }
            const int n = juce::jmin (ready, (int) tmp.size());
            int s1, n1, s2, n2;
            proc.fifo.prepareToRead (n, s1, n1, s2, n2);
            if (n1 > 0) std::copy (proc.fifoData.begin() + s1, proc.fifoData.begin() + s1 + n1, tmp.begin());
            if (n2 > 0) std::copy (proc.fifoData.begin() + s2, proc.fifoData.begin() + s2 + n2, tmp.begin() + n1);
            proc.fifo.finishedRead (n1 + n2);

            if (proc.beatSensParam != nullptr) analyzer.setBeatSensitivity (proc.beatSensParam->load());
            if (analyzer.push (tmp.data(), n1 + n2))
            {
                const auto& f = analyzer.getFrame();
                auto& a = proc.analysis;
                a.bass = f.bass; a.mid = f.mid; a.treble = f.treble; a.rms = f.rms; a.peak = f.peak; a.flux = f.flux;
                a.lowMid = f.lowMid; a.highMid = f.highMid; a.transient = f.transient;
                a.beatCount = f.beatCount;
                for (int i = 0; i < AnalysisFrame::numBands; ++i) a.bands[i] = f.bands[i];
                for (int i = 0; i < AnalysisFrame::numWave; ++i)  a.wave[i] = f.wave[i];
            }
        }
    }
private:
    MonieProcessor& proc;
    AudioAnalyzer analyzer;
};

juce::AudioProcessorValueTreeState::ParameterLayout MonieProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "scene", 1 }, "Scene", sceneNames(), 0));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "palette", 1 }, "Palette", paletteNames(), 3));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "quality", 1 }, "Render Quality", qualityNames(), 2));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "fps", 1 }, "Frame Rate", fpsNames(), 1));
    for (auto& d : kParams)
    {
        if (d.toggle)
            l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { d.id, 1 }, d.name, d.defV > .5f));
        else
        {
            juce::NormalisableRange<float> r (d.minV, d.maxV, d.step);
            l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { d.id, 1 }, d.name, r, d.defV));
        }
    }
    return l;
}

MonieProcessor::MonieProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MonieVisuals", createLayout()),
      fifoData ((size_t) fifoSize, 0.f)
{
    beatSensParam = apvts.getRawParameterValue ("beat");
    startAnalysis (currentSampleRate);
}

MonieProcessor::~MonieProcessor() { stopAnalysis(); }

void MonieProcessor::startAnalysis (double sr)
{
    stopAnalysis();
    fifo.reset();
    analysisThread = std::make_unique<AnalysisThread> (*this, sr);
    analysisThread->startThread (juce::Thread::Priority::normal);
}
void MonieProcessor::stopAnalysis()
{
    if (analysisThread != nullptr) { analysisThread->stopThread (2000); analysisThread.reset(); }
}

void MonieProcessor::prepareToPlay (double sr, int)
{
    currentSampleRate = sr;   // handles any sample-rate change: analyzer + FIFO are rebuilt off the audio thread
    startAnalysis (sr);
}

bool MonieProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto& in = l.getMainInputChannelSet();
    const auto& out = l.getMainOutputChannelSet();
    if (in != out) return false;
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

// AUDIO THREAD. The buffer is left untouched (pure passthrough). We only COPY a mono mix into a preallocated
// lock-free FIFO. No allocation, no locks, no I/O, no GPU access.
void MonieProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int nIn = getTotalNumInputChannels(), nOut = getTotalNumOutputChannels(), n = buffer.getNumSamples();
    for (int ch = nIn; ch < nOut; ++ch) buffer.clear (ch, 0, n);
    if (nIn == 0 || n == 0) return;

    const float* l = buffer.getReadPointer (0);
    const float* r = nIn > 1 ? buffer.getReadPointer (1) : l;
    int pos = 0;
    while (pos < n)
    {
        const int m = juce::jmin (n - pos, (int) scratch.size());
        for (int i = 0; i < m; ++i) scratch[(size_t) i] = 0.5f * (l[pos + i] + r[pos + i]);
        int s1, n1, s2, n2;
        fifo.prepareToWrite (m, s1, n1, s2, n2);     // if full, we simply drop samples (analysis only)
        if (n1 > 0) std::copy (scratch.begin(), scratch.begin() + n1, fifoData.begin() + s1);
        if (n2 > 0) std::copy (scratch.begin() + n1, scratch.begin() + n1 + n2, fifoData.begin() + s2);
        fifo.finishedWrite (n1 + n2);
        pos += m;
    }
}

juce::AudioProcessorEditor* MonieProcessor::createEditor() { return new MonieEditor (*this); }

// ---------------- state & presets ----------------
std::unique_ptr<juce::XmlElement> MonieProcessor::createStateXml()
{
    auto xml = std::make_unique<juce::XmlElement> ("MonieVisualsState");
    xml->setAttribute ("version", 1);
    if (auto p = apvts.copyState().createXml()) xml->addChildElement (p.release());
    auto* imgs = xml->createNewChildElement ("Images");
    imgs->setAttribute ("main", library.getMain());
    for (auto& p : library.getPaths()) imgs->createNewChildElement ("Image")->setAttribute ("path", p);
    return xml;
}

void MonieProcessor::restoreStateXml (const juce::XmlElement& xml)
{
    if (! xml.hasTagName ("MonieVisualsState")) return;
    if (auto* p = xml.getChildByName (apvts.state.getType().toString()))
        apvts.replaceState (juce::ValueTree::fromXml (*p));
    if (auto* imgs = xml.getChildByName ("Images"))
    {
        juce::StringArray paths;
        for (auto* e : imgs->getChildIterator()) paths.add (e->getStringAttribute ("path"));
        library.setFiles (paths, imgs->getIntAttribute ("main", 0));
    }
}

void MonieProcessor::getStateInformation (juce::MemoryBlock& dest) { if (auto x = createStateXml()) copyXmlToBinary (*x, dest); }
void MonieProcessor::setStateInformation (const void* data, int size)
{
    if (auto x = getXmlFromBinary (data, size)) restoreStateXml (*x);
}

void MonieProcessor::setParam (const juce::String& id, float v)
{
    if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v));
}

void MonieProcessor::resetAll()
{
    for (auto* p : getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (p))
            r->setValueNotifyingHost (r->getDefaultValue());
}

void MonieProcessor::applyFactoryPreset (int i)
{
    const auto& all = factoryPresets();
    if (! juce::isPositiveAndBelow (i, (int) all.size())) return;
    resetAll();
    const auto& fp = all[(size_t) i];
    setParam ("scene", (float) fp.scene);
    setParam ("palette", (float) fp.palette);
    for (auto& kv : fp.v) setParam (kv.first, kv.second);
}

void MonieProcessor::varyParams (unsigned seed, float amount)
{
    std::vector<monie::RParam> v;
    for (auto& d : kParams)
        if (auto* p = apvts.getRawParameterValue (d.id))
            v.push_back ({ d.id, d.minV, d.maxV, p->load(), d.toggle });   // toggles are locked: variation never flips them
    monie::vary (v, seed, amount);
    for (auto& r : v) setParam (r.id, r.value);
}

juce::File MonieProcessor::userPresetDir()
{
    auto d = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("monie visuals").getChildFile ("Presets");
    d.createDirectory();
    return d;
}
juce::Array<juce::File> MonieProcessor::getUserPresets() const
{
    return userPresetDir().findChildFiles (juce::File::findFiles, false, "*.mvpreset");
}
bool MonieProcessor::saveUserPreset (const juce::String& name)
{
    auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty()) return false;
    auto f = userPresetDir().getChildFile (clean).withFileExtension ("mvpreset");
    auto x = createStateXml();
    return x != nullptr && x->writeTo (f);
}
bool MonieProcessor::loadUserPreset (const juce::File& f)
{
    if (auto x = juce::XmlDocument::parse (f)) { restoreStateXml (*x); return true; }
    return false;
}
bool MonieProcessor::deleteUserPreset (const juce::File& f) { return f.existsAsFile() && f.moveToTrash(); }
bool MonieProcessor::duplicateUserPreset (const juce::File& f)
{
    return f.existsAsFile() && f.copyFileTo (f.getSiblingFile (f.getFileNameWithoutExtension() + " copy.mvpreset"));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MonieProcessor(); }
