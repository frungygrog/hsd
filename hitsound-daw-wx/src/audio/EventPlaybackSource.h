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

    
    void setTracks (std::vector<Track>* tracks);
    
    
    
    void updateTracksSnapshot();
    
    void setTransportSource (juce::AudioTransportSource* transport);
    void setOffset(double offset);

private:
    SampleRegistry& sampleRegistry;
    
    
    std::vector<Track>* uiTracks { nullptr };
    
    
    std::vector<Track> tracksSnapshot;
    juce::SpinLock tracksLock;
    
    juce::AudioTransportSource* transportSource { nullptr };

    double currentSampleRate { 44100.0 };
    double offsetSeconds { 0.0 };
    float masterGain { 0.6f }; 
    
public:
    void setMasterGain(float gain) { masterGain = gain; }
    
private:
    
    struct Voice
    {
        juce::AudioFormatReader* reader;
        int64_t position;
        double speedRatio;
        float gain;
        int startOffset; 
    };
    
    std::vector<Voice> activeVoices;
};
