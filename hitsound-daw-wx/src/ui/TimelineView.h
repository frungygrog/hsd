#pragma once
#include <wx/wx.h>
#include "../model/Project.h"
#include "TimelineController.h"
#include <set>
#include <utility>
#include <vector>
#include <map>
#include <algorithm>
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

    // Interaction (Public for MainFrame Menu) - delegates to controller
    void Undo() { controller.Undo(); }
    void Redo() { controller.Redo(); }
    
    UndoManager& GetUndoManager() { return controller.GetUndoManager(); }
    TimelineController& GetController() { return controller; }
    
    void CopySelection();
    void CutSelection();
    void PasteAtPlayhead();
    void DeleteSelection();
    void SelectAll();
    
private:
    Project* project = nullptr;
    ToolType currentTool = ToolType::Select;

    // Controller handles business logic (selection, clipboard, undo, validation)
    TimelineController controller;
    
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
    
    // Selection - uses {trackId, eventId} pairs for pointer stability
    std::set<std::pair<uint64_t, uint64_t>> selection; 
    std::set<std::pair<uint64_t, uint64_t>> baseSelection; // Selection before marquee drag starts 
    
    // Dragging
    bool isDragging = false;
    wxPoint dragStartPos; // Screen coords
    uint64_t dragStartTrackId = 0;
    double dragStartTime = 0.0;
    
    struct DragGhost {
        Event evt;
        uint64_t originalTrackId;
        int originalRowIndex;
        uint64_t targetTrackId;  // Calculated during drag (0 = same as original)
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
    
    // Find a track by its unique ID (searches recursively)
    Track* FindTrackById(uint64_t id);
    
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
    int timeToX(double time) const;
    double xToTime(int x) const;
    
    // Painting helpers
    void DrawRuler(wxDC& dc, const wxSize& size, double visStart, double visEnd);
    void DrawMasterTrack(wxDC& dc, const wxSize& size, int viewStartPx, int viewEndPx);
    void DrawTrackBackgrounds(wxDC& dc, const wxSize& size, const std::vector<Track*>& visibleTracks);
    void DrawGrid(wxDC& dc, const wxSize& size, double visStart, double visEnd);
    void DrawTimingPoints(wxDC& dc, const wxSize& size, double totalSeconds);
    void DrawEvents(wxDC& dc, const std::vector<Track*>& visibleTracks, double visStart, double visEnd);
    void DrawLoopRegion(wxDC& dc, const wxSize& size);
    void DrawDragGhosts(wxDC& dc, const std::vector<Track*>& visible);
    void DrawMarquee(wxDC& dc);
    void DrawPlayhead(wxDC& dc, const wxSize& size);
    
    // Mouse event helpers
    void HandleLeftDown(wxMouseEvent& evt, const wxPoint& pos);
    void HandleDragging(wxMouseEvent& evt, const wxPoint& pos);
    void HandleLeftUp(wxMouseEvent& evt, const wxPoint& pos);
    void HandleRightDown(const wxPoint& pos);
    
    // Event placement helper
    void PlaceEvent(Track* target, double time);
    
    wxDECLARE_EVENT_TABLE();
};
