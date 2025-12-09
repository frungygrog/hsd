#pragma once
#include <string>
#include <vector>
#include <optional>
#include <atomic>
#include "SampleTypes.h"


enum class ValidationState {
    Valid,      
    Invalid,    
    Warning     
};


inline std::atomic<uint64_t> g_nextEventId{1};
inline std::atomic<uint64_t> g_nextTrackId{1};

struct Event
{
    uint64_t id = g_nextEventId++;  
    
    double time;  
    
    double volume = 1.0;
    ValidationState validationState = ValidationState::Valid;
    
};


struct SampleLayer
{
    SampleSet bank;
    SampleType type;

};

struct Track
{
    uint64_t id = g_nextTrackId++;  
    
    std::string name;
    
    
    SampleSet sampleSet = SampleSet::Normal;
    SampleType sampleType = SampleType::HitNormal;
    std::string customFilename; 
    
    
    std::vector<SampleLayer> layers;
    
    
    double gain = 1.0;
    bool mute = false;
    bool solo = false;
    
    std::vector<Event> events;
    
    
    
    std::vector<Track> children;
    bool isExpanded = false;
    
    
    int primaryChildIndex = 0;
    
    
    bool isGrouping = false;
    
    
    
    bool isChildTrack = false;
};


