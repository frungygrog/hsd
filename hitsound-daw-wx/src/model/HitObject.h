#pragma once
#include <vector>
#include "SampleTypes.h"

struct HitObject
{
    double timestamp;
    
    // The "Base" sample set (HitNormal)
    SampleSet baseSampleSet;
    
    // The "Addition" sample set (Bank for Whistle, Finish, Clap)
    SampleSet additionSampleSet; 
    
    bool hasWhistle = false;
    bool hasFinish = false;
    bool hasClap = false;

    // Per-addition banks for validation
    SampleSet whistleBank = SampleSet::Normal;
    SampleSet finishBank = SampleSet::Normal;
    SampleSet clapBank = SampleSet::Normal;
    
    // Checks if all active additions use the same bank
    bool isValid() const;
    
    // Sets all addition banks to the same value
    void setAdditionBank(SampleSet set);
};