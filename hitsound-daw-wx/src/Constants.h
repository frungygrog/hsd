#pragma once


namespace TrackLayout {
    
    constexpr int ParentTrackHeight = 80;
    constexpr int ChildTrackHeight = 40;
    constexpr int MasterTrackHeight = 100;
    constexpr int RulerHeight = 30;
    constexpr int HeaderHeight = RulerHeight + MasterTrackHeight; 

    
    inline int GetTrackHeight(bool isChildTrack) {
        return isChildTrack ? ChildTrackHeight : ParentTrackHeight;
    }
}


namespace TimerIntervals {
    constexpr int PlaybackUpdate = 30;   
    constexpr int LoopCheck = 10;        
}


namespace DefaultVolumes {
    constexpr float EffectsGain = 0.6f;   
    constexpr float MasterGain = 1.0f;    
    constexpr float NewTrackGain = 0.6f;  
    constexpr float NewChildGain = 0.5f;  
}


namespace ZoomSettings {
    constexpr double DefaultPixelsPerSecond = 100.0;
    constexpr double MinPixelsPerSecond = 10.0;
    constexpr double MaxPixelsPerSecond = 5000.0;
}
