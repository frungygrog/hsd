#pragma once
#include <vector>
#include "Track.h"

struct Project
{
    std::vector<Track> tracks;
    

    struct TimingPoint {
        double time;
        double beatLength; 
        int sampleSet; 
        double volume;
        bool uninherited;
    };
    std::vector<TimingPoint> timingPoints;

    double bpm = 120.0;
    double offset = 0.0;
    
    
    std::string artist;
    std::string title;
    std::string version; 
    std::string creator;  
    
    SampleSet defaultSampleSet = SampleSet::Normal; 
    
    std::string audioFilename;
    std::string projectDirectory;
    std::string projectFilePath;
};
