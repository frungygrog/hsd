#pragma once
#include <vector>
#include "SampleTypes.h"

struct HitObject
{
    double timestamp;
    
    
    SampleSet baseSampleSet;
    
    
    SampleSet additionSampleSet; 
    
    bool hasWhistle = false;
    bool hasFinish = false;
    bool hasClap = false;

    
    SampleSet whistleBank = SampleSet::Normal;
    SampleSet finishBank = SampleSet::Normal;
    SampleSet clapBank = SampleSet::Normal;
    
    
    bool isValid() const;
    
    
    void setAdditionBank(SampleSet set);
};