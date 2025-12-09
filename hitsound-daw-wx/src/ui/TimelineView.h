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
    
    
    std::function<void(double start, double end)> OnLoopPointsChanged;
    
    
    std::function<void(double pixelsPerSecond)> OnZoomChanged;
    
    
    std::function<void()> OnTracksModified;
    
    
    std::function<void(double time)> OnPlayheadScrubbed;
    
    void SetProject(Project* p);
    void SetTool(ToolType tool) { currentTool = tool; }
    void SetPlayheadPosition(double time);
    
    void SetGridDivisor(int divisor) { gridDivisor = divisor; Refresh(); }
    
    
    void SetDefaultHitnormalBank(std::optional<SampleSet> bank) { defaultHitnormalBank = bank; }
    
    
    void OnPaint(wxPaintEvent& evt);
    void OnMouseEvents(wxMouseEvent& evt);
    void OnMouseWheel(wxMouseEvent& evt); 
    
    void UpdateVirtualSize();
    
    void SetWaveform(const std::vector<float>& peaks, double duration);

    
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

    
    TimelineController controller;
    
    
    double pixelsPerSecond = ZoomSettings::DefaultPixelsPerSecond;
    int gridDivisor = 4;
    
    
    std::optional<SampleSet> defaultHitnormalBank;
    
    int trackHeight = TrackLayout::MasterTrackHeight;
    
    const int rulerHeight = TrackLayout::RulerHeight;
    const int masterTrackHeight = TrackLayout::MasterTrackHeight;
    const int headerHeight = TrackLayout::HeaderHeight; 
    
    double playheadPosition = 0.0;
    bool isDraggingPlayhead = false;
    
    
    double loopStart = -1.0;
    double loopEnd = -1.0;
    bool isDraggingLoop = false; 
    
    
    std::set<std::pair<uint64_t, uint64_t>> selection; 
    std::set<std::pair<uint64_t, uint64_t>> baseSelection; 
    
    
    bool isDragging = false;
    wxPoint dragStartPos; 
    uint64_t dragStartTrackId = 0;
    double dragStartTime = 0.0;
    
    struct DragGhost {
        Event evt;
        uint64_t originalTrackId;
        int originalRowIndex;
        uint64_t targetTrackId;  
        double originalTime;
    };
    std::vector<DragGhost> dragGhosts;
    
    
    struct ClipboardItem {
         Event evt;
         int relativeRow; 
         double relativeTime; 
    };
    std::vector<ClipboardItem> clipboard;

    void OnKeyDown(wxKeyEvent& evt);
    
    
    bool isMarquee = false;
    wxPoint marqueeStartPos;
    wxRect marqueeRect; 
    void PerformMarqueeSelect(const wxRect& rect);
    
    
    std::vector<float> waveformPeaks;
    double audioDuration = 0.0;

    
    std::vector<Track*> GetVisibleTracks();
    void ValidateHitsounds();
    
    
    Track* FindOrCreateHitnormalTrack(SampleSet bank, double volume);
    
    
    Track* FindTrackById(uint64_t id);
    
    const Project::TimingPoint* GetTimingPointAt(double time);
    double SnapToGrid(double time);
    
    
    struct HitResult {
        Track* visualTrack = nullptr; 
        Track* logicalTrack = nullptr; 
        int eventIndex = -1;
        bool isValid() const { return logicalTrack != nullptr && eventIndex != -1; }
    };
    HitResult GetEventAt(const wxPoint& pos);

    Track* GetTrackAtY(int y);
    Track* GetEffectiveTargetTrack(Track* t);
    
    
    int timeToX(double time) const;
    double xToTime(int x) const;
    
    
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
    
    
    void HandleLeftDown(wxMouseEvent& evt, const wxPoint& pos);
    void HandleDragging(wxMouseEvent& evt, const wxPoint& pos);
    void HandleLeftUp(wxMouseEvent& evt, const wxPoint& pos);
    void HandleRightDown(const wxPoint& pos);
    
    
    void PlaceEvent(Track* target, double time);
    
    wxDECLARE_EVENT_TABLE();
};
