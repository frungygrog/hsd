#include "EventPlaybackSource.h"

EventPlaybackSource::EventPlaybackSource (SampleRegistry& registry)
    : sampleRegistry (registry), offsetSeconds(0.0)
{
}

void EventPlaybackSource::setOffset(double offset)
{
    offsetSeconds = offset;
}

EventPlaybackSource::~EventPlaybackSource()
{
}

void EventPlaybackSource::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    currentSampleRate = sampleRate;
    mixer.prepareToPlay (samplesPerBlockExpected, sampleRate);
}

void EventPlaybackSource::releaseResources()
{
    mixer.releaseResources();
    activeVoices.clear();
}

void EventPlaybackSource::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (tracks == nullptr || transportSource == nullptr)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    auto currentPos = transportSource->getCurrentPosition() + offsetSeconds;
    auto currentSample = (int64_t) (currentPos * currentSampleRate);
    auto numSamples = bufferToFill.numSamples;
    auto endSample = currentSample + numSamples;

    // 1. Trigger new events
    std::function<void(const Track&)> processTrack = [&](const Track& track)
    {
        if (track.mute)
            return;

        // Process events
        if (!track.isGrouping)
        {
            // Standard behavior: Play track's own assigned sample
            for (const auto& event : track.events)
            {
                auto eventStartSample = (int64_t) (event.time * currentSampleRate); 
                
                if (eventStartSample >= currentSample && eventStartSample < endSample)
                {
                    SampleRef ref;
                    ref.set = track.sampleSet;
                    ref.type = track.sampleType;
                    
                    auto* reader = sampleRegistry.getReader (ref);
                    if (reader != nullptr)
                    {
                        int startOffset = (int) (eventStartSample - currentSample);
                        activeVoices.emplace_back (Voice { reader, 0, (double)reader->sampleRate / currentSampleRate, (float)(track.gain * event.volume), startOffset });
                    }
                }
            }
        }
        else
        {
            // Grouping behavior: Parent events trigger ALL children
            for (const auto& event : track.events)
            {
                auto eventStartSample = (int64_t) (event.time * currentSampleRate); 
                
                if (eventStartSample >= currentSample && eventStartSample < endSample)
                {
                    // Trigger every child
                    for (const auto& child : track.children)
                    {
                        if (child.mute) continue; 
                        
                        // Helper to play a specific sample ref
                        auto playSample = [&](SampleSet bank, SampleType type) {
                            SampleRef ref;
                            ref.set = bank;
                            ref.type = type;
                            
                            auto* reader = sampleRegistry.getReader (ref);
                            if (reader != nullptr)
                            {
                                int startOffset = (int) (eventStartSample - currentSample);
                                float combinedGain = (float)(track.gain * child.gain * event.volume);
                                activeVoices.emplace_back (Voice { reader, 0, (double)reader->sampleRate / currentSampleRate, combinedGain, startOffset });
                            }
                        };

                        if (child.layers.empty())
                        {
                            // Fallback to single sample definition
                            playSample(child.sampleSet, child.sampleType);
                        }
                        else
                        {
                            // Play all layers
                            for (const auto& layer : child.layers)
                            {
                                playSample(layer.bank, layer.type);
                            }
                        }
                    }
                }
            }
        }

        // Process children
        for (const auto& child : track.children)
        {
            processTrack (child);
        }
    };

    for (const auto& track : *tracks)
    {
        // if (track.isMaster) continue; // Track.h doesn't have isMaster?
        processTrack (track);
    }
    
    // 2. Render active voices
    // CRITICAL FIX: Clear buffer before mixing! Otherwise we mix into garbage (buzzing).
    bufferToFill.clearActiveBufferRegion();
    
    for (auto it = activeVoices.begin(); it != activeVoices.end();)
    {
        auto& voice = *it;
        bool finished = false;
        
        int destOffset = 0;
        int count = numSamples;
        
        if (voice.startOffset > 0)
        {
            destOffset = voice.startOffset;
            count -= voice.startOffset;
            voice.startOffset = 0; // Consumed
        }
        
        if (count > 0)
        {
            juce::AudioBuffer<float> tempBuffer (2, count);
            
            if (voice.reader->read (&tempBuffer, 0, count, voice.position, true, true))
            {
                tempBuffer.applyGain (voice.gain);
                
                for (int ch = 0; ch < bufferToFill.buffer->getNumChannels(); ++ch)
                {
                    int sourceCh = ch % tempBuffer.getNumChannels();
                    bufferToFill.buffer->addFrom (ch, bufferToFill.startSample + destOffset, tempBuffer, sourceCh, 0, count);
                }
                
                voice.position += count;
                
                if (voice.position >= voice.reader->lengthInSamples)
                    finished = true;
            }
            else
            {
                finished = true;
            }
        }
        
        if (finished)
            it = activeVoices.erase (it);
        else
            ++it;
    }
}

void EventPlaybackSource::setTracks (std::vector<Track>* t)
{
    tracks = t;
}

void EventPlaybackSource::setTransportSource (juce::AudioTransportSource* transport)
{
    transportSource = transport;
}
