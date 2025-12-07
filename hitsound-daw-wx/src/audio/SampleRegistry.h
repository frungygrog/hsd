#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <map>
#include "../model/SampleRef.h"

class SampleRegistry
{
public:
    SampleRegistry();

    void addSample (const SampleRef& ref);
    juce::AudioFormatReader* getReader (const SampleRef& ref);
    
    void loadDefaultSamples (const juce::File& dir);
    int getDefaultSampleCount() const { return (int)defaultSamples.size(); }

private:
    struct SampleEntry
    {
        SampleRef ref;
        std::unique_ptr<juce::AudioFormatReader> reader;
    };

    std::map<juce::String, SampleEntry> samples;
    
    // Default samples storage: Key = "Set-Type" (e.g. "1-2")
    std::map<juce::String, std::unique_ptr<juce::AudioFormatReader>> defaultSamples;
    
    juce::AudioFormatManager formatManager;

    static juce::String makeKey (const SampleRef& ref);
};
