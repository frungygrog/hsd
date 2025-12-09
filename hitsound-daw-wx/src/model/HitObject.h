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

    // Validation Logic
    // "Additions have to remain in the same bank" - this is enforced by having a single `additionSampleSet` field.
    // If the data structure forced separate banks for whistle/finish/clap, we'd need a check.
    // Since we store ONE addition bank, we implicitly enforce usually.
    // However, if we are importing from a format that allows distinct banks, we flag it.
    // BUT the user said: "soft-hitnormal with normal-hitwhistle and normal-hitclap is ok." (Base Soft, Additions Normal)
    // "soft-hitnormal with normal-hitwhistle and drum-hitclap is not ok." (Base Soft, Whistle Normal, Clap Drum -> Mixed Additions)
    
    // Implementation Detail: In Osu, the "Addition" sets are actually defined by the filename usually or the timing point.
    // To represent the INVALID state, we effectively need to support the possibility of mixed addition banks in the model,
    // so that we can detect and flag it.
    
    SampleSet whistleBank = SampleSet::Normal;
    SampleSet finishBank = SampleSet::Normal;
    SampleSet clapBank = SampleSet::Normal;
    
    // If these differ, it's invalid?
    // User said: "soft-hitnormal with normal-hitwhistle and drum-hitclap is not ok"
    
    bool isValid() const
    {
        // Collect active addition banks
        std::vector<SampleSet> usedBanks;
        if (hasWhistle) usedBanks.push_back(whistleBank);
        if (hasFinish) usedBanks.push_back(finishBank);
        if (hasClap) usedBanks.push_back(clapBank);
        
        if (usedBanks.empty()) return true;
        
        SampleSet first = usedBanks[0];
        for (size_t i = 1; i < usedBanks.size(); ++i)
        {
            if (usedBanks[i] != first) return false;
        }
        return true;
    }
    
    // Helper to unify
    void setAdditionBank(SampleSet set)
    {
        whistleBank = set;
        finishBank = set;
        clapBank = set;
    }
};