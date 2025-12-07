#include "SampleRegistry.h"

SampleRegistry::SampleRegistry()
{
    formatManager.registerBasicFormats();
}

juce::String SampleRegistry::makeKey (const SampleRef& ref)
{
    return juce::String (static_cast<int> (ref.set)) + "-" + juce::String (static_cast<int> (ref.type)) + "-" + ref.file.getFullPathName();
}

void SampleRegistry::addSample (const SampleRef& ref)
{
    auto key = makeKey (ref);
    if (samples.find (key) != samples.end())
        return;

    if (! ref.file.existsAsFile())
        return;

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (ref.file));
    if (reader != nullptr)
    {
        samples[key] = { ref, std::move (reader) };
    }
}

juce::AudioFormatReader* SampleRegistry::getReader (const SampleRef& ref)
{
    auto key = makeKey (ref);
    auto it = samples.find (key);
    if (it != samples.end())
        return it->second.reader.get();
        
    // Fallback to default
    // Key for default: "Set-Type"
    juce::String defaultKey = juce::String (static_cast<int> (ref.set)) + "-" + juce::String (static_cast<int> (ref.type));
    auto defaultIt = defaultSamples.find (defaultKey);
    if (defaultIt != defaultSamples.end())
        return defaultIt->second.get();
        
    return nullptr;
}

void SampleRegistry::loadDefaultSamples (const juce::File& dir)
{
    // Map filenames to Set/Type
    // Filenames: "normal-hitnormal.wav", "soft-hitclap.wav", etc.
    // Sets: Normal=0, Soft=1, Drum=2 (Based on Enum)
    // Types: Normal=0, Whistle=3, Finish=2, Clap=1 (Based on Enum - WAIT, check Enum values)
    
    // Enum in SampleRef.h:
    // Set: Normal=0, Soft=1, Drum=2
    // Type: HitNormal=0, HitClap=1, HitFinish=2, HitWhistle=3
    
    struct DefaultDef { SampleSet set; SampleType type; juce::String filename; };
    std::vector<DefaultDef> defs = {
        { SampleSet::Normal, SampleType::HitNormal, "normal-hitnormal.wav" },
        { SampleSet::Normal, SampleType::HitWhistle, "normal-hitwhistle.wav" },
        { SampleSet::Normal, SampleType::HitFinish, "normal-hitfinish.wav" },
        { SampleSet::Normal, SampleType::HitClap, "normal-hitclap.wav" },
        
        { SampleSet::Soft, SampleType::HitNormal, "soft-hitnormal.wav" },
        { SampleSet::Soft, SampleType::HitWhistle, "soft-hitwhistle.wav" },
        { SampleSet::Soft, SampleType::HitFinish, "soft-hitfinish.wav" },
        { SampleSet::Soft, SampleType::HitClap, "soft-hitclap.wav" },
        
        { SampleSet::Drum, SampleType::HitNormal, "drum-hitnormal.wav" },
        { SampleSet::Drum, SampleType::HitWhistle, "drum-hitwhistle.wav" },
        { SampleSet::Drum, SampleType::HitFinish, "drum-hitfinish.wav" },
        { SampleSet::Drum, SampleType::HitClap, "drum-hitclap.wav" }
    };
    
    for (const auto& def : defs)
    {
        auto file = dir.getChildFile (def.filename);
        if (file.existsAsFile())
        {
            std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
            if (reader != nullptr)
            {
                juce::String key = juce::String (static_cast<int> (def.set)) + "-" + juce::String (static_cast<int> (def.type));
                defaultSamples[key] = std::move (reader);
            }
        }
    }
}
