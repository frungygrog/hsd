#include "OsuParser.h"
#include <map>
#include <algorithm>

// Helper to generate a unique key for grouping events into tracks
static std::string getTrackKey(SampleSet set, SampleType type, const std::string& filename)
{
    if (!filename.empty())
        return filename;
    
    std::string setStr;
    switch (set) {
        case SampleSet::Normal: setStr = "normal"; break;
        case SampleSet::Soft: setStr = "soft"; break;
        case SampleSet::Drum: setStr = "drum"; break;
    }
    
    std::string typeStr;
    switch (type) {
        case SampleType::HitNormal: typeStr = "hitnormal"; break;
        case SampleType::HitWhistle: typeStr = "hitwhistle"; break;
        case SampleType::HitFinish: typeStr = "hitfinish"; break;
        case SampleType::HitClap: typeStr = "hitclap"; break;
        default: typeStr = "other"; break;
    }
    
    return setStr + "-" + typeStr;
}

Project OsuParser::parse(const juce::File& file)
{
    Project project;
    if (!file.existsAsFile())
        return project;
        
    project.projectDirectory = file.getParentDirectory().getFullPathName().toStdString();
        
    juce::StringArray lines;
    file.readLines(lines);
    
    using TimingPoint = Project::TimingPoint;
    std::vector<TimingPoint> timingPoints;
    
    juce::String currentSection;
    
    // Parse Timing Points and General
    for (const auto& line : lines)
    {
        auto t = line.trim();
        if (t.startsWith("[")) { currentSection = t.removeCharacters("[]"); continue; }
        if (t.isEmpty() || t.startsWith("//")) continue;
        
        if (currentSection == "General")
        {
            if (t.startsWith("AudioFilename:"))
            {
                project.audioFilename = t.substring(14).trim().toStdString();
            }
            else if (t.startsWith("SampleSet:"))
            {
                auto val = t.substring(10).trim().toLowerCase();
                if (val == "soft") project.defaultSampleSet = SampleSet::Soft;
                else if (val == "drum") project.defaultSampleSet = SampleSet::Drum;
                else project.defaultSampleSet = SampleSet::Normal;
            }
        }
        else if (currentSection == "Metadata")
        {
            if (t.startsWith("Title:"))
                project.title = t.substring(6).trim().toStdString();
            else if (t.startsWith("Artist:"))
                project.artist = t.substring(7).trim().toStdString();
            else if (t.startsWith("Creator:"))
                project.creator = t.substring(8).trim().toStdString();
            else if (t.startsWith("Version:"))
                project.version = t.substring(8).trim().toStdString();
        }
        else if (currentSection == "TimingPoints")
        {
            auto parts = juce::StringArray::fromTokens(t, ",", "");
            if (parts.size() >= 2)
            {
                TimingPoint tp;
                tp.time = parts[0].getDoubleValue(); 
                tp.beatLength = parts[1].getDoubleValue();
                tp.uninherited = (tp.beatLength > 0);
                tp.sampleSet = (parts.size() >= 4) ? parts[3].getIntValue() : 1;
                tp.volume = (parts.size() >= 6) ? parts[5].getIntValue() : 100.0;
                timingPoints.push_back(tp);
                
                if (tp.uninherited && tp.beatLength > 0)
                {
                    project.bpm = 60000.0 / tp.beatLength;
                    project.offset = tp.time;
                }
            }
        }
    }
    
    std::sort(timingPoints.begin(), timingPoints.end(), [](const auto& a, const auto& b){
        return a.time < b.time;
    });
    
    project.timingPoints = timingPoints;

    auto getStateAt = [&](double time) -> std::pair<int, double> {
         int sSet = 1; 
         double vol = 100.0;
         for (auto it = timingPoints.rbegin(); it != timingPoints.rend(); ++it) {
             if (it->time <= time) {
                 sSet = it->sampleSet;
                 vol = it->volume;
                 return {sSet, vol};
             }
         }
         return {sSet, vol};
    };

    // Intermediate storage: ParentKey -> Volume -> List of Events
    // ParentKey identifies the Sound (Set-Type-Filename)
    // Volume identifies the Child Track
    struct TrackData {
        SampleSet set;
        SampleType type;
        std::string filename;
        std::map<int, std::vector<Event>> eventsByVolume;
    };
    
    std::map<std::string, TrackData> hierarchy;

    currentSection = "";
    for (const auto& line : lines)
    {
        auto t = line.trim();
        if (t.startsWith("[")) { currentSection = t.removeCharacters("[]"); continue; }
        if (t.isEmpty() || t.startsWith("//")) continue;
        
        if (currentSection == "HitObjects")
        {
            auto parts = juce::StringArray::fromTokens(t, ",", "");
            if (parts.size() >= 5)
            {
                double time = parts[2].getDoubleValue(); 
                int type = parts[3].getIntValue();
                int hitSound = parts[4].getIntValue();
                
                int normalSet = 0; 
                int additionSet = 0; 
                int volume = 0;
                std::string filename = "";
                
                if (parts.size() > 5)
                {
                   auto indentParts = juce::StringArray::fromTokens(parts[parts.size()-1], ":", "");
                   if (indentParts.size() >= 1) normalSet = indentParts[0].getIntValue();
                   if (indentParts.size() >= 2) additionSet = indentParts[1].getIntValue();
                   if (indentParts.size() >= 4) volume = indentParts[3].getIntValue();
                   if (indentParts.size() >= 5) filename = indentParts[4].toStdString();
                }
                
                auto [inheritedSet, inheritedVol] = getStateAt(time);
                if (volume == 0) volume = (int)inheritedVol;
                
                // Resolution Logic - uses defaultSampleSet as ultimate fallback
                auto resolve = [&](int val) {
                    if (val == 0) { 
                        // Use inherited timing point set, or fall back to project default
                        if (inheritedSet == 2) return SampleSet::Soft;
                        if (inheritedSet == 3) return SampleSet::Drum;
                        if (inheritedSet == 1) return SampleSet::Normal;
                        // inheritedSet == 0 means "auto" - use the project default
                        return project.defaultSampleSet;
                    }
                    if (val == 2) return SampleSet::Soft;
                    if (val == 3) return SampleSet::Drum;
                    return SampleSet::Normal;
                };
                
                SampleSet finalBaseSet = resolve(normalSet);
                SampleSet finalAddSet = (additionSet == 0) ? finalBaseSet : resolve(additionSet);

                // Add Event Helper
                auto addEvent = [&](SampleSet s, SampleType ty, const std::string& fn, double vol) {
                    std::string key = getTrackKey(s, ty, fn); // Using helper from top of file
                    
                    if (hierarchy.find(key) == hierarchy.end()) {
                        TrackData td;
                        td.set = s;
                        td.type = ty;
                        td.filename = fn;
                        hierarchy[key] = td;
                    }
                    
                    Event e;
                    e.time = time / 1000.0;
                    e.volume = vol / 100.0; // scale 0-1
                    
                    int volInt = (int)vol;
                    hierarchy[key].eventsByVolume[volInt].push_back(e);
                };
                
                bool isSpinner = (type & 8);
                if (!isSpinner)
                {
                    addEvent(finalBaseSet, SampleType::HitNormal, filename, volume);
                    if (hitSound & 2) addEvent(finalAddSet, SampleType::HitWhistle, filename, volume);
                    if (hitSound & 4) addEvent(finalAddSet, SampleType::HitFinish, filename, volume);
                    if (hitSound & 8) addEvent(finalAddSet, SampleType::HitClap, filename, volume);
                }
            }
        }
    }
    
    // Build Project Tracks
    for (auto& [key, data] : hierarchy)
    {
        Track parent;
        parent.name = key;
        parent.sampleSet = data.set;
        parent.sampleType = data.type;
        parent.customFilename = data.filename;
        parent.isExpanded = false; // Default collapsed
        
        // Create Children for each volume
        for (auto& [vol, events] : data.eventsByVolume)
        {
            Track child;
            child.name = parent.name + " (" + std::to_string(vol) + "%)";
            child.sampleSet = data.set;
            child.sampleType = data.type;
            child.customFilename = data.filename;
            child.events = events;
            child.gain = (float)vol / 100.0f; // Track gain applied? Or event volume? 
            // Model: Event has volume (0-1). Track likely has gain.
            // If we use Child Track for volume, we can set Track Gain = 1, and Event Volume = 1?
            // Original logic: Event volume is stored.
            // If we group by volume, we can optimization: set Track Gain = vol/100, Event Volume = 1.
            // But let's keep Event Volume as is from parsing to be safe.
            
            parent.children.push_back(child);
        }
        
        // Sort children?
        
        project.tracks.push_back(parent);
    }
    
    // Sort Tracks?
    
    return project;
}

bool OsuParser::CreateHitsoundDiff(const juce::File& referenceFile, const juce::File& targetFile)
{
    if (!referenceFile.existsAsFile())
        return false;

    juce::StringArray lines;
    referenceFile.readLines(lines);

    juce::String audioFilename;
    juce::String title, artist, creator;
    std::vector<juce::String> timingPoints;
    juce::String currentSection;
    bool inTimingPoints = false;
    
    // Parse Reference
    for (const auto& line : lines)
    {
        auto t = line.trim();
        if (t.startsWith("[")) 
        { 
            currentSection = t.removeCharacters("[]"); 
            inTimingPoints = (currentSection == "TimingPoints");
            continue; 
        }
        if (t.isEmpty() || t.startsWith("//")) continue;

        if (currentSection == "General")
        {
            if (t.startsWith("AudioFilename:"))
                audioFilename = t.substring(14).trim();
        }
        else if (currentSection == "Metadata")
        {
            if (t.startsWith("Title:"))
                title = t.substring(6).trim();
            else if (t.startsWith("Artist:"))
                artist = t.substring(7).trim();
            else if (t.startsWith("Creator:"))
                creator = t.substring(8).trim();
        }
        else if (inTimingPoints)
        {
            timingPoints.push_back(line); // Keep original line to preserve precision
        }
    }

    // Build New Content
    juce::String newLine = "\r\n";
    juce::String content;
    
    content += "osu file format v14" + newLine;
    content += newLine;
    content += "[General]" + newLine;
    content += "AudioFilename: " + audioFilename + newLine;
    content += "AudioLeadIn: 0" + newLine;
    content += "PreviewTime: -1" + newLine;
    content += "Countdown: 0" + newLine;
    content += "SampleSet: Normal" + newLine;
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
    content += "Title:" + title + newLine;
    content += "TitleUnicode:" + title + newLine;
    content += "Artist:" + artist + newLine;
    content += "ArtistUnicode:" + artist + newLine;
    content += "Creator:" + creator + newLine;
    content += "Version:Hitsounds" + newLine;
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
    content += "//Background and Video events" + newLine;
    content += "//Break Periods" + newLine;
    content += "//Storyboard Layer 0 (Background)" + newLine;
    content += "//Storyboard Layer 1 (Fail)" + newLine;
    content += "//Storyboard Layer 2 (Pass)" + newLine;
    content += "//Storyboard Layer 3 (Foreground)" + newLine;
    content += "//Storyboard Layer 4 (Overlay)" + newLine;
    content += "//Storyboard Sound Samples" + newLine;
    content += newLine;
    
    content += "[TimingPoints]" + newLine;
    for (const auto& tp : timingPoints)
        content += tp + newLine;
    content += newLine;
    
    content += "[HitObjects]" + newLine;
    
    return targetFile.replaceWithText(content);
}
