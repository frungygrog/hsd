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

// Global ID generators for undo/redo matching
inline std::atomic<uint64_t> g_nextEventId{1};
inline std::atomic<uint64_t> g_nextTrackId{1};

struct Event
{
    uint64_t id = g_nextEventId++;
    double time;  // In seconds
    double volume = 1.0;
    ValidationState validationState = ValidationState::Valid;
};

// Represents a sample layer for grouping tracks
struct SampleLayer
{
    SampleSet bank;
    SampleType type;
};

struct Track
{
    uint64_t id = g_nextTrackId++;
    std::string name;

    // Sample metadata
    SampleSet sampleSet = SampleSet::Normal;
    SampleType sampleType = SampleType::HitNormal;
    std::string customFilename;

    // Composite layers (for groupings that play multiple samples)
    std::vector<SampleLayer> layers;

    // Playback properties
    double gain = 1.0;
    bool mute = false;
    bool solo = false;

    std::vector<Event> events;

    // Hierarchy
    std::vector<Track> children;
    bool isExpanded = false;
    int primaryChildIndex = 0;  // Default child for collapsed event placement

    // Flags
    bool isGrouping = false;
    bool isChildTrack = false;
};
