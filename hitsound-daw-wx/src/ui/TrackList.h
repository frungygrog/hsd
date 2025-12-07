#pragma once
#include <wx/wx.h>
#include "../model/Track.h"

// Forward declarations
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
    void OnSize(wxSizeEvent& evt);
    
    Project* project = nullptr;
    TimelineView* timelineView = nullptr;
    int scrollOffsetY = 0;
    
    // Slider Dragging
    bool isDraggingSlider = false;
    Track* sliderTrack = nullptr;
    
    // Track Reordering
    bool isDraggingTrack = false;
    Track* dragSourceTrack = nullptr;
    int dragSourceY = 0; 
    int dragCurrentY = 0; 
    
    struct DropTarget {
        Track* parent = nullptr; // nullptr if root (parent list)
        int index = -1; // Index in the vector (before which to insert)
        bool isValid = false;
    };
    DropTarget currentDropTarget;
    
    wxDECLARE_EVENT_TABLE();
};
