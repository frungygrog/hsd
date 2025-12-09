#include "ProjectValidator.h"
#include <map>
#include <cmath>
#include <functional>

std::vector<ProjectValidator::ValidationError> ProjectValidator::Validate(Project& project)
{
    std::vector<ValidationError> errors;

    // Group events by timestamp (rounded to ms) for conflict detection
    struct TimeSlice {
        std::vector<Event*> events;
        std::vector<Track*> tracks;
    };
    std::map<long long, TimeSlice> slices;

    // Recursively collect all events including from child tracks
    std::function<void(Track&)> collectEvents = [&](Track& track) {
        for (auto& event : track.events)
        {
            long long timeMs = (long long)(event.time * 1000.0 + 0.5);
            slices[timeMs].events.push_back(&event);
            slices[timeMs].tracks.push_back(&track);
        }
        for (auto& child : track.children)
        {
            collectEvents(child);
        }
    };

    for (auto& track : project.tracks)
    {
        collectEvents(track);
    }

    // Check each time slice for bank conflicts
    for (auto& [time, slice] : slices)
    {
        SampleSet additionBank = SampleSet::Normal;
        bool additionBankSet = false;

        SampleSet normalBank = SampleSet::Normal;
        bool normalBankSet = false;

        bool conflict = false;
        std::string conflictingBankName = "";

        for (size_t i = 0; i < slice.events.size(); ++i)
        {
            Track* t = slice.tracks[i];

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
                else if (t->sampleSet != additionBank)
                {
                    conflict = true;
                    conflictingBankName = (t->sampleSet == SampleSet::Normal) ? "Normal" : (t->sampleSet == SampleSet::Soft ? "Soft" : "Drum");
                }
            }

            if (t->sampleType == SampleType::HitNormal)
            {
                if (!normalBankSet)
                {
                    normalBank = t->sampleSet;
                    normalBankSet = true;
                }
                else if (t->sampleSet != normalBank)
                {
                    conflict = true;
                    conflictingBankName = (t->sampleSet == SampleSet::Normal) ? "Normal" : (t->sampleSet == SampleSet::Soft ? "Soft" : "Drum");
                }
            }
        }

        if (conflict)
        {
            std::string bankName = (additionBank == SampleSet::Normal) ? "Normal" : (additionBank == SampleSet::Soft ? "Soft" : "Drum");
            errors.push_back({ (double)time / 1000.0, "Conflicting addition banks: " + bankName + " vs " + conflictingBankName });

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

    return errors;
}
