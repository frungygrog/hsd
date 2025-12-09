#pragma once
#include <vector>
#include "Track.h"

struct Project
{
    std::vector<Track> tracks;
    
// Metadata
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
    
    // Original file info
    std::string artist;
    std::string title;
    std::string version; // Difficulty name
    std::string creator;  // Mapper name
    
    SampleSet defaultSampleSet = SampleSet::Normal; // From [General] SampleSet
    
    std::string audioFilename;
    std::string projectDirectory;
    std::string projectFilePath;
};
