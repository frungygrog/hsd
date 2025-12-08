#pragma once
#include "Track.h"
#include <map>
#include <vector>
#include <cmath>

class ProjectValidator
{
public:
    static void Validate(Project& project)
    {
        // Gather all events by time
        // Key: Time (int ms for loose comparison? double seconds?)
        // Use a small epsilon or quantized time.
        // Osu uses integer ms.
        
        struct TimeSlice {
            std::vector<Event*> events;
            std::vector<Track*> tracks;
        };
        
        // Use a sorted map or just iterating?
        // Iterating all tracks and putting events into a map is O(N_events * log(N_distinct_times)).
        
        std::map<long long, TimeSlice> slices;
        
        for (auto& track : project.tracks)
        {
            for (auto& event : track.events)
            {
                long long timeMs = (long long)(event.time * 1000.0 + 0.5);
                slices[timeMs].events.push_back(&event);
                slices[timeMs].tracks.push_back(&track);
            }
        }
        
        // Validate each slice
        for (auto& [time, slice] : slices)
        {
            // Gather used addition banks
            // Rule: All additions (Whistle, Finish, Clap) must share the same bank.
            // Base (HitNormal) bank doesn't matter? 
            // User says: "soft-hitnormal with normal-hitwhistle and normal-hitclap is ok." -> Base Soft, Additions Normal.
            // "soft-hitnormal with normal-hitwhistle and drum-hitclap is not ok." -> Additions Normal & Drum.
            
            SampleSet additionBank = SampleSet::Normal; 
            bool additionBankSet = false;
            bool conflict = false;
            
            for (size_t i = 0; i < slice.events.size(); ++i)
            {
                Track* t = slice.tracks[i];
                Event* e = slice.events[i];
                
                // Identify if this track is an "Addition" or "Base"?
                // t->sampleType
                bool isAddition = (t->sampleType == SampleType::HitWhistle ||
                                   t->sampleType == SampleType::HitFinish ||
                                   t->sampleType == SampleType::HitClap);
                                   
                if (isAddition)
                {
                    if (!additionBankSet)
                    {
                        additionBank = t->sampleSet;
                        additionBankSet = true;
                    }
                    else
                    {
                        if (t->sampleSet != additionBank)
                        {
                            conflict = true;
                        }
                    }
                }
            }
            
            if (conflict)
            {
                // Mark ONLY the additions as invalid? or all events at this time?
                // User said "the notes should highlight red".
                // I'll mark all events at this tick as invalid (or just additions?).
                // Usually helpful to mark all to show the group is bad.
                for (auto* e : slice.events)
                {
                    e->validationState = ValidationState::Invalid;
                }
            }
            else
            {
                for (auto* e : slice.events)
                {
                    e->validationState = ValidationState::Valid;
                }
            }
        }
    }
};
