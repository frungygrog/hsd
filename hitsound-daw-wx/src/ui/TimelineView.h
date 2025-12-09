#pragma once
#include <wx/wx.h>
#include "../model/Project.h"
#include <set>
#include <utility>
#include <vector>
#include <map>
#include <algorithm>
#include "../model/Command.h"
#include "../Constants.h"

enum class ToolType
{
    Select = 0,
    Draw
};

class TimelineView : public wxScrolledWindow
{
public:
    TimelineView(wxWindow* parent);
    
    // Callback for loop changes
    std::function<void(double start, double end)> OnLoopPointsChanged;
    
    // Callback for zoom changes
    std::function<void(double pixelsPerSecond)> OnZoomChanged;
    
    // Callback when tracks are modified (events added/removed/moved) - for audio thread sync
    std::function<void()> OnTracksModified;
    
    // Callback when user scrubs the playhead (drag in ruler/master area)
    std::function<void(double time)> OnPlayheadScrubbed;
    
    void SetProject(Project* p);
    void SetTool(ToolType tool) { currentTool = tool; }
    void SetPlayheadPosition(double time);
    
    void SetGridDivisor(int divisor) { gridDivisor = divisor; Refresh(); }
    
    // Default bank for auto-hitnormal placement
    void SetDefaultHitnormalBank(std::optional<SampleSet> bank) { defaultHitnormalBank = bank; }
    
    // Rendering
    void OnPaint(wxPaintEvent& evt);
    void OnMouseEvents(wxMouseEvent& evt);
    void OnMouseWheel(wxMouseEvent& evt); // Custom Zoom/Scroll
    
    void UpdateVirtualSize();
    
    void SetWaveform(const std::vector<float>& peaks, double duration);

    // Interaction (Public for MainFrame Menu)
    void Undo() { undoManager.Undo(); Refresh(); } // Add Refresh just in case commands don't trigger it adequately (though they should)
    void Redo() { undoManager.Redo(); Refresh(); }
    
    void CopySelection();
    void CutSelection();
    void PasteAtPlayhead();
    void DeleteSelection();
    void SelectAll();
    
private:
    Project* project = nullptr;
    ToolType currentTool = ToolType::Select;

    // Undo/Redo
    UndoManager undoManager;
    
    Track* lastFocusedTrack = nullptr;
    
    // Grid & Zoom
    double pixelsPerSecond = ZoomSettings::DefaultPixelsPerSecond;
    int gridDivisor = 4;
    
    // Default bank for auto-hitnormal (nullopt = disabled)
    std::optional<SampleSet> defaultHitnormalBank;
    
    int trackHeight = TrackLayout::MasterTrackHeight;
    
    const int rulerHeight = TrackLayout::RulerHeight;
    const int masterTrackHeight = TrackLayout::MasterTrackHeight;
    const int headerHeight = TrackLayout::HeaderHeight; // ruler + master
    
    double playheadPosition = 0.0;
    bool isDraggingPlayhead = false;
    
    // Loop
    double loopStart = -1.0;
    double loopEnd = -1.0;
    bool isDraggingLoop = false; // Dragging in ruler
    
    // Selection
    std::set<std::pair<Track*, int>> selection; 
    std::set<std::pair<Track*, int>> baseSelection; // Selection before marquee drag starts 
    
    // Dragging
    bool isDragging = false;
    wxPoint dragStartPos; // Screen coords
    Track* dragStartTrack = nullptr;
    double dragStartTime = 0.0;
    
    struct DragGhost {
        Event evt;
        Track* originalTrack; 
        int originalRowIndex;
        Track* targetTrack; // Calculated during drag
        double originalTime;
    };
    std::vector<DragGhost> dragGhosts;
    
    // Clipboard
    struct ClipboardItem {
         Event evt;
         int relativeRow; // Offset from the "top" track in selection
         double relativeTime; // Offset from first event time
    };
    std::vector<ClipboardItem> clipboard;

    void OnKeyDown(wxKeyEvent& evt);
    
    // Marquee
    bool isMarquee = false;
    wxPoint marqueeStartPos;
    wxRect marqueeRect; // Normalized
    void PerformMarqueeSelect(const wxRect& rect);
    
    // Waveform
    std::vector<float> waveformPeaks;
    double audioDuration = 0.0;

    // Helpers
    std::vector<Track*> GetVisibleTracks();
    void ValidateHitsounds();
    
    // Find or create a HitNormal track for the given bank with matching volume
    Track* FindOrCreateHitnormalTrack(SampleSet bank, double volume);
    
    const Project::TimingPoint* GetTimingPointAt(double time);
    double SnapToGrid(double time);
    
    // Drag helpers
    struct HitResult {
        Track* visualTrack = nullptr; // The track row clicked physically
        Track* logicalTrack = nullptr; // The actual track owning the event
        int eventIndex = -1;
        bool isValid() const { return logicalTrack != nullptr && eventIndex != -1; }
    };
    HitResult GetEventAt(const wxPoint& pos);

    Track* GetTrackAtY(int y);
    Track* GetEffectiveTargetTrack(Track* t);
    
    // Coordinate helpers
    
    // Coordinate helpers
    int timeToX(double time) const;
    double xToTime(int x) const;
    
    wxDECLARE_EVENT_TABLE();
};
