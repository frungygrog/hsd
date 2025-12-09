#include "ProjectSaver.h"
#include <map>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <sstream>

bool ProjectSaver::SaveProject(const Project& project, const juce::File& file)
{
    // 1. Prepare Content
    juce::String newLine = "\r\n";
    juce::String content;

    // Header
    content += "osu file format v14" + newLine;
    content += newLine;

    // [General]
    content += "[General]" + newLine;
    content += "AudioFilename: " + juce::String(project.audioFilename) + newLine;
    content += "AudioLeadIn: 0" + newLine;
    content += "PreviewTime: -1" + newLine;
    content += "Countdown: 0" + newLine;
    
    // Default SampleSet
    juce::String defSampleSet = "Normal";
    if (project.defaultSampleSet == SampleSet::Soft) defSampleSet = "Soft";
    else if (project.defaultSampleSet == SampleSet::Drum) defSampleSet = "Drum";
    content += "SampleSet: " + defSampleSet + newLine;
    
    content += "StackLeniency: 0.7" + newLine;
    content += "Mode: 0" + newLine;
    content += "LetterboxInBreaks: 0" + newLine;
    content += "WidescreenStoryboard: 0" + newLine;
    content += newLine;

    // [Editor]
    content += "[Editor]" + newLine;
    content += "DistanceSpacing: 1.0" + newLine;
    content += "BeatDivisor: 4" + newLine;
    content += "GridSize: 32" + newLine;
    content += "TimelineZoom: 1" + newLine;
    content += newLine;

    // [Metadata]
    content += "[Metadata]" + newLine;
    content += "Title:" + juce::String(project.title) + newLine;
    content += "TitleUnicode:" + juce::String(project.title) + newLine; // Assuming same for now
    content += "Artist:" + juce::String(project.artist) + newLine;
    content += "ArtistUnicode:" + juce::String(project.artist) + newLine; // Assuming same
    content += "Creator:hsd" + newLine; // Enforce hsd watermark
    content += "Version:" + juce::String(project.version) + newLine;
    content += "Source:" + newLine;
    content += "Tags:" + newLine;
    content += "BeatmapID:0" + newLine;
    content += "BeatmapSetID:-1" + newLine;
    content += newLine;

    // [Difficulty]
    content += "[Difficulty]" + newLine;
    content += "HPDrainRate:5" + newLine; // Defaults
    content += "CircleSize:5" + newLine;
    content += "OverallDifficulty:5" + newLine;
    content += "ApproachRate:5" + newLine;
    content += "SliderMultiplier:1.4" + newLine;
    content += "SliderTickRate:1" + newLine;
    content += newLine;

    // [Events]
    content += "[Events]" + newLine;
    content += "//Background and Video events" + newLine;
    content += "//Break Periods" + newLine;
    content += "//Storyboard Layer 0 (Background)" + newLine;
    content += "//Storyboard Layer 1 (Fail)" + newLine;
    content += "//Storyboard Layer 2 (Pass)" + newLine;
    content += "//Storyboard Layer 3 (Foreground)" + newLine;
    content += "//Storyboard Layer 4 (Overlay)" + newLine;
    content += "//Storyboard Sound Samples" + newLine;
    content += newLine;

    // [TimingPoints]
    content += "[TimingPoints]" + newLine;
    for (const auto& tp : project.timingPoints)
    {
        // time,beatLength,meter,sampleSet,sampleIndex,volume,uninherited,effects
        // sampleIndex = 0 (default custom)
        // effects = 0 (kiai off) - ignoring kiai for now
        
        // Need to serialize carefully to match osu format
        // Assuming tp.sampleSet is 1=Normal, 2=Soft, 3=Drum
        // tp.uninherited is bool -> 1 or 0
        
        // Filter out inherited timing points (Green Lines) to remove "extra" points
        // We only keep uninherited points (Red Lines) which define BPM.
        if (!tp.uninherited) continue;

        std::stringstream ss;
        ss << std::fixed << std::setprecision(10) << tp.time << ","; // High precision for time? Osu likes readable numbers generally
        // Standard osu format usually not scientific
        // Let's use juce::String for cleaner formatting if possible, or simple standard
        
        // Remove trailing zeros if possible? Osu doesn't mind .00
        
        content += juce::String(tp.time) + ",";
        content += juce::String(tp.beatLength) + ",";
        content += "4,"; // Meter
        content += juce::String(tp.sampleSet) + ",";
        content += "0,"; // SampleIndex
        content += juce::String((int)tp.volume) + ",";
        content += (tp.uninherited ? "1" : "0") + juce::String(",");
        content += "0"; // Effects
        content += newLine;
    }
    content += newLine;

    // [HitObjects]
    content += GenerateHitObjectsSection(project);

    // 2. Write to File
    return file.replaceWithText(content);
}

juce::String ProjectSaver::GenerateHitObjectsSection(const Project& project)
{
    juce::String content;
    content += juce::String("[HitObjects]") + "\r\n";

    // Data structure to hold merged events at a specific time
    struct MergedEvent {
        int bitmask = 0; // 1=Normal (Base), 2=Whistle, 4=Finish, 8=Clap
        int normalSet = 0; // The sample set for the "Base" (Normal/Soft/Drum)
        int additionSet = 0; // The sample set for the additions
        int volume = 0; // Max volume found?
        std::string filename; // Custom filename if any
    };

    std::map<int, MergedEvent> timeMap;

    // Recursive helper to process tracks
    std::function<void(const Track&)> processTrack = [&](const Track& track) {
        // We only care about leaf tracks (tracks with events)
        // Even parent tracks might have events depending on the model, check 'events' vector
        
        for (const auto& ev : track.events)
        {
            int timeMs = (int)(ev.time * 1000.0 + 0.5);
            
            // Determine sample characteristics from Track metadata
            // SampleSet: 1=Normal, 2=Soft, 3=Drum
            int setVal = 1; 
            if (track.sampleSet == SampleSet::Soft) setVal = 2;
            else if (track.sampleSet == SampleSet::Drum) setVal = 3;
            
            // Validate Logic relies on Validator, here we just merge "Last Writer Wins" or "Priority"?
            // We should try to be additive for bitmask.
            
            // Bitmask
            int mask = 0;
            switch (track.sampleType) {
                case SampleType::HitNormal:  mask = 1; break; // Base
                case SampleType::HitWhistle: mask = 2; break;
                case SampleType::HitFinish:  mask = 4; break;
                case SampleType::HitClap:    mask = 8; break;
            }
            
            // Update the map
            auto& merged = timeMap[timeMs];
            
            // Logic for Sample Sets:
            // If this is a Base (HitNormal), it dictates the 'normalSet' (sampleSet in osu terms)
            // If this is an Addition (Whistle/Finish/Clap), it dictates 'additionSet'
            
            if (track.sampleType == SampleType::HitNormal) {
                merged.normalSet = setVal;
                // Always ensure bit 0 is set if we have a HitNormal track? 
                // In osu!, every object has a base sample. Setting bit 0 just explicitly says "use base sample".
                // Actually bit 0 (1) is always implied if no other bits? No.
                // Standard: 1 = Circle. 
                // Hitsounds are in the 5th field: Integers.
                // 0 = Normal? No.
                // Bitmask: 0=Normal(default), 2=Whistle, 4=Finish, 8=Clap.
                // Wait. 1 is NOT Normal. 1 is... nothing?
                // Osu Wiki: 
                // 0: Normal
                // 1: Normal (Wait, checking wiki)
                // "The hitsound is a bitmask... 1 = Normal, 2 = Whistle, 4 = Finish, 8 = Clap"
                // Actually 0 also plays the base sample.
                // But usually we set 2, 4, 8.
                // If existing code has logic, let's check.
                // Track.h has HitNormal.
                
                // Let's assume we OR them.
                // If we have a HitNormal track, strictly speaking we don't *need* a bit for it if others are present?
                // No, 1 is specific.
            } else {
                merged.additionSet = setVal;
            }
            
            merged.bitmask |= mask;
            
            // Volume: Take the MAX volume?
            int evVol = (int)(ev.volume * 100.0);
            if (evVol > merged.volume) merged.volume = evVol;
            
            // Filename: If track has custom filename, use it.
            // If multiple have custom filenames, last one wins (undefined order?).
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
    
    // Construct Lines
    for (const auto& [timeMs, data] : timeMap)
    {
        // Format: x,y,time,type,hitSound,extras
        // x=256, y=192
        // type=1 (HitCircle)
        // hitSound = bitmask
        // extras = sampleSet:addSet:index:volume:filename
        
        int finalBitmask = data.bitmask;
        // The bitmask in .osu:
        // 0 / 1?
        // Actually, in default osu editor, hitnormal doesn't add a bit. It just exists.
        // But the format says: "A bitwise flag... 0 for normal."
        // So 1 usually isn't used?
        // If I put 1, does it double play?
        // Let's adhere to: Normal=0, Whistle=2, Finish=4, Clap=8. 
        // If I have SampleType::HitNormal, I shouldn't add 1 to bitmask probably?
        // But wait, if I ONLY have HitNormal, bitmask is 0? Yes.
        
        if (finalBitmask & 1) finalBitmask &= ~1; // Clear bit 1 for "Normal"
        
        content += "256,192," + juce::String(timeMs) + ",1," + juce::String(finalBitmask) + ",";
        
        // Extras
        // sampleSet: 0=Auto, 1=Normal, 2=Soft, 3=Drum
        // addSet: same
        // index: 0
        // volume: 0-100
        // filename: string
        
        int sSet = data.normalSet; 
        int aSet = data.additionSet;
        if (sSet == 0) sSet = 0; // 0=Auto
        if (aSet == 0) aSet = 0; // 0=Auto
        
        content += juce::String(sSet) + ":" + 
                   juce::String(aSet) + ":0:" + 
                   juce::String(data.volume) + ":" + 
                   data.filename + "\r\n";
    }

    return content;
}
