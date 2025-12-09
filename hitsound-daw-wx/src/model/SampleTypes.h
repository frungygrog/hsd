#pragma once

enum class SampleSet
{
    Normal,
    Soft,
    Drum
};

enum class SampleType
{
    // Currently used sample types for hit objects
    HitNormal,
    HitWhistle,
    HitFinish,
    HitClap,
    
    // Reserved for future slider hitsounding support
    // These are defined for osu! format compatibility but not yet implemented
    SliderSlide,
    SliderTick,
    SliderWhistle
};
