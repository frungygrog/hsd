#pragma once
#include <juce_core/juce_core.h>
#include "../model/Project.h"

class ProjectSaver
{
public:
    /**
     * @brief Saves the project to the specified .osu file.
     * 
     * This function will:
     * 1. Set the Creator metadata to "hsd".
     * 2. Serialize all tracks into a single [HitObjects] section using the x256,y192 format.
     * 3. Preserve [General], [Editor], [Metadata], and [TimingPoints] from the internal Project state.
     * 
     * @param project The project data to save.
     * @param file The destination file.
     * @return true if successful, false otherwise.
     */
    static bool SaveProject(const Project& project, const juce::File& file);

private:
    static juce::String GenerateHitObjectsSection(const Project& project);
    
    
    struct HitObjectState {
        int timeMs;
        int hitSoundBitmask = 0; 
        int normalSet = 0;   
        int additionSet = 0; 
        int volume = 0;
        std::string filename;
        
    };
};
