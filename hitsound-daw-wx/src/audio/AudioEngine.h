#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "EventPlaybackSource.h"
#include "SampleRegistry.h"
#include "../model/Track.h"
#include <memory>
#include <vector>

class AudioEngine : public juce::AudioIODeviceCallback, public juce::HighResolutionTimer
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
    
    // AudioIODeviceCallback implementation
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;
    
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    // HighResolutionTimer
    void hiResTimerCallback() override;

    void SetTracks(std::vector<Track>* tracks);
    void SetLooping(bool looping);
    void SetLoopPoints(double start, double end);
    
    SampleRegistry& GetSampleRegistry() { return sampleRegistry; }

    // Master Track
    void LoadMasterTrack(const std::string& path);
    std::vector<float> GetWaveform(int numSamples);
    
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
    
    double masterOffset = -0.029; // Fixed offset for now as requested
};
