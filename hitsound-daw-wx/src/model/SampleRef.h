#pragma once

#include <juce_core/juce_core.h>
#include "SampleTypes.h"

struct SampleRef
{
    SampleSet set{ SampleSet::Normal };
    SampleType type{ SampleType::HitNormal };
    float volumePercent{ 100.0f };
    juce::File file;
};
