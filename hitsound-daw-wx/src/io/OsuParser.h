#pragma once
#include <string>
#include <vector>
#include "../model/Project.h"


#include <juce_core/juce_core.h>

class OsuParser
{
public:
    static Project parse(const juce::File& file);
    static bool CreateHitsoundDiff(const juce::File& referenceFile, const juce::File& targetFile);
};
