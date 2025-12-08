#pragma once
#include <string>
#include <vector>
#include <optional>
#include "SampleTypes.h"

// Tri-state validation for events
enum class ValidationState {
    Valid,      // Normal state (blue) - properly backed hitsound
    Invalid,    // Error state (red) - e.g., conflicting addition banks
    Warning     // Warning state (pink) - e.g., addition without hitnormal backing
};

struct Event
{
    double time; // Seconds or Milliseconds? Osu is ms. Let's use seconds internally for audio? Or ms for DAW UI?
                 // JUCE Audio usually uses samples or seconds.
                 // Osu files are integer ms.
                 // Let's use double seconds for precision in DAW.
    
    double volume = 1.0;
    ValidationState validationState = ValidationState::Valid;
    // Maybe velocity or other params
};

// Composite Layer Definition
struct SampleLayer
{
    SampleSet bank;
    SampleType type;
//  float volumeScale = 1.0f; // Could add later, user said same volume for all additions
};

struct Track
{
    std::string name;
    
    // Metadata for validation and playback
    SampleSet sampleSet = SampleSet::Normal;
    SampleType sampleType = SampleType::HitNormal;
    std::string customFilename; // If overridden
    
    // Composite Layers (If not empty, these override or augment behavior)
    std::vector<SampleLayer> layers;
    
    // Playback properties
    double gain = 1.0;
    bool mute = false;
    bool solo = false;
    
    std::vector<Event> events;
    
    // Hierarchy (for UI collapsible tracks)
    // Parent tracks might aggregated volume controls or types.
    std::vector<Track> children;
    bool isExpanded = true;
    
    // For drag-and-drop routing when collapsed
    int primaryChildIndex = 0;
    
    // Grouping flag (Composite track)
    bool isGrouping = false;
};


