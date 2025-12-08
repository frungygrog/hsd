#include "AudioEngine.h"

AudioEngine::AudioEngine()
    : eventPlaybackSource(sampleRegistry)
{
    formatManager.registerBasicFormats();
}

AudioEngine::~AudioEngine()
{
    shutdown();
}

void AudioEngine::initialize()
{
    // Initialize Device Manager
    deviceManager.initialiseWithDefaultDevices(0, 2); 
    
    // Connect mixer to player
    audioSourcePlayer.setSource(&mixer);
    deviceManager.addAudioCallback(&audioSourcePlayer);
    
    // Connect sources to mixer
    mixer.addInputSource(&masterTransport, false);
    mixer.addInputSource(&eventPlaybackSource, false);
    
    eventPlaybackSource.setTransportSource(&masterTransport);
    eventPlaybackSource.setOffset(masterOffset);
    
    startTimer(10); // 10ms timer for looping
}

void AudioEngine::shutdown()
{
    stopTimer();
    deviceManager.removeAudioCallback(&audioSourcePlayer);
    audioSourcePlayer.setSource(nullptr);
    mixer.removeAllInputs();
}

void AudioEngine::Start()
{
    masterTransport.start();
}

void AudioEngine::Stop()
{
    masterTransport.stop();
}

bool AudioEngine::IsPlaying() const
{
    return masterTransport.isPlaying();
}

void AudioEngine::SetPosition(double seconds)
{
    masterTransport.setPosition(seconds - masterOffset);
}

double AudioEngine::GetPosition() const
{
    // Return Project Time
    return masterTransport.getCurrentPosition() + masterOffset;
}

double AudioEngine::GetDuration() const
{
    return masterTransport.getLengthInSeconds();
}

void AudioEngine::SetTracks(std::vector<Track>* tracks)
{
    eventPlaybackSource.setTracks(tracks);
}

void AudioEngine::SetLooping(bool shouldLoop)
{
    looping = shouldLoop;
}

void AudioEngine::SetLoopPoints(double start, double end)
{
    loopStart = start;
    loopEnd = end;
}

void AudioEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                      int numInputChannels,
                                      float* const* outputChannelData,
                                      int numOutputChannels,
                                      int numSamples,
                                      const juce::AudioIODeviceCallbackContext& context)
{
    // This is handled by audioSourcePlayer automatically usually.
    // Since we registered audioSourcePlayer as callback, this might be redundant if we inherit AudioIODeviceCallback manually?
    // Wait, AudioSourcePlayer IS an AudioIODeviceCallback.
    // We don't need to manually implement this if we use AudioSourcePlayer.
    // However, MainFrame might want to access this?
    // Actually, we registered audioSourcePlayer in initialize(), so THAT handles the callback.
    // The AudioEngine inherits AudioIODeviceCallback but we didn't register 'this'.
    // So these methods are unused unless we register 'this'.
    // We should probably NOT inherit AudioIODeviceCallback if we delegate to AudioSourcePlayer.
    // But for ABI compatibility / future custom processing, we can leave it.
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device) {}
void AudioEngine::audioDeviceStopped() {}

void AudioEngine::LoadMasterTrack(const std::string& path)
{
    masterTransport.stop();
    masterTransport.setSource(nullptr);
    masterReaderSource.reset();
    
    juce::File file(path);
    if (!file.existsAsFile()) return;
    
    auto* reader = formatManager.createReaderFor(file);
    if (reader)
    {
        masterReaderSource.reset(new juce::AudioFormatReaderSource(reader, true));
        masterTransport.setSource(masterReaderSource.get(), 0, nullptr, reader->sampleRate);
    }
}

std::vector<float> AudioEngine::GetWaveform(int numSamples)
{
    std::vector<float> peaks;
    if (!masterReaderSource) return peaks;
    
    auto* reader = masterReaderSource->getAudioFormatReader();
    if (!reader) return peaks;
    
    long long length = reader->lengthInSamples;
    int channels = reader->numChannels;
    
    if (length <= 0 || channels <= 0) return peaks;
    
    // We want 'numSamples' peaks.
    // Chunk size
    long long chunkSize = length / numSamples;
    if (chunkSize < 1) chunkSize = 1;
    
    // Buffer for reading
    juce::AudioBuffer<float> buffer(channels, (int)chunkSize);
    
    long long position = 0;
    for (int i = 0; i < numSamples; ++i)
    {
        reader->read(&buffer, 0, (int)chunkSize, position, true, true);
        
        float maxVal = 0.0f;
        // Check all channels
        for (int c = 0; c < channels; ++c)
        {
            auto range = buffer.findMinMax(c, 0, (int)chunkSize);
            float currMax = std::max(std::abs(range.getStart()), std::abs(range.getEnd()));
            if (currMax > maxVal) maxVal = currMax;
        }
        peaks.push_back(maxVal);
        
        position += chunkSize;
    }
    
    return peaks;
}

void AudioEngine::hiResTimerCallback()
{
    // Only loop if we have a valid loop region
    if (looping && masterTransport.isPlaying() && loopStart >= 0 && loopEnd > loopStart)
    {
        double currentPos = masterTransport.getCurrentPosition();
        if (currentPos >= loopEnd - masterOffset)
        {
            masterTransport.setPosition(loopStart - masterOffset);
        }
    }
}
