#include "TimelineView.h"
#include "../model/Commands.h" // Include Concrete Commands
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <cmath>
#include <algorithm>
#include <functional>

wxBEGIN_EVENT_TABLE(TimelineView, wxScrolledWindow)
    EVT_PAINT(TimelineView::OnPaint)
    EVT_MOUSEWHEEL(TimelineView::OnMouseWheel)
    EVT_MOUSE_EVENTS(TimelineView::OnMouseEvents)
    EVT_KEY_DOWN(TimelineView::OnKeyDown)
wxEND_EVENT_TABLE()

TimelineView::TimelineView(wxWindow* parent)
    : wxScrolledWindow(parent, wxID_ANY)
    , project(nullptr)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT); // Required for wxAutoBufferedPaintDC
    SetBackgroundColour(*wxBLACK);
    
    // Set a virtual size large enough to scroll
    UpdateVirtualSize();
}

void TimelineView::SetProject(Project* p)
{
    project = p;
    UpdateVirtualSize();
    ValidateHitsounds();
    Refresh();
}

void TimelineView::SetPlayheadPosition(double time)
{
    playheadPosition = time;
    Refresh();
}

void TimelineView::SetWaveform(const std::vector<float>& peaks, double duration)
{
    waveformPeaks = peaks;
    audioDuration = duration;
    UpdateVirtualSize();
    Refresh();
}

void TimelineView::OnPaint(wxPaintEvent& evt)
{
    wxAutoBufferedPaintDC dc(this);
    PrepareDC(dc);
    
    dc.Clear();
    
    if (!project) return;
    
    wxSize size = GetVirtualSize();
    wxSize clientSize = GetClientSize(); // To know visible area for optimization
    
    // Viewport Optimization
    int scrollX, scrollY;
    GetViewStart(&scrollX, &scrollY);
    int ppuX, ppuY;
    GetScrollPixelsPerUnit(&ppuX, &ppuY);
    if (ppuX == 0) ppuX = 10;
    
    int viewStartPx = scrollX * ppuX;
    int viewEndPx = viewStartPx + clientSize.x;
    
    double visStart = xToTime(viewStartPx);
    double visEnd = xToTime(viewEndPx);
    
    // -------------------------------------------------------------------------
    // 1. Draw Master Track (Top Header)
    // -------------------------------------------------------------------------
    // Defines a fixed area at the top
    // -------------------------------------------------------------------------
    // 1. Draw Header (Ruler + Master Track)
    // -------------------------------------------------------------------------
    // RULER
    wxRect rulerRect(0, 0, size.x, rulerHeight);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(wxColour(40, 40, 40))); // Ruler BG
    dc.DrawRectangle(rulerRect);
    
    // Draw Ruler Ticks (Basic 1s interval for now, or use GridDivisor logic?)
    dc.SetPen(wxPen(wxColour(150, 150, 150)));
    dc.SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    dc.SetTextForeground(wxColour(200, 200, 200));
    
    double startTime = xToTime(0); // If we optimize visible area
    double endTime = xToTime(size.x);
    // Draw every second?
    double rulerStart = std::floor(visStart);
    for (double t = rulerStart; t <= visEnd; t += 1.0)
    {
        int rx = timeToX(t);
        dc.DrawLine(rx, rulerHeight - 10, rx, rulerHeight);
        dc.DrawText(wxString::Format("%.0fs", t), rx + 2, 2);
    }
    
    // MASTER TRACK (Below Ruler)
    int masterY = rulerHeight;
    // masterTrackHeight is 100
        
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(wxColour(20, 20, 25))); // Darker background
    dc.DrawRectangle(0, masterY, size.x, masterTrackHeight);
    
    // Loop Region removed from here to draw over tracks later

    
    // Draw Waveform
    if (!waveformPeaks.empty() && audioDuration > 0)
    {
        dc.SetPen(wxPen(wxColour(100, 150, 200), 1));
        
        int centerY = masterY + masterTrackHeight / 2;
        float heightScale = masterTrackHeight / 2.0f;
        
        // The waveform peaks are distributed over 'audioDuration'
        // x = 0 is time 0. x = pixelsPerSecond * duration.
        // We have 'waveformPeaks.size()' samples.
        
        double totalWidth = audioDuration * pixelsPerSecond;
        double samplesPerPixel = (double)waveformPeaks.size() / totalWidth;
        
        if (samplesPerPixel > 0)
        {
            int startX = std::max(0, viewStartPx);
            int endX = std::min((int)totalWidth, viewEndPx);
            
            for (int x = startX; x < endX; ++x)
            {
                // Simple sampling
                int infoIndex = (int)(x * samplesPerPixel);
                if (infoIndex >= 0 && infoIndex < waveformPeaks.size())
                {
                    float val = waveformPeaks[infoIndex];
                    int h = (int)(val * heightScale);
                    dc.DrawLine(x, centerY - h, x, centerY + h);
                }
            }
        }
    }
    
    // -------------------------------------------------------------------------
    // 2. Draw Tracks
    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------
    // 2. Draw Tracks
    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------
    // 2. Draw Tracks (Backgrounds)
    // -------------------------------------------------------------------------
    // Pass 1: Backgrounds & Separators
    std::vector<Track*> visibleTracks = GetVisibleTracks();
    int y = headerHeight; 

    for (Track* track : visibleTracks)
    {
        bool isChild = (track->name.find("%)") != std::string::npos);
        int currentHeight = isChild ? 40 : 80;
        bool isParentExpanded = !track->children.empty() && track->isExpanded;

        // Background
        dc.SetPen(*wxTRANSPARENT_PEN);
        if (isParentExpanded)
            dc.SetBrush(wxBrush(wxColour(25, 25, 25), wxBRUSHSTYLE_BDIAGONAL_HATCH)); 
        else
            dc.SetBrush(wxBrush(wxColour(30, 30, 30)));
            
        dc.DrawRectangle(0, y, size.x, currentHeight);
        
        // Separator removed - moved to Grid Overlay pass
        
        y += currentHeight;
    }

    // -------------------------------------------------------------------------
    // 3. Draw Grid (Overlay) - NOW RENDERED BEFORE EVENTS
    // -------------------------------------------------------------------------

    // 3a. Horizontal Grid Lines
    {
        int gy = headerHeight;
        dc.SetPen(wxPen(wxColour(60, 60, 60)));
        for (Track* track : visibleTracks)
        {
             bool isChild = (track->name.find("%)") != std::string::npos);
             int h = isChild ? 40 : 80;
             
             // Draw Bottom Line
             dc.DrawLine(0, gy + h, size.x, gy + h);
             gy += h;
        }
    }
    
    double totalSeconds = size.x / pixelsPerSecond;
    if (totalSeconds < 1.0) totalSeconds = 10.0;
    
    // Multi-Timing Point Grid Drawing
    if (project && !project->timingPoints.empty())
    {
        // 1. Filter relevant uninherited points
        std::vector<const Project::TimingPoint*> sections;
        for (const auto& tp : project->timingPoints) {
            if (tp.uninherited) sections.push_back(&tp);
        }
        
        if (!sections.empty())
        {
             double visibleGridStart = visStart; 
             double visibleGridEnd = visEnd;
             
             int startIndex = 0;
             for (int i=0; i<(int)sections.size(); ++i) {
                 if (sections[i]->time / 1000.0 <= visStart) startIndex = i;
                 else break;
             }
             
             for (int i = startIndex; i < (int)sections.size(); ++i)
             {
                 const auto* tp = sections[i];
                 double sectionStart = tp->time / 1000.0;
                 double sectionEnd = (i + 1 < (int)sections.size()) ? (sections[i+1]->time / 1000.0) : totalSeconds + 10.0;
                 
                 double beatLen = tp->beatLength / 1000.0;
                 if (beatLen <= 0.001) beatLen = 0.5;
                 
                 double step = beatLen / gridDivisor;
                 
                 double visibleSectionStart = std::max(sectionStart, visStart);
                 double visibleSectionEnd = std::min(sectionEnd, visEnd);
                 
                 if (visibleSectionStart >= visibleSectionEnd) continue;
                 
                 double n = std::ceil((visibleSectionStart - sectionStart) / step);
                 double t = sectionStart + n * step;
                 
                 while (t < visibleSectionEnd)
                 {
                      int x = timeToX(t);
                      
                      // Relative to sectionStart
                      double rel = t - sectionStart;
                      double beats = rel / beatLen;
                      double remainder = std::abs(beats - std::round(beats));
                      bool isMajor = (remainder < 0.001); // Close to integer beat
                      
                      if (isMajor)
                          dc.SetPen(wxPen(wxColour(110, 110, 110, 120)));
                      else
                          dc.SetPen(wxPen(wxColour(60, 60, 60, 60)));
                      
                      dc.DrawLine(x, 0, x, size.y);
                      
                      t += step;
                 }
             }
        }
    }
    
    // Timing Points (Green Lines)
    if (project && !project->timingPoints.empty())
    {
        dc.SetPen(wxPen(wxColour(0, 255, 0), 2));
        for (const auto& tp : project->timingPoints)
        {
            if (!tp.uninherited) continue;
            double timeSec = tp.time / 1000.0;
            if (timeSec >= 0 && timeSec <= totalSeconds)
            {
                int x = timeToX(timeSec);
                dc.DrawLine(x, 0, x, size.y);
            }
        }
    }

    // -------------------------------------------------------------------------
    // 4. Draw Events (Pass 2)
    // -------------------------------------------------------------------------
    y = headerHeight; // Reset Y
    for (Track* track : visibleTracks)
    {
        bool isChild = (track->name.find("%)") != std::string::npos);
        int currentHeight = isChild ? 40 : 80;
        bool isParentExpanded = !track->children.empty() && track->isExpanded;
        
        // Draw Events
        if (!isParentExpanded)
        {
            // Helper to draw a list of events
            auto drawEvents = [&](const std::vector<Event>& events, Track* srcTrack, wxColour color) {
                dc.SetPen(*wxTRANSPARENT_PEN);
                
                for (int i = 0; i < (int)events.size(); ++i)
                {
                    const auto& event = events[i];
                    if (event.time < visStart - 0.5 || event.time > visEnd + 0.5) continue;
                    int x = timeToX(event.time); 
                    
                    bool isSelected = selection.count({srcTrack, i});
                    
                    if (isSelected)
                    {
                        if (!event.isValid) dc.SetBrush(wxBrush(wxColour(255, 165, 0))); // Orange for invalid & selected
                        else dc.SetBrush(wxBrush(wxColour(255, 255, 0))); // Yellow for valid & selected
                    }
                    else
                    {
                        if (!event.isValid) dc.SetBrush(wxBrush(wxColour(255, 50, 50)));
                        else dc.SetBrush(wxBrush(color));
                    }
                    
                    dc.DrawRectangle(x - 2, y + 2, 4, currentHeight - 4); // Adjusted
                    
                    // Selection Highlight Border (White) - kept for extra visibility
                    if (isSelected)
                    {
                        dc.SetBrush(*wxTRANSPARENT_BRUSH);
                        dc.SetPen(wxPen(*wxWHITE, 2));
                        dc.DrawRectangle(x - 4, y, 8, currentHeight);
                        dc.SetPen(*wxTRANSPARENT_PEN);
                    }
                }
            };
    
            if (!track->isExpanded && !track->children.empty())
            {
                 // Collapsed Parent - Draw children aggregated
                 for (Track& child : track->children)
                 {
                      drawEvents(child.events, &child, wxColour(100, 200, 255, 100));
                 }
            }
            else
            {
                drawEvents(track->events, track, wxColour(100, 200, 255));
            }
        }
        
        y += currentHeight;
    }
    

    
    // -------------------------------------------------------------------------
    // 3b. Draw Loop Region (Overlay)
    // -------------------------------------------------------------------------
    if (loopStart >= 0 && loopEnd > loopStart)
    {
        int lx = timeToX(loopStart);
        int lw = timeToX(loopEnd) - lx;
        if (lw < 1) lw = 1;
        
        // Draw in Ruler - subtle green tint
        dc.SetBrush(wxBrush(wxColour(80, 180, 80)));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(lx, 0, lw, rulerHeight);
        
        // Draw subtle overlay on tracks using a hatched pattern
        // Using a semi-transparent brush if possible, otherwise hatch
        // wxGCDC can do transparent, but we are using wxDC (AutoBufferedPaintDC).
        // Standard wxDC doesn't support alpha on all platforms well, but Hatch is safe.
        dc.SetBrush(wxBrush(wxColour(0, 255, 0), wxBRUSHSTYLE_FDIAGONAL_HATCH));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(lx, rulerHeight, lw, size.y - rulerHeight);
    }
    
    // -------------------------------------------------------------------------
    // 5. Draw Drag Ghosts
    // -------------------------------------------------------------------------
    if (isDragging && !dragGhosts.empty())
    {
        int gy = headerHeight;
        std::vector<Track*> visible = GetVisibleTracks();
        
        for (Track* t : visible)
        {
            bool isChild = (t->name.find("%)") != std::string::npos);
            int currentHeight = isChild ? 40 : 80;
            
            // Draw ghosts targetting this track (or its children if collapsed)
            for (const auto& ghost : dragGhosts)
            {
                Track* target = ghost.targetTrack ? ghost.targetTrack : ghost.originalTrack;
                bool match = (target == t);
                if (!match && !t->isExpanded && !t->children.empty())
                {
                     // Check children if parent is collapsed
                     for(auto& c : t->children) if(&c == target) { match = true; break; }
                }
                
                if (match)
                {
                     int x = timeToX(ghost.evt.time);
                     
                     // Visual style for ghosts
                     dc.SetPen(wxPen(wxColour(255, 255, 255), 1, wxPENSTYLE_SHORT_DASH));
                     if (ghost.evt.isValid)
                        dc.SetBrush(wxBrush(wxColour(255, 255, 0, 180))); 
                     else
                        dc.SetBrush(wxBrush(wxColour(255, 100, 0, 180)));
                        
                     dc.DrawRectangle(x - 2, gy + 2, 4, currentHeight - 4);
                }
            }
            gy += currentHeight;
        }
    }

    // Marquee
    if (isMarquee)
    {
        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::CreateFromUnknownDC(dc));
        if (gc)
        {
            gc->SetPen(wxPen(wxColour(0, 120, 215), 1));
            gc->SetBrush(wxBrush(wxColour(0, 120, 215, 64))); // Semi-transparent blue
            gc->DrawRectangle(marqueeRect.x, marqueeRect.y, marqueeRect.width, marqueeRect.height);
        }
        else
        {
            // Fallback if GC fails
            dc.SetPen(wxPen(*wxWHITE, 1, wxPENSTYLE_DOT));
            dc.SetBrush(wxBrush(wxColour(255, 255, 255, 50))); 
            dc.DrawRectangle(marqueeRect);
        }
    }

    // -------------------------------------------------------------------------
    // 4. Draw Playhead
    // -------------------------------------------------------------------------
    int px = timeToX(playheadPosition);
    dc.SetPen(wxPen(wxColour(255, 0, 0)));
    dc.DrawLine(px, 0, px, size.y);
    
    wxPoint triangle[] = {
        wxPoint(px - 5, 0),
        wxPoint(px + 5, 0),
        wxPoint(px, 10)
    };
    dc.SetBrush(wxBrush(wxColour(255, 0, 0)));
    dc.DrawPolygon(3, triangle);
}

void TimelineView::OnMouseEvents(wxMouseEvent& evt)
{
    if (!project) return;
    
    wxPoint pos = evt.GetPosition();
    CalcUnscrolledPosition(pos.x, pos.y, &pos.x, &pos.y);
    
    if (evt.LeftDown())
    {
        SetFocus();
        
        // 1. Check Header (Ruler vs Master)
        if (pos.y < rulerHeight)
        {
             // Check for Playhead Handle (Triangle) Hit
             int px = timeToX(playheadPosition);
             if (std::abs(pos.x - px) <= 8) // Hit test +/- 8 pixels
             {
                 isDraggingPlayhead = true;
                 return;
             }
             
             // LOOP SELECTION
             double t = SnapToGrid(xToTime(pos.x));
             loopStart = t;
             loopEnd = t; // Reset end
             isDraggingLoop = true;
             Refresh();
             return;
        }
        else if (pos.y >= rulerHeight && pos.y < headerHeight)
        {
             // Seek (Master Track)
             double t = SnapToGrid(xToTime(pos.x));
             SetPlayheadPosition(t); 
             isDraggingPlayhead = true;
             return;
        }
        
        HitResult hit = GetEventAt(pos);
        Track* hitTrack = GetTrackAtY(pos.y); // Still useful for empty space clicks
        if (hitTrack) lastFocusedTrack = hitTrack;
        
        // 3. Selection Logic
        bool ctrl = evt.ControlDown();
        
        if (hit.isValid())
        {
            // Clicked an event (Expanded or Collapsed Child)
            std::pair<Track*, int> id = {hit.logicalTrack, hit.eventIndex};
            
            if (!ctrl && selection.find(id) == selection.end())
            {
                selection.clear();
                selection.insert(id);
            }
            else if (ctrl)
            {
                 if (selection.count(id)) selection.erase(id);
                 else selection.insert(id);
            }
            
            // Start Drag
            if (!selection.empty())
            {
                isDragging = true;
                dragStartPos = pos;
                dragStartTrack = hit.visualTrack; // Critical: Use the VISUAL track for row calculation
                dragStartTime = SnapToGrid(xToTime(pos.x));
                
                // Extract events to ghosts
                dragGhosts.clear();
                
                std::map<Track*, std::vector<int>> toRemove;
                for (auto& sel : selection) {
                    toRemove[sel.first].push_back(sel.second);
                }
                
                std::vector<Track*> visible = GetVisibleTracks();
                
                for (auto& pair : toRemove)
                {
                    Track* t = pair.first; // Logical Track
                    std::vector<int>& indices = pair.second;
                    std::sort(indices.rbegin(), indices.rend());
                    
                    // Find Visual Row Index
                    // If t is in visible, use it.
                    // If t is not in visible, check if it's a child of a collapsed visible track.
                    int rowIndex = -1;
                    
                    for(int r=0; r<(int)visible.size(); ++r) 
                    {
                        if(visible[r] == t) 
                        { 
                            rowIndex = r; 
                            break; 
                        }
                        // Check children if collapsed
                        if (!visible[r]->isExpanded) 
                        {
                            for(const auto& child : visible[r]->children) 
                            {
                                if (&child == t) 
                                { 
                                    rowIndex = r; 
                                    goto found_row; 
                                }
                            }
                        }
                    }
                    found_row:;

                    // If row index is -1, something is weird (maybe track hidden?), but we can default to 0 or something safe.
                    if (rowIndex == -1) rowIndex = 0;

                    for (int idx : indices)
                    {
                        DragGhost g;
                        g.evt = t->events[idx];
                        g.originalTime = g.evt.time;
                        g.originalTrack = t; // Logical Source
                        g.originalRowIndex = rowIndex; // Visual Row
                        g.targetTrack = t; // Default to self
                        dragGhosts.push_back(g);
                        
                        t->events.erase(t->events.begin() + idx);
                    }
                }
                selection.clear(); 
                Refresh();
            }
        }
        else
        {
            // Clicked empty space
            if (currentTool == ToolType::Draw && hitTrack)
            {
                 // SMART BEHAVIOR: We checked GetEventAt above. If we are here, we are NOT on an event.
                 // So we can place.
                 
                 Track* target = GetEffectiveTargetTrack(hitTrack);
                 if (!target) target = hitTrack;

                 double t = SnapToGrid(xToTime(pos.x));
                 Event newEvent;
                 newEvent.time = t;
                 
                 auto refreshFn = [this](){ ValidateHitsounds(); Refresh(); };
                 undoManager.PushCommand(std::make_unique<AddEventCommand>(target, newEvent, refreshFn));
                 return;
            }
            
            if (!ctrl) selection.clear();
            
            isMarquee = true;
            marqueeStartPos = pos;
            marqueeRect = wxRect(pos, pos);
            
            // Store base selection for Ctrl+Drag
            if (ctrl) baseSelection = selection;
            else baseSelection.clear();
            
            Refresh();
        }
    }
    else if (evt.Dragging())
    {
        if (isDraggingPlayhead)
        {
             double t = SnapToGrid(xToTime(pos.x));
             SetPlayheadPosition(t);
        }
        else if (isDraggingLoop)
        {
            double t = SnapToGrid(xToTime(pos.x));
            loopEnd = t;
            Refresh();
            
            if (OnLoopPointsChanged)
            {
                 double s = std::min(loopStart, loopEnd);
                 double e = std::max(loopStart, loopEnd);
                 OnLoopPointsChanged(s, e);
            }
        }
        else if (isDragging)
        {
             double curTime = SnapToGrid(xToTime(pos.x));
             double timeDelta = curTime - dragStartTime;
             
             Track* curTrack = GetTrackAtY(pos.y);
             
             // Calculate Row Delta using Visual Tracks
             int startRow = -1;
             int curRow = -1;
             std::vector<Track*> visible = GetVisibleTracks();
             
             for(int i=0; i<(int)visible.size(); ++i) {
                 if (visible[i] == dragStartTrack) startRow = i;
                 if (visible[i] == curTrack) curRow = i;
             }
             
             int rowDelta = 0;
             if (startRow != -1 && curRow != -1) rowDelta = curRow - startRow;
             
             // Update Ghosts
             for (auto& g : dragGhosts)
             {
                 // Time
                 g.evt.time = std::max(0.0, g.originalTime + timeDelta);
                 
                 // Target Track
                 int targetRowIndex = g.originalRowIndex + rowDelta;
                 
                 Track* visualTarget = nullptr;
                 if (targetRowIndex >= 0 && targetRowIndex < (int)visible.size())
                 {
                     visualTarget = visible[targetRowIndex];
                 }
                 else
                 {
                     // Clamp
                     if (targetRowIndex < 0) targetRowIndex = 0;
                     if (targetRowIndex >= (int)visible.size()) targetRowIndex = visible.size() - 1;
                     visualTarget = visible[targetRowIndex];
                 }
                 
                 g.targetTrack = GetEffectiveTargetTrack(visualTarget);
             }
             Refresh();
        }
        else if (isMarquee)
        {
            // Update Rect
            int x = std::min(marqueeStartPos.x, pos.x);
            int y = std::min(marqueeStartPos.y, pos.y);
            int w = std::abs(pos.x - marqueeStartPos.x);
            int h = std::abs(pos.y - marqueeStartPos.y);
            marqueeRect = wxRect(x, y, w, h);
            
            // Real-time Update
            PerformMarqueeSelect(marqueeRect);
            
            Refresh();
        }
    }

    else if (evt.RightDown())
    {
        // Right Click Delete - ALSO needs Smart Hit Test
        HitResult hit = GetEventAt(pos);
        if (hit.isValid())
        {
             Track* t = hit.logicalTrack;
             int idx = hit.eventIndex;
             
             if (idx >= 0 && idx < (int)t->events.size()) {
                 std::vector<RemoveEventsCommand::Item> items;
                 items.push_back({t, t->events[idx]});
                 
                 auto refreshFn = [this](){ selection.clear(); ValidateHitsounds(); Refresh(); };
                 undoManager.PushCommand(std::make_unique<RemoveEventsCommand>(items, refreshFn));
             }
        }
    }
    else if (evt.LeftUp())
    {
        if (isDraggingPlayhead)
        {
            isDraggingPlayhead = false;
        }
        else if (isDraggingLoop)
        {
            if (loopEnd < loopStart) std::swap(loopStart, loopEnd);
            isDraggingLoop = false;
            Refresh();
            if (OnLoopPointsChanged) OnLoopPointsChanged(loopStart, loopEnd);
        }
        else if (isDragging)
        {
            // Commit Drag
            selection.clear();
            
            // 1. Restore Originals temporarily so Command can start from clean state
            for (const auto& g : dragGhosts) {
                // We use originalTrack and originalTime to reconstruct
                Event origEvt = g.evt;
                origEvt.time = g.originalTime; 
                // Note: If we had other mutable props, we'd need to be careful, but currently only time moves.
                g.originalTrack->events.push_back(origEvt);
            }
            
            // 2. Build Move Command
            std::vector<MoveEventsCommand::MoveInfo> moves;
            for (auto& g : dragGhosts)
            {
                Track* target = g.targetTrack;
                if (!target) target = g.originalTrack; // Fallback
                
                if (target)
                {
                    Event newEvt = g.evt;
                    // Ensure time is finalized? It should be from Dragging.
                    
                    Event origEvt = g.evt;
                    origEvt.time = g.originalTime;
                    
                    moves.push_back({g.originalTrack, origEvt, target, newEvt});
                }
            }
            
            auto refreshFn = [this](){ ValidateHitsounds(); Refresh(); };
            undoManager.PushCommand(std::make_unique<MoveEventsCommand>(moves, refreshFn));
            
            // 3. Selection Update (Post-Move)
            // The MoveCommand::Do() has executed. We can re-select the new events.
            // We need to find them. 
            selection.clear();
            for (auto& m : moves) {
                // Find m.newEvent in m.newTrack
                auto& evts = m.newTrack->events;
                // Assuming pushed back at end? Or sorted? 
                // Events vector order is not guaranteed sorted yet (we should probably sort them eventually)
                // But generally keys are pushed back.
                // Safest to search.
                for (int i = 0; i < (int)evts.size(); ++i) {
                     if (evts[i].time == m.newEvent.time) { // Weak match but ok for now
                         selection.insert({m.newTrack, i});
                         break; // One match
                     }
                }
            }
            
            dragGhosts.clear();
            isDragging = false;
        }
        else if (isMarquee)
        {
            PerformMarqueeSelect(marqueeRect);
            isMarquee = false;
            baseSelection.clear();
            Refresh();
        }
    }
    evt.Skip();
}

// Helpers
void CollectVisibleTracks(std::vector<Track*>& out, std::vector<Track>& tracks)
{
    for (auto& track : tracks)
    {
        out.push_back(&track);
        if (track.isExpanded)
        {
            CollectVisibleTracks(out, track.children);
        }
    }
}

std::vector<Track*> TimelineView::GetVisibleTracks()
{
    std::vector<Track*> visible;
    if (project)
    {
        CollectVisibleTracks(visible, project->tracks);
    }
    return visible;
}

TimelineView::HitResult TimelineView::GetEventAt(const wxPoint& pos)
{
    HitResult result;
    result.visualTrack = GetTrackAtY(pos.y);
    if (!result.visualTrack) return result;

    Track* vt = result.visualTrack;
    
    // Check visual track events (or parent events if collapsed)
    for (int i = vt->events.size() - 1; i >= 0; --i)
    {
        int x = timeToX(vt->events[i].time);
        if (pos.x >= x - 2 && pos.x <= x + 6)
        {
            result.logicalTrack = vt;
            result.eventIndex = i;
            return result;
        }
    }
    
    // If collapsed, check children
    if (!vt->isExpanded && !vt->children.empty())
    {
        for (Track& child : vt->children)
        {
             for (int i = child.events.size() - 1; i >= 0; --i)
             {
                int x = timeToX(child.events[i].time);
                if (pos.x >= x - 2 && pos.x <= x + 6)
                {
                    result.logicalTrack = &child;
                    result.eventIndex = i;
                    return result;
                }
             }
        }
    }
    return result;
}

Track* TimelineView::GetTrackAtY(int y)
{
    std::vector<Track*> visibleFn = GetVisibleTracks();
    int currentY = headerHeight;
    for (Track* t : visibleFn)
    {
        bool isChild = (t->name.find("%)") != std::string::npos);
        int h = isChild ? 40 : 80;
        
        if (y >= currentY && y < currentY + h)
            return t;
        currentY += h;
    }
    return nullptr;
}

Track* TimelineView::GetEffectiveTargetTrack(Track* t)
{
    if (!t) return nullptr;
    // Always route to primary child if children exist, to keep parents as containers
    if (!t->children.empty())
    {
        if (t->primaryChildIndex >= 0 && t->primaryChildIndex < (int)t->children.size())
            return &t->children[t->primaryChildIndex];
        return &t->children[0]; // Fallback
    }
    return t;
}

const Project::TimingPoint* TimelineView::GetTimingPointAt(double time)
{
    if (!project || project->timingPoints.empty()) return nullptr;
    
    // Sort logic should ideally be enforced in Project loader, but we assume sorted or we search carefully
    // We want the last uninherited timing point <= time.
    
    const Project::TimingPoint* best = nullptr;
    
    // Iterating is fine for now; optimization possible later
    for (const auto& tp : project->timingPoints)
    {
        if (!tp.uninherited) continue;
        
        double tSec = tp.time / 1000.0;
        if (tSec <= time)
        {
            best = &tp;
        }
        else
        {
            // Since we assume they are sorted by time, we can break? 
            // Better safe than sorry if unsorted, but assuming sorted for now is standard.
            // Actually, let's just find the max <= time
            break; 
        }
    }
    
    // If no timing point found before 'time', we might use the first one if it exists?
    // Or just fallback to default BPM/Offset
    return best;
}

void TimelineView::PerformMarqueeSelect(const wxRect& rect)
{
    // Restore base selection first (so we don't lose initial selection in CTRL mode, or clear it in Normal mode)
    selection = baseSelection;
    
    std::vector<Track*> visible = GetVisibleTracks();
    int y = headerHeight;
    
    for (Track* t : visible)
    {
        bool isChild = (t->name.find("%)") != std::string::npos);
        int currentHeight = isChild ? 40 : 80;
        
        // Culling optimization
        if (y + currentHeight < rect.GetTop() || y > rect.GetBottom())
        {
            y += currentHeight;
            continue;
        }
        
        if (!t->isExpanded && !t->children.empty())
        {
            // Check Parent's own events (if any)
            for (int i = 0; i < (int)t->events.size(); ++i)
            {
                int ex = timeToX(t->events[i].time);
                wxRect eventRect(ex - 2, y + 2, 4, currentHeight - 4); 
                if (rect.Intersects(eventRect)) selection.insert({t, i});
            }

            // Check Children's events (Aggregated)
            for (Track& child : t->children)
            {
                 for (int i = 0; i < (int)child.events.size(); ++i)
                 {
                      int ex = timeToX(child.events[i].time);
                      // Use PARENT's Y position 'y'
                      wxRect eventRect(ex - 2, y + 2, 4, currentHeight - 4);
                      if (rect.Intersects(eventRect)) selection.insert({&child, i});
                 }
            }
        }
        else
        {
            // Normal Expanded / Leaf Track
            for (int i = 0; i < (int)t->events.size(); ++i)
            {
                int ex = timeToX(t->events[i].time);
                wxRect eventRect(ex - 2, y + 2, 4, currentHeight - 4); 
                if (rect.Intersects(eventRect)) selection.insert({t, i});
            }
        }
        y += currentHeight;
    }
}

double TimelineView::SnapToGrid(double time)
{
    if (!project) return time;
    
    const Project::TimingPoint* tp = GetTimingPointAt(time);
    
    double beatLen = 0.5; // Default 120BPM
    double startOffset = 0.0;
    
    if (tp)
    {
        startOffset = tp->time / 1000.0;
        beatLen = tp->beatLength / 1000.0; // beatLength is ms per beat
    }
    else if (!project->timingPoints.empty())
    {
         // If before first timing point, project backwards from the first one
         // Find first
         for (const auto& p : project->timingPoints) {
             if (p.uninherited) {
                 startOffset = p.time / 1000.0;
                 beatLen = p.beatLength / 1000.0;
                 break;
             }
         }
    }
    
    // Check BPM sanity
    if (beatLen <= 0.001) beatLen = 0.5; // Avoid div by zero
    
    double snapStep = beatLen / gridDivisor;
    
    // time = startOffset + n * snapStep
    double n = std::round((time - startOffset) / snapStep);
    double snapped = startOffset + n * snapStep;
    
    if (snapped < 0) snapped = 0;
    return snapped;
}

int TimelineView::timeToX(double time) const
{
    return static_cast<int>(time * pixelsPerSecond);
}

double TimelineView::xToTime(int x) const
{
    return static_cast<double>(x) / pixelsPerSecond;
}

void TimelineView::OnKeyDown(wxKeyEvent& evt)
{
    if (evt.GetModifiers() == wxMOD_CONTROL)
    {
        if (evt.GetKeyCode() == 'C')
        {
            CopySelection();
            return;
        }
        else if (evt.GetKeyCode() == 'V')
        {
            PasteAtPlayhead();
            return;
        }
        else if (evt.GetKeyCode() == 'Z')
        {
            undoManager.Undo();
            return;
        }
        else if (evt.GetKeyCode() == 'Y')
        {
            undoManager.Redo();
            return;
        }
        else if (evt.GetKeyCode() == 'B') // Duplicate
        {
            // Implement Duplicate later or now? Let's just hook it up if logic exists, else ignore
             // Duplicate logic is essentially "Copy Selection + Paste at End of Selection"
             // I'll defer implementation to next step
        }
    }
    else if (evt.GetModifiers() == (wxMOD_CONTROL | wxMOD_SHIFT)) 
    {
        if (evt.GetKeyCode() == 'Z') {
            undoManager.Redo();
            return;
        }
    }
    
    if (evt.GetKeyCode() == WXK_DELETE)
    {
        DeleteSelection();
        return;
    }
    
    evt.Skip();
}

void TimelineView::CopySelection()
{
    if (selection.empty()) return;
    
    clipboard.clear();
    
    std::vector<Track*> visible = GetVisibleTracks();
    
    // Helper to find visual row for a generic track (handling collapsed parents)
    auto getVisualRow = [&](Track* t) -> int {
        for (int i = 0; i < (int)visible.size(); ++i) {
            if (visible[i] == t) return i;
            // If visible track is a collapsed parent, check if t is one of its children
            if (!visible[i]->isExpanded && !visible[i]->children.empty()) {
                for (auto& child : visible[i]->children) {
                    if (&child == t) return i;
                }
            }
        }
        return -1;
    };
    
    // 1. Find Min Row and Min Time
    int minRow = 999999;
    double minTime = 1e9;
    
    for (const auto& pair : selection)
    {
        Track* t = pair.first;
        int eventIdx = pair.second;
        if (eventIdx >= 0 && eventIdx < (int)t->events.size())
        {
             double time = t->events[eventIdx].time;
             if (time < minTime) minTime = time;
             
             int row = getVisualRow(t);
             if (row != -1 && row < minRow) minRow = row;
        }
    }
    
    // If we couldn't resolve any rows, abort (shouldn't happen with valid selection)
    if (minRow == 999999) return;
    
    // 2. Populate
    for (const auto& pair : selection)
    {
        Track* t = pair.first;
        int eventIdx = pair.second;
         if (eventIdx >= 0 && eventIdx < (int)t->events.size())
         {
             int row = getVisualRow(t);
             
             if (row != -1)
             {
                 ClipboardItem item;
                 item.evt = t->events[eventIdx];
                 item.relativeRow = row - minRow;
                 item.relativeTime = item.evt.time - minTime; // Relative to earliest event
                 clipboard.push_back(item);
             }
         }
    }
}

void TimelineView::CutSelection()
{
    CopySelection();
    DeleteSelection();
}

void TimelineView::SelectAll()
{
    if (!project) return;
    selection.clear();
    
    std::function<void(std::vector<Track>&)> traverse = [&](std::vector<Track>& tracks) {
        for(auto& t : tracks) {
            for (int i = 0; i < (int)t.events.size(); ++i) {
                selection.insert({&t, i});
            }
            traverse(t.children);
        }
    };
    traverse(project->tracks);
    Refresh();
}

void TimelineView::PasteAtPlayhead()
{
    if (clipboard.empty()) return;
    
    // Determine Anchor Track
    // 1. If we have a 'lastFocusedTrack', use its visual row.
    // 2. If one track is explicitly selected (via checking selection for single track events?), use it.
    // 3. Fallback to first visible.
    
    std::vector<Track*> visible = GetVisibleTracks();
    int anchorRow = 0;
    
    if (lastFocusedTrack)
    {
         for(int r=0; r<(int)visible.size(); ++r) if(visible[r] == lastFocusedTrack) { anchorRow = r; break; }
    }
    
    // Special Smart Logic: 
    // If clipboard is single-track (all relativeRow == 0), we paste strictly to the anchorRow track.
    // If clipboard is multi-track, we paste relative to anchorRow.
    
    std::vector<PasteEventsCommand::PasteItem> items;
    
    for (const auto& item : clipboard)
    {
        int targetRow = anchorRow + item.relativeRow;
        
        // Boundary check
        if (targetRow >= 0 && targetRow < (int)visible.size())
        {
            Track* target = GetEffectiveTargetTrack(visible[targetRow]);
            if (target)
            {
                Event newEvt = item.evt;
                newEvt.time = playheadPosition + item.relativeTime; // Paste at playhead
                
                items.push_back({target, newEvt});
            }
        }
    }
    
    if (!items.empty())
    {
        auto selectFn = [this](const std::vector<Track*>& tracks){
            // Improve: Select specific pasted events?
            // Command doesn't return the indices. 
            // We can search for them by time?
            // Implementation detail: PasteEventsCommand uses push_back.
            // So we can just scan for last N events? Unsafe if async.
            // For now, let's just clear selection to indicate "Something happened" or leave it.
            // Actually users prefer pasted items to be selected.
            
            selection.clear();
            // Heuristic selection
            for (auto* t : tracks) {
                if (!t->events.empty()) {
                    selection.insert({t, (int)t->events.size() - 1});
                }
            }
            Refresh();
        };
        
        auto refreshFn = [this](){ ValidateHitsounds(); Refresh(); };
        undoManager.PushCommand(std::make_unique<PasteEventsCommand>(items, selectFn, refreshFn));
    }
}

void TimelineView::DeleteSelection()
{
    if (selection.empty()) return;
    
    std::vector<RemoveEventsCommand::Item> items;
    
    // Gather Items
    for (auto& pair : selection)
    {
        Track* t = pair.first;
        int eventIdx = pair.second;
        if (eventIdx >= 0 && eventIdx < (int)t->events.size())
        {
            items.push_back({t, t->events[eventIdx]});
        }
    }
    
    if (!items.empty())
    {
        auto refreshFn = [this](){ selection.clear(); ValidateHitsounds(); Refresh(); };
        undoManager.PushCommand(std::make_unique<RemoveEventsCommand>(items, refreshFn));
    }
}

void TimelineView::OnMouseWheel(wxMouseEvent& evt)
{
    if (evt.ControlDown())
    {
        // Zoom
        wxPoint mousePos = evt.GetPosition(); // Client Coords
        
        // We want to keep the time at 'mousePos.x' constant.
        int x, y;
        GetViewStart(&x, &y);
        int ppuX, ppuY;
        GetScrollPixelsPerUnit(&ppuX, &ppuY);
        
        if (ppuX == 0) ppuX = 10; // Safety
        
        // Logical X is the pixel coordinate in the full virtual canvas
        int logicalX = x * ppuX + mousePos.x;
        double timeFocus = xToTime(logicalX);
        
        double zoomFactor = (evt.GetWheelRotation() > 0) ? 1.25 : 0.8;
        pixelsPerSecond *= zoomFactor;
        
        // Clamp Zoom
        if (pixelsPerSecond < 10.0) pixelsPerSecond = 10.0;
        if (pixelsPerSecond > 5000.0) pixelsPerSecond = 5000.0;
        
        UpdateVirtualSize();
        
        // Re-calculate scroll to keep timeFocus at mousePos.x
        int newLogicalX = timeToX(timeFocus);
        int newScrollPixel = newLogicalX - mousePos.x;
        if (newScrollPixel < 0) newScrollPixel = 0;
        
        Scroll(newScrollPixel / ppuX, -1);
        Refresh();
        
        // Notify zoom change
        if (OnZoomChanged) OnZoomChanged(pixelsPerSecond);
    }
    else if (evt.ShiftDown())
    {
        // Horizontal Scroll
        int rotation = evt.GetWheelRotation();
        int delta = evt.GetWheelDelta();
        if (delta == 0) delta = 120;
        
        int lines = rotation / delta;
        
        int x, y;
        GetViewStart(&x, &y);
        
        // Scroll 5 units per notch (reversed: wheel up = left usually? Or wheel down = right?)
        // Standard: Wheel Down (negative) -> Right (inc X)
        // Wheel Up (positive) -> Left (dec X)
        int direction = (rotation > 0) ? -1 : 1;
        
        int scrollAmt = std::abs(lines) * 5 * direction;
        
        Scroll(x + scrollAmt, -1);
    }
    else
    {
        // Vertical Scroll (Default)
        evt.Skip();
    }
}

void TimelineView::UpdateVirtualSize()
{
    // 1. Calculate Width (Max Time)
    double maxTime = 60.0; // Minimum 1 minute
    if (audioDuration > 0) maxTime = std::max(maxTime, audioDuration);
    
    if (project)
    {
        std::function<void(const std::vector<Track>&)> checkTracks = 
            [&](const std::vector<Track>& tracks) {
            for (const auto& t : tracks) {
                if (!t.events.empty()) {
                    // Assuming sorted events for now, or just check last?
                    // Safe to check all if small, but let's check back. 
                    // To be safe against unsorted:
                    for(const auto& e : t.events) {
                        if (e.time > maxTime) maxTime = e.time;
                    }
                }
                checkTracks(t.children);
            }
        };
        checkTracks(project->tracks);
    }
    
    // Add margin
    maxTime += 5.0;
    
    int w = timeToX(maxTime);
    
    // 2. Calculate Height (Visible Tracks)
    int h = headerHeight;
    std::vector<Track*> visible = GetVisibleTracks();
    for(Track* t : visible) {
        bool isChild = (t->name.find("%)") != std::string::npos); // Hacky check from before
        h += isChild ? 40 : 80;
    }
    h += 400; // Bottom Padding
    
    // Maintain current position if possible? 
    // SetScrollbars resets position if we aren't careful?
    // "The position is preserved if possible." - wxWidgets docs.
    
    int x, y;
    GetViewStart(&x, &y);
    
    SetScrollbars(10, 10, w / 10, h / 10, x, y);
}

void TimelineView::ValidateHitsounds()
{
    if (!project) return;

    struct EvtRef {
        Event* evt;
        Track* track;
    };
    std::map<int64_t, std::vector<EvtRef>> timeMap;

    std::function<void(std::vector<Track>&)> traverse = [&](std::vector<Track>& tracks) {
        for (auto& t : tracks) {
            for (auto& e : t.events) {
                e.isValid = true; // Reset
                int64_t ms = (int64_t)std::round(e.time * 1000.0);
                timeMap[ms].push_back({&e, &t});
            }
            traverse(t.children);
        }
    };
    traverse(project->tracks);

    // Validate
    for (auto& [time, evts] : timeMap) {
        std::vector<EvtRef> hitNormals;
        std::vector<EvtRef> additions;
        std::set<SampleSet> additionBanks;

        for (auto& ref : evts) {
            if (ref.track->sampleType == SampleType::HitNormal) {
                hitNormals.push_back(ref);
            } else if (ref.track->sampleType == SampleType::HitWhistle ||
                       ref.track->sampleType == SampleType::HitFinish ||
                       ref.track->sampleType == SampleType::HitClap) {
                additions.push_back(ref);
                additionBanks.insert(ref.track->sampleSet);
            }
        }

        // Rule 1: Max 1 HitNormal
        if (hitNormals.size() > 1) {
            for (auto& ref : hitNormals) ref.evt->isValid = false;
        }

        // Rule 2: Additions must share same bank
        if (additionBanks.size() > 1) {
            for (auto& ref : additions) ref.evt->isValid = false;
        }
    }
}
