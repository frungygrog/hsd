#pragma once
#include <wx/wx.h>
#include "../model/Track.h"

struct Project;
class TimelineView;

class TrackList : public wxPanel
{
public:
    TrackList(wxWindow* parent);

    void SetProject(Project* p);
    void SetVerticalScrollOffset(int y);
    void SetTimelineView(TimelineView* view) { timelineView = view; }

    int GetTotalContentHeight() const;
    std::vector<Track*> GetVisibleTracks();
    int GetTrackHeight(const Track& track);

private:
    void OnPaint(wxPaintEvent& evt);
    void OnMouseEvents(wxMouseEvent& evt);
    void OnContextMenu(wxContextMenuEvent& evt);
    void OnCaptureLost(wxMouseCaptureLostEvent& evt);
    void OnSize(wxSizeEvent& evt);

    // Painting helpers
    void DrawHeader(wxDC& dc, int& y, int width);
    void DrawTrackRow(wxDC& dc, Track& track, int& y, int width, int indent);
    void DrawTrackControls(wxDC& dc, Track& track, int y, int width, int indent);
    void DrawSlider(wxDC& dc, Track& track, int y, int width);
    void DrawPrimarySelector(wxDC& dc, Track& track, int y, int width);
    void DrawAbbreviation(wxDC& dc, Track& track, int y, int width, int height);
    void DrawDropIndicator(wxDC& dc, int width, int headerHeight);

    // Hit testing
    Track* FindTrackAtY(int y, int& outIndent, Track** outParent = nullptr);
    bool HandleHeaderClick(int clickX, int clickY);
    bool HandleTrackClick(Track& track, int clickX, int localY, int y, int width, int indent);

    // Context menus
    void ShowParentContextMenu(Track* track);
    void ShowChildContextMenu(Track* track);

    // Track operations
    void AddChildToTrack(Track* parent);
    void EditTrack(Track* track);
    void DeleteTrack(Track* track, Track* parent);

    // Utilities
    static std::pair<std::string, bool> GetAbbreviation(SampleSet s, SampleType t);
    void UpdateTrackNameWithVolume(Track& track, double volume);

    Project* project = nullptr;
    TimelineView* timelineView = nullptr;
    int scrollOffsetY = 0;

    // Child track volume slider drag state
    bool isDraggingSlider = false;
    Track* sliderTrack = nullptr;

    // Track reordering drag state
    bool isDraggingTrack = false;
    Track* dragSourceTrack = nullptr;
    int dragSourceY = 0;
    int dragCurrentY = 0;

    struct DropTarget {
        Track* parent = nullptr;
        int index = -1;
        bool isValid = false;
    };
    DropTarget currentDropTarget;

    wxBitmap trackIcon;
    wxBitmap groupingIcon;

    wxDECLARE_EVENT_TABLE();
};
