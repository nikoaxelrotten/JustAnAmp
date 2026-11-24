#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

static float sanitize (float v)
{
    if (! std::isfinite (v)) return 0.0f;
    return juce::jlimit (-4.0f, 4.0f, v);
}

//==============================================================================
APVTS::ParameterLayout JustAnAmpProcessor::createParameterLayout()
{
    using namespace juce;
    APVTS::ParameterLayout layout;

    auto addFloat = [&layout] (const char* id, const char* name, float min, float max, float def)
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
            NormalisableRange<float> (min, max, 0.1f), def,
            AudioParameterFloatAttributes().withLabel (name)));
    };

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "head", 1 }, "Head",
        StringArray { "Pulga", "NotFieldy", "RoAguMiSSTruVe", "Ryandemar", "Canciller", "TimmyC" }, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "pedal", 1 }, "Pedal",
        StringArray { "Off", "SansAmp", "Darkglass" }, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "ctype", 1 }, "Compressor",
        StringArray { "Off", "Opto", "FET" }, 0));

    // Head (escala 0 a 10, 5 = plano)
    addFloat ("gain",    "Gain",     0.0f, 10.0f, 5.0f);
    addFloat ("bass",    "Bass",     0.0f, 10.0f, 5.0f);
    addFloat ("mid",     "Mid",      0.0f, 10.0f, 5.0f);
    addFloat ("himid",   "Hi Mid",   0.0f, 10.0f, 5.0f);
    addFloat ("treble",  "Treble",   0.0f, 10.0f, 5.0f);
    addFloat ("contour", "Contour",  0.0f, 10.0f, 0.0f);
    addFloat ("dirty",   "Dirty Mix", 0.0f, 10.0f, 4.0f);
    addFloat ("ulo",     "Ultra Lo", 0.0f, 10.0f, 0.0f);
    addFloat ("uhi",     "Ultra Hi", 0.0f, 10.0f, 0.0f);
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "freq", 1 }, "Mid Frequency",
        NormalisableRange<float> (0.0f, 4.0f, 1.0f), 2.0f));
    for (int b = 1; b <= 7; ++b)
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ("eq" + String (b)).toRawUTF8(), 1 }, "EQ " + String (b),
            NormalisableRange<float> (-12.0f, 12.0f, 0.5f), 0.0f));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "bright", 1 }, "Bright", false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "boost", 1 }, "Boost", false));
    addFloat ("master",  "Master",   -24.0f, 12.0f, 0.0f);

    // Pedal
    addFloat ("pdrive",  "Pedal Drive",    0.0f, 10.0f, 4.0f);
    addFloat ("pblend",  "Pedal Blend",    0.0f, 10.0f, 5.0f);
    addFloat ("plevel",  "Pedal Level",  -24.0f, 12.0f, 0.0f);
    addFloat ("pbass",   "Pedal Bass",     0.0f, 10.0f, 5.0f);
    addFloat ("ptreble", "Pedal Treble",   0.0f, 10.0f, 5.0f);
    addFloat ("pextra",  "Pedal Presence/Grunt", 0.0f, 10.0f, 5.0f);

    // --- Efectos post-head ---
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "mod", 1 }, "Mod",
        StringArray { "Off", "Chorus", "Flanger", "Phaser", "Octaver", "Env Filter" }, 0));

    auto addFx = [&layout] (const char* id, const char* name, float min, float max, float def, float step,
                            float skew, const String& unit)
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
            NormalisableRange<float> (min, max, step, skew), def,
            AudioParameterFloatAttributes()
                .withStringFromValueFunction ([unit, step] (float v, int)
                    { return String (step >= 1.0f ? String ((int) std::round (v)) : String (v, 1)) + unit; })
                .withValueFromStringFunction ([] (const String& t) { return t.getFloatValue(); })));
    };
    addFx ("mrate",  "Mod Rate",     0.0f, 10.0f,   3.0f, 0.1f, 1.0f, "");
    addFx ("mdepth", "Mod Depth",    0.0f, 10.0f,   6.0f, 0.1f, 1.0f, "");
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "fxorder", 1 }, "FX Order",
        StringArray { "Mod-Delay-Reverb", "Mod-Reverb-Delay", "Delay-Mod-Reverb", "Delay-Reverb-Mod", "Reverb-Mod-Delay", "Reverb-Delay-Mod" }, 0));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "don", 1 }, "Delay On", true));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "ron", 1 }, "Reverb On", true));
    addFx ("mshape", "Mod Shape",    0.0f, 10.0f,   3.0f, 0.1f, 1.0f, "");
    addFx ("gthr",   "Gate Threshold", -80.0f, -20.0f, -80.0f, 0.5f, 1.0f, " dB");
    addFx ("grel",   "Gate Release", 10.0f, 500.0f,  80.0f, 1.0f, 0.5f, " ms");
    addFx ("cthr",   "Comp Threshold", -40.0f, 0.0f, -18.0f, 0.5f, 1.0f, " dB");
    addFx ("cratio", "Comp Ratio",    1.5f, 20.0f,   4.0f, 0.1f, 0.6f, ":1");
    addFx ("catk",   "Comp Attack",   0.1f, 60.0f,  10.0f, 0.1f, 0.4f, " ms");
    addFx ("crel",   "Comp Release",  20.0f, 1200.0f, 200.0f, 1.0f, 0.5f, " ms");
    addFx ("cblend", "Comp Blend",    0.0f, 100.0f, 100.0f, 1.0f, 1.0f, "%");
    addFx ("cgain",  "Comp Makeup",  -6.0f, 18.0f,   0.0f, 0.5f, 1.0f, " dB");
    addFx ("mmix",   "Mod Mix",      0.0f, 100.0f, 50.0f, 1.0f, 1.0f, "%");
    addFx ("dtime",  "Delay Time",  40.0f, 1200.0f, 380.0f, 1.0f, 0.5f, " ms");
    addFx ("dfb",    "Delay Feedback", 0.0f, 95.0f, 35.0f, 1.0f, 1.0f, "%");
    addFx ("dmix",   "Delay Mix",    0.0f, 100.0f,  0.0f, 1.0f, 1.0f, "%");
    addFx ("rsize",  "Reverb Size",  0.0f, 10.0f,   4.0f, 0.1f, 1.0f, "");
    addFx ("rdamp",  "Reverb Damp",  0.0f, 10.0f,   5.0f, 0.1f, 1.0f, "");
    addFx ("rmix",   "Reverb Mix",   0.0f, 100.0f,  0.0f, 1.0f, 1.0f, "%");

    return layout;
}

//==============================================================================
JustAnAmpProcessor::JustAnAmpProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

void JustAnAmpProcessor::prepareToPlay (double sr, int samplesPerBlock)
{
    engine.prepare (sr, samplesPerBlock);
    fx.prepare (sr);
    gate.prepare (sr);
    comp.prepare (sr);
    setLatencySamples (engine.getLatency());
    mono.assign ((size_t) juce::jmax (1, samplesPerBlock), 0.0f);
    master.reset (sr, 0.02);
    master.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("master")->load()));
}

bool JustAnAmpProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
}

void JustAnAmpProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int total = buffer.getNumSamples();
    if (total <= 0 || buffer.getNumChannels() < 2) return;

    auto param = [this] (const char* id) -> float { return apvts.getRawParameterValue (id)->load(); };

    jah::Params p;
    p.head = (int) param ("head");
    p.pedal = (int) param ("pedal");
    p.gain = param ("gain");
    p.bass = param ("bass");
    p.mid = param ("mid");
    p.himid = param ("himid");
    p.treble = param ("treble");
    p.contour = param ("contour");
    p.dirty = param ("dirty");
    p.ulo = param ("ulo");
    p.uhi = param ("uhi");
    p.freq = param ("freq");
    for (int b = 0; b < 7; ++b) p.eq[b] = param (("eq" + juce::String (b + 1)).toRawUTF8());
    p.bright = param ("bright") > 0.5f;
    p.boost = param ("boost") > 0.5f;
    p.pdrive = param ("pdrive");
    p.pblend = param ("pblend");
    p.plevelDb = param ("plevel");
    p.pbass = param ("pbass");
    p.ptreble = param ("ptreble");
    p.pextra = param ("pextra");
    engine.setParams (p);

    jah::FxParams fp;
    fp.mod = (int) param ("mod");
    fp.mrate = param ("mrate");
    fp.mdepth = param ("mdepth");
    fp.mmix = param ("mmix") / 100.0f;
    fp.mshape = param ("mshape");
    const float gateThr = param ("gthr"), gateRel = param ("grel");
    const int ctype = (int) param ("ctype");
    const double cthr = param ("cthr"), cratio = param ("cratio"), catk = param ("catk"), crel = param ("crel"),
                 cblend = param ("cblend") / 100.0, cgain = param ("cgain");
    fp.dtimeMs = param ("dtime");
    fp.dfb = param ("dfb") / 100.0f;
    fp.dmix = param ("don") > 0.5f ? param ("dmix") / 100.0f : 0.0f;
    fp.rsize = param ("rsize");
    fp.rdamp = param ("rdamp");
    fp.rmix = param ("ron") > 0.5f ? param ("rmix") / 100.0f : 0.0f;
    {
        static const int perms[6][3] = { {0,1,2}, {0,2,1}, {1,0,2}, {1,2,0}, {2,0,1}, {2,1,0} };
        const int o = juce::jlimit (0, 5, (int) param ("fxorder"));
        for (int k = 0; k < 3; ++k) fp.order[k] = perms[o][k];
    }
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto bpm = pos->getBpm()) fp.bpm = *bpm;
            fp.playing = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition()) { fp.ppq = *q; fp.hasPpq = true; }
        }
    const double fsHost = getSampleRate();
    master.setTargetValue (juce::Decibels::decibelsToGain (param ("master")));

    float* l = buffer.getWritePointer (0);
    float* r = buffer.getWritePointer (1);
    const int chunk = (int) mono.size();

    // Un amp es mono: se suma la entrada, se procesa una vez y se copia a los dos canales.
    for (int start = 0; start < total; start += chunk)
    {
        const int n = juce::jmin (chunk, total - start);
        float inPk = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            mono[(size_t) i] = 0.5f * (l[start + i] + r[start + i]);
            inPk = std::max (inPk, std::abs (mono[(size_t) i]));
        }

        gate.process (mono.data(), n, gateThr, gateRel);
        comp.process (mono.data(), n, ctype, cthr, cratio, catk, crel, cblend, cgain);
        engine.process (mono.data(), n);

        jah::FxParams fpc = fp;
        if (fpc.hasPpq && fsHost > 0.0) fpc.ppq += (double) start / fsHost * fp.bpm / 60.0;
        fx.process (mono.data(), l + start, r + start, n, fpc);

        float outPk = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            const float g = master.getNextValue();
            l[start + i] = sanitize (l[start + i] * g);
            r[start + i] = sanitize (r[start + i] * g);
            outPk = std::max (outPk, std::max (std::abs (l[start + i]), std::abs (r[start + i])));
        }
        // medidores: pico con caida suave
        const float decay = (float) std::exp (-(double) n / (std::max (8000.0, fsHost) * 0.3));
        meterIn.store (std::max (inPk, meterIn.load() * decay));
        meterOut.store (std::max (outPk, meterOut.load() * decay));
        meterGr.store ((float) comp.lastGainReduction());
    }
}

void JustAnAmpProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void JustAnAmpProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml.get() != nullptr)
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* JustAnAmpProcessor::createEditor()
{
    return new JustAnAmpEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JustAnAmpProcessor();
}
