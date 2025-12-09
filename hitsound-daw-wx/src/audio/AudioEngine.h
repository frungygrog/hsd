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
    
    // HighResolutionTimer
    void hiResTimerCallback() override;

    void SetTracks(std::vector<Track>* tracks);
    
    // Call after modifying tracks to sync audio thread's snapshot
    void NotifyTracksChanged();
    
    void SetLooping(bool looping);
    void SetLoopPoints(double start, double end);
    
    SampleRegistry& GetSampleRegistry() { return sampleRegistry; }

    // Master Track
    void LoadMasterTrack(const std::string& path);
    std::vector<float> GetWaveform(int numSamples);
    
    // Volume Control
    void SetMasterVolume(float volume);  // Song volume (0.0-1.0)
    void SetEffectsVolume(float volume); // Effects volume (0.0-1.0)
    
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
    
    // Audio latency compensation offset (in seconds).
    // Negative value shifts events earlier to account for audio processing delay.
    // TODO: Consider making this configurable via settings if different systems need different values.
    double masterOffset = -0.029;
};
