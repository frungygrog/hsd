#pragma once
#include <string>
#include <vector>
#include "../model/Project.h"

#include <juce_core/juce_core.h>

class OsuParser
{
public:
    static Project parse(const juce::File& file);
    
    // Creates a new hitsound difficulty by copying timing points from reference
    static bool CreateHitsoundDiff(const juce::File& referenceFile, const juce::File& targetFile);
};
