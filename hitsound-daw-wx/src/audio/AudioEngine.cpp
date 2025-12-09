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
    
    deviceManager.initialiseWithDefaultDevices(0, 2); 
    
    
    audioSourcePlayer.setSource(&mixer);
    deviceManager.addAudioCallback(&audioSourcePlayer);
    
    
    mixer.addInputSource(&masterTransport, false);
    mixer.addInputSource(&eventPlaybackSource, false);
    
    eventPlaybackSource.setTransportSource(&masterTransport);
    eventPlaybackSource.setOffset(masterOffset);
    
    startTimer(10); 
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

void AudioEngine::NotifyTracksChanged()
{
    eventPlaybackSource.updateTracksSnapshot();
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

void AudioEngine::LoadMasterTrack(const std::string& path)
{
    masterTransport.stop();
    masterTransport.setSource(nullptr);
    masterReaderSource.reset();
    
    juce::File file(path);
    if (!file.existsAsFile())
    {
        DBG("AudioEngine: Master track file not found: " + juce::String(path));
        return;
    }
    
    auto* reader = formatManager.createReaderFor(file);
    if (reader)
    {
        masterReaderSource.reset(new juce::AudioFormatReaderSource(reader, true));
        masterTransport.setSource(masterReaderSource.get(), 0, nullptr, reader->sampleRate);
        DBG("AudioEngine: Loaded master track: " + file.getFileName());
    }
    else
    {
        DBG("AudioEngine: Failed to create reader for: " + file.getFileName());
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
    
    
    
    long long chunkSize = length / numSamples;
    if (chunkSize < 1) chunkSize = 1;
    
    
    juce::AudioBuffer<float> buffer(channels, (int)chunkSize);
    
    long long position = 0;
    for (int i = 0; i < numSamples; ++i)
    {
        reader->read(&buffer, 0, (int)chunkSize, position, true, true);
        
        float maxVal = 0.0f;
        
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
    
    if (looping && masterTransport.isPlaying() && loopStart >= 0 && loopEnd > loopStart)
    {
        double currentPos = masterTransport.getCurrentPosition();
        if (currentPos >= loopEnd - masterOffset)
        {
            masterTransport.setPosition(loopStart - masterOffset);
        }
    }
}

void AudioEngine::SetMasterVolume(float volume)
{
    masterTransport.setGain(volume);
}

void AudioEngine::SetEffectsVolume(float volume)
{
    eventPlaybackSource.setMasterGain(volume);
}
