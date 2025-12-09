#include "ProjectSaver.h"
#include <map>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <sstream>

bool ProjectSaver::SaveProject(const Project& project, const juce::File& file)
{
    
    juce::String newLine = "\r\n";
    juce::String content;

    
    content += "osu file format v14" + newLine;
    content += newLine;

    
    content += "[General]" + newLine;
    content += "AudioFilename: " + juce::String(project.audioFilename) + newLine;
    content += "AudioLeadIn: 0" + newLine;
    content += "PreviewTime: -1" + newLine;
    content += "Countdown: 0" + newLine;
    
    
    juce::String defSampleSet = "Normal";
    if (project.defaultSampleSet == SampleSet::Soft) defSampleSet = "Soft";
    else if (project.defaultSampleSet == SampleSet::Drum) defSampleSet = "Drum";
    content += "SampleSet: " + defSampleSet + newLine;
    
    content += "StackLeniency: 0.7" + newLine;
    content += "Mode: 0" + newLine;
    content += "LetterboxInBreaks: 0" + newLine;
    content += "WidescreenStoryboard: 0" + newLine;
    content += newLine;

    
    content += "[Editor]" + newLine;
    content += "DistanceSpacing: 1.0" + newLine;
    content += "BeatDivisor: 4" + newLine;
    content += "GridSize: 32" + newLine;
    content += "TimelineZoom: 1" + newLine;
    content += newLine;

    
    content += "[Metadata]" + newLine;
    content += "Title:" + juce::String(project.title) + newLine;
    content += "TitleUnicode:" + juce::String(project.title) + newLine; 
    content += "Artist:" + juce::String(project.artist) + newLine;
    content += "ArtistUnicode:" + juce::String(project.artist) + newLine; 
    content += "Creator:hsd" + newLine; 
    content += "Version:" + juce::String(project.version) + newLine;
    content += "Source:" + newLine;
    content += "Tags:" + newLine;
    content += "BeatmapID:0" + newLine;
    content += "BeatmapSetID:-1" + newLine;
    content += newLine;

    
    content += "[Difficulty]" + newLine;
    content += "HPDrainRate:5" + newLine; 
    content += "CircleSize:5" + newLine;
    content += "OverallDifficulty:5" + newLine;
    content += "ApproachRate:5" + newLine;
    content += "SliderMultiplier:1.4" + newLine;
    content += "SliderTickRate:1" + newLine;
    content += newLine;

    
    content += "[Events]" + newLine;
    content += "
    content += "
    content += "
    content += "
    content += "
    content += "
    content += "
    content += "
    content += newLine;

    
    content += "[TimingPoints]" + newLine;
    for (const auto& tp : project.timingPoints)
    {
        
        
        
        
        
        
        
        
        
        
        if (!tp.uninherited) continue;

        std::stringstream ss;
        ss << std::fixed << std::setprecision(10) << tp.time << ","; 
        
        
        
        
        
        content += juce::String(tp.time) + ",";
        content += juce::String(tp.beatLength) + ",";
        content += "4,"; 
        content += juce::String(tp.sampleSet) + ",";
        content += "0,"; 
        content += juce::String((int)tp.volume) + ",";
        content += (tp.uninherited ? "1" : "0") + juce::String(",");
        content += "0"; 
        content += newLine;
    }
    content += newLine;

    
    content += GenerateHitObjectsSection(project);

    
    return file.replaceWithText(content);
}

juce::String ProjectSaver::GenerateHitObjectsSection(const Project& project)
{
    juce::String content;
    content += juce::String("[HitObjects]") + "\r\n";

    
    struct MergedEvent {
        int bitmask = 0; 
        int normalSet = 0; 
        int additionSet = 0; 
        int volume = 0; 
        std::string filename; 
    };

    std::map<int, MergedEvent> timeMap;

    
    std::function<void(const Track&)> processTrack = [&](const Track& track) {
        
        
        
        for (const auto& ev : track.events)
        {
            int timeMs = (int)(ev.time * 1000.0 + 0.5);
            
            
            
            int setVal = 1; 
            if (track.sampleSet == SampleSet::Soft) setVal = 2;
            else if (track.sampleSet == SampleSet::Drum) setVal = 3;
            
            
            
            
            
            int mask = 0;
            switch (track.sampleType) {
                case SampleType::HitNormal:  mask = 1; break; 
                case SampleType::HitWhistle: mask = 2; break;
                case SampleType::HitFinish:  mask = 4; break;
                case SampleType::HitClap:    mask = 8; break;
            }
            
            
            auto& merged = timeMap[timeMs];
            
            
            
            
            
            if (track.sampleType == SampleType::HitNormal) {
                merged.normalSet = setVal;
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
            } else {
                merged.additionSet = setVal;
            }
            
            merged.bitmask |= mask;
            
            
            int evVol = (int)(ev.volume * 100.0);
            if (evVol > merged.volume) merged.volume = evVol;
            
            
            
            if (!track.customFilename.empty()) {
                merged.filename = track.customFilename;
            }
        }
        
        for (const auto& child : track.children) {
            processTrack(child);
        }
    };

    for (const auto& t : project.tracks) {
        processTrack(t);
    }
    
    
    for (const auto& [timeMs, data] : timeMap)
    {
        
        
        
        
        
        
        int finalBitmask = data.bitmask;
        
        
        
        
        
        
        
        
        
        
        if (finalBitmask & 1) finalBitmask &= ~1; 
        
        content += "256,192," + juce::String(timeMs) + ",1," + juce::String(finalBitmask) + ",";
        
        
        
        
        
        
        
        
        int sSet = data.normalSet; 
        int aSet = data.additionSet;
        if (sSet == 0) sSet = 0; 
        if (aSet == 0) aSet = 0; 
        
        content += juce::String(sSet) + ":" + 
                   juce::String(aSet) + ":0:" + 
                   juce::String(data.volume) + ":" + 
                   data.filename + "\r\n";
    }

    return content;
}
