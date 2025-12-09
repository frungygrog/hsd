#pragma once
#include <vector>
#include "SampleTypes.h"

// Intermediate structure used during .osu file parsing
struct HitObject
{
    double timestamp;
    SampleSet baseSampleSet;
    SampleSet additionSampleSet;

    bool hasWhistle = false;
    bool hasFinish = false;
    bool hasClap = false;

    // Per-addition bank overrides
    SampleSet whistleBank = SampleSet::Normal;
    SampleSet finishBank = SampleSet::Normal;
    SampleSet clapBank = SampleSet::Normal;

    // Returns true if all active additions use the same bank
    bool isValid() const;
    void setAdditionBank(SampleSet set);
};