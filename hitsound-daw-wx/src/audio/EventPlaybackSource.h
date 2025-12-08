#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "../model/Track.h"
#include "SampleRegistry.h"

class EventPlaybackSource : public juce::AudioSource
{
public:
    EventPlaybackSource (SampleRegistry& registry);
    ~EventPlaybackSource() override;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;

    // Call from UI thread to provide a pointer to the live tracks (for legacy compatibility)
    void setTracks (std::vector<Track>* tracks);
    
    // Call from UI thread to safely update the audio thread's snapshot
    // This should be called after any track modifications (add/remove events, etc.)
    void updateTracksSnapshot();
    
    void setTransportSource (juce::AudioTransportSource* transport);
    void setOffset(double offset);

private:
    SampleRegistry& sampleRegistry;
    
    // UI thread's live tracks pointer (used as source for snapshots)
    std::vector<Track>* uiTracks { nullptr };
    
    // Thread-safe snapshot that audio thread reads from
    std::vector<Track> tracksSnapshot;
    juce::SpinLock tracksLock;
    
    juce::AudioTransportSource* transportSource { nullptr };

    double currentSampleRate { 44100.0 };
    double offsetSeconds { 0.0 };
    float masterGain { 0.6f }; // Default 60% for effects
    
public:
    void setMasterGain(float gain) { masterGain = gain; }
    
private:
    
    struct Voice
    {
        juce::AudioFormatReader* reader;
        int64_t position;
        double speedRatio;
        float gain;
        int startOffset; // Delay in samples within the current block
    };
    
    std::vector<Voice> activeVoices;
};
