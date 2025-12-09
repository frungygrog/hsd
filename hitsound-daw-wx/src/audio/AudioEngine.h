#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "EventPlaybackSource.h"
#include "SampleRegistry.h"
#include "../model/Track.h"
#include <memory>
#include <vector>

class AudioEngine : public juce::HighResolutionTimer
{
public:
    AudioEngine();
    ~AudioEngine();

    void initialize();
    void shutdown();

    void Start();
    void Stop();
    bool IsPlaying() const;
    void SetPosition(double seconds);
    double GetPosition() const;
    double GetDuration() const;
    
    
    void hiResTimerCallback() override;

    void SetTracks(std::vector<Track>* tracks);
    
    
    void NotifyTracksChanged();
    
    void SetLooping(bool looping);
    void SetLoopPoints(double start, double end);
    
    SampleRegistry& GetSampleRegistry() { return sampleRegistry; }

    
    void LoadMasterTrack(const std::string& path);
    std::vector<float> GetWaveform(int numSamples);
    
    
    void SetMasterVolume(float volume);  
    void SetEffectsVolume(float volume); 
    
private:
    juce::AudioDeviceManager deviceManager;
    juce::AudioSourcePlayer audioSourcePlayer;
    juce::MixerAudioSource mixer;
    juce::AudioFormatManager formatManager;
    
    juce::AudioTransportSource masterTransport;
    std::unique_ptr<juce::AudioFormatReaderSource> masterReaderSource;
    
    SampleRegistry sampleRegistry;
    EventPlaybackSource eventPlaybackSource;
    
    bool looping = false;
    double loopStart = 0.0;
    double loopEnd = 0.0;
    
    
    
    
    double masterOffset = -0.029;
};
