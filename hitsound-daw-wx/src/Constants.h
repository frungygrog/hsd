#pragma once

// Track Layout Constants
namespace TrackLayout {
    // Track Heights (in pixels)
    constexpr int ParentTrackHeight = 80;
    constexpr int ChildTrackHeight = 40;
    constexpr int MasterTrackHeight = 100;
    constexpr int RulerHeight = 30;
    constexpr int HeaderHeight = RulerHeight + MasterTrackHeight; // 130

    // Helper function - inline to avoid ODR issues
    inline int GetTrackHeight(bool isChildTrack) {
        return isChildTrack ? ChildTrackHeight : ParentTrackHeight;
    }
}

// Timer Intervals (in milliseconds)
namespace TimerIntervals {
    constexpr int PlaybackUpdate = 30;   // ~30fps for playback position updates
    constexpr int LoopCheck = 10;        // High-frequency loop boundary checking
}

// Default Volumes
namespace DefaultVolumes {
    constexpr float EffectsGain = 0.6f;   // 60% default for effects/hitsounds
    constexpr float MasterGain = 1.0f;    // 100% default for master
    constexpr float NewTrackGain = 0.6f;  // 60% default for new preset tracks
    constexpr float NewChildGain = 0.5f;  // 50% default for added children
}

// Zoom Settings
namespace ZoomSettings {
    constexpr double DefaultPixelsPerSecond = 100.0;
    constexpr double MinPixelsPerSecond = 10.0;
    constexpr double MaxPixelsPerSecond = 5000.0;
}
