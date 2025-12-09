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
}

void EventPlaybackSource::releaseResources()
{
    activeVoices.clear();
}

void EventPlaybackSource::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    const juce::SpinLock::ScopedLockType lock (tracksLock);

    if (tracksSnapshot.empty() || transportSource == nullptr)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    auto currentPos = transportSource->getCurrentPosition() + offsetSeconds;
    auto currentSample = (int64_t) (currentPos * currentSampleRate);
    auto numSamples = bufferToFill.numSamples;
    auto endSample = currentSample + numSamples;

    // Check if any track is soloed
    bool anySolo = false;
    std::function<void(const Track&)> checkSolo = [&](const Track& t) {
        if (t.solo) anySolo = true;
        for (const auto& child : t.children) checkSolo(child);
    };
    for (const auto& t : tracksSnapshot) checkSolo(t);

    // Process each track and trigger samples
    std::function<void(const Track&)> processTrack = [&](const Track& track)
    {
        if (track.mute || (anySolo && !track.solo))
            return;

        if (!track.isGrouping)
        {
            for (const auto& event : track.events)
            {
                auto eventStartSample = (int64_t) (event.time * currentSampleRate);

                if (eventStartSample >= currentSample && eventStartSample < endSample)
                {
                    auto playSample = [&](SampleSet bank, SampleType type) {
                        SampleRef ref;
                        ref.set = bank;
                        ref.type = type;

                        auto* reader = sampleRegistry.getReader (ref);
                        if (reader != nullptr)
                        {
                            int startOffset = (int) (eventStartSample - currentSample);
                            activeVoices.emplace_back (Voice { reader, 0, (double)reader->sampleRate / currentSampleRate, (float)(track.gain * event.volume), startOffset });
                        }
                    };

                    if (track.layers.empty())
                    {
                        playSample(track.sampleSet, track.sampleType);
                    }
                    else
                    {
                        for (const auto& layer : track.layers)
                        {
                            playSample(layer.bank, layer.type);
                        }
                    }
                }
            }
        }
        else
        {
            // Grouping: trigger child samples on parent's events
            for (const auto& event : track.events)
            {
                auto eventStartSample = (int64_t) (event.time * currentSampleRate);

                if (eventStartSample >= currentSample && eventStartSample < endSample)
                {
                    for (const auto& child : track.children)
                    {
                        if (child.mute) continue;

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
                            playSample(child.sampleSet, child.sampleType);
                        }
                        else
                        {
                            for (const auto& layer : child.layers)
                            {
                                playSample(layer.bank, layer.type);
                            }
                        }
                    }
                }
            }
        }

        for (const auto& child : track.children)
        {
            processTrack (child);
        }
    };

    for (const auto& track : tracksSnapshot)
    {
        processTrack (track);
    }

    // Mix active voices into output buffer
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
            voice.startOffset = 0;
        }

        if (count > 0)
        {
            juce::AudioBuffer<float> tempBuffer (2, count);

            if (voice.reader->read (&tempBuffer, 0, count, voice.position, true, true))
            {
                tempBuffer.applyGain(voice.gain * masterGain);

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
    uiTracks = t;

    if (uiTracks != nullptr)
    {
        const juce::SpinLock::ScopedLockType lock (tracksLock);
        tracksSnapshot = *uiTracks;
    }
}

void EventPlaybackSource::updateTracksSnapshot()
{
    if (uiTracks == nullptr) return;

    const juce::SpinLock::ScopedLockType lock (tracksLock);
    tracksSnapshot = *uiTracks;
}

void EventPlaybackSource::setTransportSource (juce::AudioTransportSource* transport)
{
    transportSource = transport;
}
