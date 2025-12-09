#pragma once
#include <vector>
#include "Track.h"

struct Project
{
    std::vector<Track> tracks;

    struct TimingPoint {
        double time;       // Milliseconds from start
        double beatLength; // Milliseconds per beat (BPM = 60000 / beatLength)
        int sampleSet;     // 0=auto, 1=normal, 2=soft, 3=drum
        double volume;     // 0-100
        bool uninherited;  // True = red line (BPM), false = green line (SV)
    };
    std::vector<TimingPoint> timingPoints;

    double bpm = 120.0;
    double offset = 0.0;

    // Metadata
    std::string artist;
    std::string title;
    std::string version;  // Difficulty name
    std::string creator;

    SampleSet defaultSampleSet = SampleSet::Normal;

    // File paths
    std::string audioFilename;
    std::string projectDirectory;
    std::string projectFilePath;
};
