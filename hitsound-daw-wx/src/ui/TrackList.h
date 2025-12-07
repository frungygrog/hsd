#pragma once
#include <wx/wx.h>
#include "../model/Track.h"

struct Project;

// TrackList is a simple wxPanel that mirrors TimelineView's vertical scroll position.
// This ensures perfect sync since there's only one source of truth for scroll state.
class TrackList : public wxPanel
{
public:
    TrackList(wxWindow* parent);
    
    void SetProject(Project* p);
    void SetVerticalScrollOffset(int y); // Set from TimelineView's scroll position
    
    // Helpers
    std::vector<Track*> GetVisibleTracks();
    
    int GetHeaderHeight() const { return 130; } // Ruler + Master
    int GetTotalContentHeight() const; // Calculate total height for TimelineView's virtual size
    
    // Interaction
    bool isDraggingSlider = false;
    Track* sliderTrack = nullptr;

private:
    Project* project = nullptr;
    int scrollOffsetY = 0; // Vertical scroll offset from TimelineView
    int trackHeight = 100;
    
    int GetTrackHeight(const Track& track);
    
    void OnPaint(wxPaintEvent& evt);
    void OnMouseEvents(wxMouseEvent& evt);
    
    wxDECLARE_EVENT_TABLE();
};
