#include "HitObject.h"

bool HitObject::isValid() const
{
    // Collect all active addition banks
    std::vector<SampleSet> usedBanks;
    if (hasWhistle) usedBanks.push_back(whistleBank);
    if (hasFinish) usedBanks.push_back(finishBank);
    if (hasClap) usedBanks.push_back(clapBank);

    if (usedBanks.empty()) return true;

    // All additions must use the same bank
    SampleSet first = usedBanks[0];
    for (size_t i = 1; i < usedBanks.size(); ++i)
    {
        if (usedBanks[i] != first) return false;
    }
    return true;
}

void HitObject::setAdditionBank(SampleSet set)
{
    whistleBank = set;
    finishBank = set;
    clapBank = set;
}
