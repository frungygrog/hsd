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
    {
        DBG("SampleRegistry: Sample file not found: " + ref.file.getFullPathName());
        return;
    }

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (ref.file));
    if (reader != nullptr)
    {
        samples[key] = { ref, std::move (reader) };
        DBG("SampleRegistry: Loaded sample: " + ref.file.getFileName());
    }
    else
    {
        DBG("SampleRegistry: Failed to create reader for: " + ref.file.getFileName());
    }
}

juce::AudioFormatReader* SampleRegistry::getReader (const SampleRef& ref)
{
    auto key = makeKey (ref);
    auto it = samples.find (key);
    if (it != samples.end())
        return it->second.reader.get();
        
    
    
    juce::String defaultKey = juce::String (static_cast<int> (ref.set)) + "-" + juce::String (static_cast<int> (ref.type));
    auto defaultIt = defaultSamples.find (defaultKey);
    if (defaultIt != defaultSamples.end())
        return defaultIt->second.get();
        
    return nullptr;
}

void SampleRegistry::loadDefaultSamples (const juce::File& dir)
{
    
    
    
    
    
    
    
    
    
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
                DBG("SampleRegistry: Loaded default sample: " + def.filename);
            }
            else
            {
                DBG("SampleRegistry: Failed to load default sample: " + def.filename);
            }
        }
        else
        {
            DBG("SampleRegistry: Default sample not found: " + def.filename);
        }
    }
}
