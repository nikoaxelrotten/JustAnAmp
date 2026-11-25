#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

// Presets de fabrica. El 0 es "Init": todos los parametros en su valor por defecto.
namespace jah
{
struct PV { const char* id; float v; };
struct Preset { const char* name; std::vector<PV> v; };

inline const std::vector<Preset>& presets()
{
    // head: 0 Pulga, 1 NotFieldy, 2 RoAguMiSSTruVe, 3 Ryandemar, 4 Canciller, 5 TimmyC | pedal: 0 Off, 1 SansAmp, 2 Darkglass
    // ctype: 0 Off, 1 Opto, 2 FET | mod: 1 Chorus, 2 Flanger, 3 Phaser, 4 Octaver, 5 Env Filter
    static const std::vector<Preset> p = {
        { "Init", {} },

        { "Pulga - Slap Funk", { {"head",0}, {"gain",4.5f}, {"bass",6}, {"mid",4}, {"himid",6.5f}, {"treble",7}, {"contour",5}, {"master",-6}, {"ctype",2}, {"cthr",-20}, {"cratio",6}, {"catk",3}, {"crel",120}, {"cgain",3}, {"rmix",8}, {"ron",1} } },
        { "Pulga - Punk Drive", { {"head",0}, {"pedal",2}, {"pdrive",4}, {"pblend",5}, {"pbass",6}, {"ptreble",6}, {"pextra",5}, {"gain",6}, {"bass",6}, {"mid",5}, {"himid",6}, {"treble",6}, {"contour",2}, {"master",-6}, {"ctype",2}, {"cthr",-18}, {"cratio",8}, {"catk",5}, {"crel",150}, {"cgain",4} } },
        { "Pulga - Clean DI", { {"head",0}, {"gain",3}, {"bass",5.5f}, {"mid",5}, {"himid",5}, {"treble",5.5f}, {"master",-4}, {"ctype",1}, {"cthr",-22}, {"cblend",70}, {"cgain",3}, {"chon",1}, {"chrate",2.5f}, {"chdepth",3}, {"chmix",25} } },

        { "NotFieldy - Clack Slap", { {"head",1}, {"gain",6.5f}, {"bass",5}, {"mid",3.5f}, {"treble",7}, {"bright",1}, {"master",-6}, {"eq1",1}, {"eq2",2}, {"eq3",-3}, {"eq4",-5}, {"eq5",2}, {"eq6",6}, {"eq7",5}, {"ctype",2}, {"cthr",-22}, {"cratio",8}, {"catk",2}, {"crel",110}, {"cgain",4}, {"gthr",-50} } },
        { "NotFieldy - Down Grind", { {"head",1}, {"pedal",2}, {"pdrive",6}, {"pblend",6}, {"pbass",5}, {"ptreble",6}, {"pextra",6}, {"gain",7.5f}, {"bass",6}, {"mid",4}, {"treble",6}, {"master",-8}, {"eq2",3}, {"eq4",-4}, {"eq6",4}, {"ctype",2}, {"cthr",-20}, {"cratio",10}, {"catk",4}, {"crel",140}, {"cgain",4}, {"gthr",-45} } },
        { "NotFieldy - Clean DI", { {"head",1}, {"gain",3.5f}, {"bass",5.5f}, {"mid",5}, {"treble",6}, {"master",-4}, {"eq3",-1}, {"eq6",2}, {"ctype",1}, {"cthr",-22}, {"cblend",75}, {"cgain",3} } },

        { "RoAgu - Rock Round", { {"head",2}, {"gain",5.5f}, {"bass",6}, {"mid",5}, {"freq",1}, {"treble",5}, {"master",-6}, {"ulo",0}, {"ctype",1}, {"cthr",-20}, {"cblend",85}, {"cgain",3}, {"rmix",8}, {"ron",1} } },
        { "RoAgu - Funk Metal", { {"head",2}, {"pedal",1}, {"pdrive",4}, {"pblend",4}, {"pbass",5}, {"ptreble",6}, {"pextra",6}, {"gain",6.5f}, {"bass",6}, {"mid",6}, {"freq",2}, {"treble",6}, {"uhi",10}, {"master",-7}, {"ctype",2}, {"cthr",-20}, {"cratio",6}, {"catk",5}, {"crel",160}, {"cgain",3} } },
        { "RoAgu - Sub Heavy", { {"head",2}, {"gain",4.5f}, {"bass",7.5f}, {"mid",4}, {"freq",0}, {"treble",4}, {"ulo",10}, {"master",-6}, {"ctype",1}, {"cthr",-24}, {"cblend",90}, {"cgain",4} } },

        { "Ryandemar - Modern Metal", { {"head",3}, {"pedal",2}, {"pdrive",5}, {"pblend",5}, {"pbass",5}, {"ptreble",6}, {"pextra",6}, {"gain",7}, {"ulo",4}, {"bass",5.5f}, {"treble",6}, {"mid",4.5f}, {"freq",2}, {"uhi",5}, {"master",-7}, {"ctype",2}, {"cthr",-20}, {"cratio",8}, {"catk",4}, {"crel",130}, {"cgain",4}, {"gthr",-48} } },
        { "Ryandemar - Clean Fingers", { {"head",3}, {"gain",3.5f}, {"ulo",2}, {"bass",5.5f}, {"treble",5}, {"mid",5}, {"freq",1}, {"uhi",3}, {"master",-4}, {"ctype",1}, {"cthr",-22}, {"cblend",80}, {"cgain",3}, {"rmix",10}, {"ron",1} } },
        { "Ryandemar - Octave Fuzz", { {"head",3}, {"pedal",1}, {"pdrive",7}, {"pblend",6}, {"pbass",5}, {"ptreble",5}, {"pextra",5}, {"gain",5.5f}, {"bass",6}, {"mid",5}, {"freq",1}, {"treble",5}, {"master",-7}, {"ocon",1}, {"ocdown",6}, {"ocup",3}, {"octone",3}, {"ocdirect",60} } },

        { "Canciller - Dual Rig", { {"head",4}, {"pedal",2}, {"pdrive",4}, {"pblend",5}, {"pbass",5}, {"ptreble",6}, {"pextra",5}, {"gain",6}, {"bass",5.5f}, {"mid",4.5f}, {"himid",6.5f}, {"treble",5.5f}, {"contour",4}, {"dirty",4.5f}, {"master",-6}, {"ctype",1}, {"cthr",-22}, {"cblend",85}, {"cgain",3}, {"dmix",15}, {"dtime",375}, {"flon",1}, {"flrate",2}, {"fldepth",4}, {"flmix",20}, {"don",1} } },
        { "Canciller - Clean Rig", { {"head",4}, {"gain",4}, {"bass",5.5f}, {"mid",5}, {"himid",6}, {"treble",5.5f}, {"contour",3}, {"dirty",0}, {"master",-5}, {"ctype",1}, {"cthr",-22}, {"cblend",80}, {"cgain",3} } },
        { "Canciller - Gnarly", { {"head",4}, {"pedal",2}, {"pdrive",6}, {"pblend",6}, {"pbass",5}, {"ptreble",6}, {"pextra",6}, {"gain",7.5f}, {"bass",5.5f}, {"mid",4}, {"himid",6}, {"treble",6}, {"contour",7}, {"dirty",9}, {"master",-7}, {"ctype",2}, {"cthr",-20}, {"cratio",6}, {"catk",4}, {"crel",140}, {"cgain",3}, {"gthr",-48} } },

        { "TimmyC - Rage Dual", { {"head",5}, {"pedal",2}, {"pdrive",4}, {"pblend",5}, {"pbass",5}, {"ptreble",6}, {"pextra",5}, {"gain",5.5f}, {"bass",6}, {"mid",5}, {"freq",1}, {"treble",5}, {"ulo",10}, {"dirty",5}, {"master",-6}, {"ctype",2}, {"cthr",-20}, {"cratio",6}, {"catk",5}, {"crel",150}, {"cgain",3} } },
        { "TimmyC - Clean SVT", { {"head",5}, {"gain",4}, {"bass",6}, {"mid",5}, {"freq",1}, {"treble",5}, {"dirty",0}, {"master",-5}, {"ctype",1}, {"cthr",-22}, {"cblend",85}, {"cgain",3} } },
        { "TimmyC - Full Distortion", { {"head",5}, {"pedal",1}, {"pdrive",5}, {"pblend",5}, {"pbass",5}, {"ptreble",5}, {"pextra",5}, {"gain",8}, {"bass",6.5f}, {"mid",4}, {"freq",2}, {"treble",6}, {"ulo",8}, {"uhi",6}, {"dirty",10}, {"master",-8}, {"ctype",2}, {"cthr",-18}, {"cratio",8}, {"catk",4}, {"crel",130}, {"cgain",4}, {"gthr",-48} } },

        { "Especial - Octave Sub", { {"head",2}, {"gain",4}, {"bass",6}, {"mid",4}, {"freq",0}, {"treble",4}, {"master",-6}, {"ctype",1}, {"cthr",-22}, {"cblend",80}, {"cgain",3}, {"ocon",1}, {"ocdown",7}, {"ocup",0}, {"octone",3}, {"ocdirect",55} } },
        { "Especial - Envelope Funk", { {"head",0}, {"gain",4}, {"bass",5.5f}, {"mid",5}, {"himid",6}, {"treble",6}, {"master",-6}, {"ctype",2}, {"cthr",-20}, {"cratio",6}, {"catk",5}, {"crel",150}, {"cgain",3}, {"efon",1}, {"efsens",5}, {"efrange",6}, {"efres",5}, {"efmix",85} } },
    };
    return p;
}

inline int presetCount() { return (int) presets().size(); }
inline juce::String presetName (int i) { return presets()[(size_t) juce::jlimit (0, presetCount() - 1, i)].name; }

inline void applyPreset (juce::AudioProcessorValueTreeState& apvts, int index)
{
    index = juce::jlimit (0, presetCount() - 1, index);
    for (auto* prm : apvts.processor.getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
            r->setValueNotifyingHost (r->getDefaultValue());
    for (const auto& pv : presets()[(size_t) index].v)
        if (auto* r = apvts.getParameter (pv.id))
            r->setValueNotifyingHost (r->convertTo0to1 (pv.v));
}
}
