#include "TimelineView.h"
#include "../model/Commands.h"
#include "../model/ProjectValidator.h"
#include "../Constants.h"
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

// Use constants from Constants.h via TrackLayout namespace

// -----------------------------------------------------------------------------
// Constructor and Basic Methods
// -----------------------------------------------------------------------------

TimelineView::TimelineView(wxWindow* parent)
    : wxScrolledWindow(parent, wxID_ANY)
    , project(nullptr)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxBLACK);
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

int TimelineView::timeToX(double time) const
{
    return static_cast<int>(time * pixelsPerSecond);
}

double TimelineView::xToTime(int x) const
{
    return static_cast<double>(x) / pixelsPerSecond;
}

// -----------------------------------------------------------------------------
// Painting Helpers
// -----------------------------------------------------------------------------

void TimelineView::DrawRuler(wxDC& dc, const wxSize& size, double visStart, double visEnd)
{
    wxRect rulerRect(0, 0, size.x, rulerHeight);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(wxColour(40, 40, 40)));
    dc.DrawRectangle(rulerRect);
    
    dc.SetPen(wxPen(wxColour(150, 150, 150)));
    dc.SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    dc.SetTextForeground(wxColour(200, 200, 200));
    
    double rulerStart = std::floor(visStart);
    for (double t = rulerStart; t <= visEnd; t += 1.0)
    {
        int rx = timeToX(t);
        dc.DrawLine(rx, rulerHeight - 10, rx, rulerHeight);
        dc.DrawText(wxString::Format("%.0fs", t), rx + 2, 2);
    }
}

void TimelineView::DrawMasterTrack(wxDC& dc, const wxSize& size, int viewStartPx, int viewEndPx)
{
    int masterY = rulerHeight;
    
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(wxColour(20, 20, 25)));
    dc.DrawRectangle(0, masterY, size.x, masterTrackHeight);
    
    // Draw Waveform
    if (!waveformPeaks.empty() && audioDuration > 0)
    {
        dc.SetPen(wxPen(wxColour(100, 150, 200), 1));
        
        int centerY = masterY + masterTrackHeight / 2;
        float heightScale = masterTrackHeight / 2.0f;
        
        double totalWidth = audioDuration * pixelsPerSecond;
        double samplesPerPixel = (double)waveformPeaks.size() / totalWidth;
        
        if (samplesPerPixel > 0)
        {
            int startX = std::max(0, viewStartPx);
            int endX = std::min((int)totalWidth, viewEndPx);
            
            for (int x = startX; x < endX; ++x)
            {
                int infoIndex = (int)(x * samplesPerPixel);
                if (infoIndex >= 0 && infoIndex < (int)waveformPeaks.size())
                {
                    float val = waveformPeaks[infoIndex];
                    int h = (int)(val * heightScale);
                    dc.DrawLine(x, centerY - h, x, centerY + h);
                }
            }
        }
    }
}

void TimelineView::DrawTrackBackgrounds(wxDC& dc, const wxSize& size, const std::vector<Track*>& visibleTracks)
{
    int y = headerHeight;
    
    for (Track* track : visibleTracks)
    {
        bool isChild = track->isChildTrack;
        int currentHeight = isChild ? TrackLayout::ChildTrackHeight : TrackLayout::ParentTrackHeight;
        bool isParentExpanded = !track->children.empty() && track->isExpanded;

        dc.SetPen(*wxTRANSPARENT_PEN);
        if (isParentExpanded)
            dc.SetBrush(wxBrush(wxColour(25, 25, 25), wxBRUSHSTYLE_BDIAGONAL_HATCH)); 
        else
            dc.SetBrush(wxBrush(wxColour(30, 30, 30)));
            
        dc.DrawRectangle(0, y, size.x, currentHeight);
        y += currentHeight;
    }
}

void TimelineView::DrawGrid(wxDC& dc, const wxSize& size, double visStart, double visEnd)
{
    // Horizontal grid lines
    std::vector<Track*> visible = GetVisibleTracks();
    int gy = headerHeight;
    dc.SetPen(wxPen(wxColour(60, 60, 60)));
    for (Track* track : visible)
    {
        int h = track->isChildTrack ? TrackLayout::ChildTrackHeight : TrackLayout::ParentTrackHeight;
        dc.DrawLine(0, gy + h, size.x, gy + h);
        gy += h;
    }
    
    double totalSeconds = size.x / pixelsPerSecond;
    if (totalSeconds < 1.0) totalSeconds = 10.0;
    
    // Vertical grid lines based on timing points
    if (project && !project->timingPoints.empty())
    {
        std::vector<const Project::TimingPoint*> sections;
        for (const auto& tp : project->timingPoints) {
            if (tp.uninherited) sections.push_back(&tp);
        }
        
        if (!sections.empty())
        {
            int startIndex = 0;
            for (int i = 0; i < (int)sections.size(); ++i) {
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
                    
                    double rel = t - sectionStart;
                    double beats = rel / beatLen;
                    double remainder = std::abs(beats - std::round(beats));
                    bool isMajor = (remainder < 0.001);
                    
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
}

void TimelineView::DrawTimingPoints(wxDC& dc, const wxSize& size, double totalSeconds)
{
    if (!project || project->timingPoints.empty()) return;
    
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

void TimelineView::DrawEvents(wxDC& dc, const std::vector<Track*>& visibleTracks, double visStart, double visEnd)
{
    int y = headerHeight;
    
    for (Track* track : visibleTracks)
    {
        bool isChild = track->isChildTrack;
        int currentHeight = isChild ? TrackLayout::ChildTrackHeight : TrackLayout::ParentTrackHeight;
        bool isParentExpanded = !track->children.empty() && track->isExpanded;
        
        if (!isParentExpanded)
        {
            auto drawEventList = [&](const std::vector<Event>& events, Track* srcTrack, wxColour color) {
                dc.SetPen(*wxTRANSPARENT_PEN);
                
                for (int i = 0; i < (int)events.size(); ++i)
                {
                    const auto& event = events[i];
                    if (event.time < visStart - 0.5 || event.time > visEnd + 0.5) continue;
                    int x = timeToX(event.time);
                    
                    // Check selection using track ID and event ID
                    bool isSelected = selection.count({srcTrack->id, event.id}) > 0;
                    
                    if (isSelected)
                    {
                        switch (event.validationState) {
                            case ValidationState::Invalid:
                                dc.SetBrush(wxBrush(wxColour(255, 165, 0)));
                                break;
                            case ValidationState::Warning:
                                dc.SetBrush(wxBrush(wxColour(255, 180, 200)));
                                break;
                            default:
                                dc.SetBrush(wxBrush(wxColour(255, 255, 0)));
                                break;
                        }
                    }
                    else
                    {
                        switch (event.validationState) {
                            case ValidationState::Invalid:
                                dc.SetBrush(wxBrush(wxColour(255, 50, 50)));
                                break;
                            case ValidationState::Warning:
                                dc.SetBrush(wxBrush(wxColour(255, 100, 180)));
                                break;
                            default:
                                dc.SetBrush(wxBrush(color));
                                break;
                        }
                    }
                    dc.DrawRectangle(x - 2, y + 2, 4, currentHeight - 4);
                    
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
                for (Track& child : track->children)
                {
                    drawEventList(child.events, &child, wxColour(100, 200, 255, 100));
                }
            }
            else
            {
                drawEventList(track->events, track, wxColour(100, 200, 255));
            }
        }
        
        y += currentHeight;
    }
}

void TimelineView::DrawLoopRegion(wxDC& dc, const wxSize& size)
{
    if (loopStart < 0 || loopEnd <= loopStart) return;
    
    int lx = timeToX(loopStart);
    int lw = timeToX(loopEnd) - lx;
    if (lw < 1) lw = 1;
    
    dc.SetBrush(wxBrush(wxColour(80, 180, 80)));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(lx, 0, lw, rulerHeight);
    
    dc.SetBrush(wxBrush(wxColour(0, 255, 0), wxBRUSHSTYLE_FDIAGONAL_HATCH));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(lx, rulerHeight, lw, size.y - rulerHeight);
}

void TimelineView::DrawDragGhosts(wxDC& dc, const std::vector<Track*>& visible)
{
    if (!isDragging || dragGhosts.empty()) return;
    
    int gy = headerHeight;
    
    for (Track* t : visible)
    {
        int currentHeight = t->isChildTrack ? TrackLayout::ChildTrackHeight : TrackLayout::ParentTrackHeight;
        
        for (const auto& ghost : dragGhosts)
        {
            // Resolve target track from ID
            uint64_t targetId = ghost.targetTrackId != 0 ? ghost.targetTrackId : ghost.originalTrackId;
            bool match = (t->id == targetId);
            if (!match && !t->isExpanded && !t->children.empty())
            {
                for (auto& c : t->children)
                    if (c.id == targetId) { match = true; break; }
            }
            
            if (match)
            {
                int x = timeToX(ghost.evt.time);
                
                dc.SetPen(wxPen(wxColour(255, 255, 255), 1, wxPENSTYLE_SHORT_DASH));
                switch (ghost.evt.validationState) {
                    case ValidationState::Invalid:
                        dc.SetBrush(wxBrush(wxColour(255, 100, 0, 180)));
                        break;
                    case ValidationState::Warning:
                        dc.SetBrush(wxBrush(wxColour(255, 150, 180, 180)));
                        break;
                    default:
                        dc.SetBrush(wxBrush(wxColour(255, 255, 0, 180)));
                        break;
                }
                    
                dc.DrawRectangle(x - 2, gy + 2, 4, currentHeight - 4);
            }
        }
        gy += currentHeight;
    }
}

void TimelineView::DrawMarquee(wxDC& dc)
{
    if (!isMarquee) return;
    
    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::CreateFromUnknownDC(dc));
    if (gc)
    {
        gc->SetPen(wxPen(wxColour(0, 120, 215), 1));
        gc->SetBrush(wxBrush(wxColour(0, 120, 215, 64)));
        gc->DrawRectangle(marqueeRect.x, marqueeRect.y, marqueeRect.width, marqueeRect.height);
    }
    else
    {
        dc.SetPen(wxPen(*wxWHITE, 1, wxPENSTYLE_DOT));
        dc.SetBrush(wxBrush(wxColour(255, 255, 255, 50))); 
        dc.DrawRectangle(marqueeRect);
    }
}

void TimelineView::DrawPlayhead(wxDC& dc, const wxSize& size)
{
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

void TimelineView::OnPaint(wxPaintEvent& evt)
{
    wxAutoBufferedPaintDC dc(this);
    PrepareDC(dc);
    dc.Clear();
    
    if (!project) return;
    
    wxSize size = GetVirtualSize();
    wxSize clientSize = GetClientSize();
    
    int scrollX, scrollY;
    GetViewStart(&scrollX, &scrollY);
    int ppuX, ppuY;
    GetScrollPixelsPerUnit(&ppuX, &ppuY);
    if (ppuX == 0) ppuX = 10;
    
    int viewStartPx = scrollX * ppuX;
    int viewEndPx = viewStartPx + clientSize.x;
    
    double visStart = xToTime(viewStartPx);
    double visEnd = xToTime(viewEndPx);
    double totalSeconds = size.x / pixelsPerSecond;
    if (totalSeconds < 1.0) totalSeconds = 10.0;
    
    std::vector<Track*> visibleTracks = GetVisibleTracks();
    
    // Draw in order
    DrawRuler(dc, size, visStart, visEnd);
    DrawMasterTrack(dc, size, viewStartPx, viewEndPx);
    DrawTrackBackgrounds(dc, size, visibleTracks);
    DrawGrid(dc, size, visStart, visEnd);
    DrawTimingPoints(dc, size, totalSeconds);
    DrawEvents(dc, visibleTracks, visStart, visEnd);
    DrawLoopRegion(dc, size);
    DrawDragGhosts(dc, visibleTracks);
    DrawMarquee(dc);
    DrawPlayhead(dc, size);
}

// -----------------------------------------------------------------------------
// Event Placement Helper
// -----------------------------------------------------------------------------

void TimelineView::PlaceEvent(Track* target, double time)
{
    Event newEvent;
    newEvent.time = time;
    
    auto refreshFn = [this](){ ValidateHitsounds(); Refresh(); };
    
    bool isAddition = (target->sampleType == SampleType::HitWhistle ||
                       target->sampleType == SampleType::HitFinish ||
                       target->sampleType == SampleType::HitClap);
    
    if (isAddition && defaultHitnormalBank.has_value()) {
        Track* hitnormalTrack = FindOrCreateHitnormalTrack(
            defaultHitnormalBank.value(),
            target->gain
        );
        
        if (hitnormalTrack) {
            // Check if ANY hitnormal already exists at this timestamp
            bool hitnormalExists = false;
            for (auto& track : project->tracks) {
                if (track.sampleType != SampleType::HitNormal) continue;
                
                for (const auto& evt : track.events) {
                    if (std::abs(evt.time - time) < 0.001) {
                        hitnormalExists = true;
                        break;
                    }
                }
                if (hitnormalExists) break;
                
                for (auto& child : track.children) {
                    for (const auto& evt : child.events) {
                        if (std::abs(evt.time - time) < 0.001) {
                            hitnormalExists = true;
                            break;
                        }
                    }
                    if (hitnormalExists) break;
                }
                if (hitnormalExists) break;
            }
            
            if (hitnormalExists) {
                controller.GetUndoManager().PushCommand(std::make_unique<AddEventCommand>(target, newEvent, refreshFn));
            } else {
                Event hnEvent;
                hnEvent.time = time;
                
                std::vector<AddMultipleEventsCommand::Item> items;
                items.push_back({hitnormalTrack, hnEvent});
                items.push_back({target, newEvent});
                
                controller.GetUndoManager().PushCommand(std::make_unique<AddMultipleEventsCommand>(items, refreshFn));
            }
            return;
        }
    }
    
    controller.GetUndoManager().PushCommand(std::make_unique<AddEventCommand>(target, newEvent, refreshFn));
}

// -----------------------------------------------------------------------------
// Mouse Event Helpers
// -----------------------------------------------------------------------------

void TimelineView::HandleLeftDown(wxMouseEvent& evt, const wxPoint& pos)
{
    SetFocus();
    
    // Check Header (Ruler vs Master)
    if (pos.y < rulerHeight)
    {
        int px = timeToX(playheadPosition);
        if (std::abs(pos.x - px) <= 8) {
            isDraggingPlayhead = true;
            return;
        }
        
        double t = SnapToGrid(xToTime(pos.x));
        loopStart = t;
        loopEnd = t;
        isDraggingLoop = true;
        Refresh();
        return;
    }
    else if (pos.y >= rulerHeight && pos.y < headerHeight)
    {
        double t = SnapToGrid(xToTime(pos.x));
        SetPlayheadPosition(t); 
        isDraggingPlayhead = true;
        return;
    }
    
    HitResult hit = GetEventAt(pos);
    Track* hitTrack = GetTrackAtY(pos.y);
    if (hitTrack) controller.SetLastFocusedTrack(hitTrack->id);
    
    bool ctrl = evt.ControlDown();
    
    if (hit.isValid())
    {
        // Create ID-based selection key
        std::pair<uint64_t, uint64_t> selId = {hit.logicalTrack->id, hit.logicalTrack->events[hit.eventIndex].id};
        
        if (!ctrl && selection.find(selId) == selection.end())
        {
            selection.clear();
            selection.insert(selId);
        }
        else if (ctrl)
        {
            if (selection.count(selId)) selection.erase(selId);
            else selection.insert(selId);
        }
        
        if (!selection.empty())
        {
            isDragging = true;
            dragStartPos = pos;
            dragStartTrackId = hit.visualTrack ? hit.visualTrack->id : 0;
            dragStartTime = SnapToGrid(xToTime(pos.x));
            
            dragGhosts.clear();
            
            // Group selected events by track ID, find events by their IDs
            std::map<uint64_t, std::vector<uint64_t>> toRemoveByTrack;
            for (auto& sel : selection) {
                toRemoveByTrack[sel.first].push_back(sel.second);
            }
            
            std::vector<Track*> visible = GetVisibleTracks();
            
            auto findRowIndex = [&visible](Track* target) -> int {
                for (int r = 0; r < (int)visible.size(); ++r) {
                    if (visible[r] == target) return r;
                    if (!visible[r]->isExpanded) {
                        for (const auto& child : visible[r]->children) {
                            if (&child == target) return r;
                        }
                    }
                }
                return -1;
            };
            
            for (auto& pair : toRemoveByTrack)
            {
                Track* t = FindTrackById(pair.first);
                if (!t) continue;
                
                int rowIndex = findRowIndex(t);
                if (rowIndex == -1) rowIndex = 0;
                
                // Find event indices from IDs and sort in reverse
                std::vector<int> indices;
                for (uint64_t eventId : pair.second) {
                    for (int i = 0; i < (int)t->events.size(); ++i) {
                        if (t->events[i].id == eventId) {
                            indices.push_back(i);
                            break;
                        }
                    }
                }
                std::sort(indices.rbegin(), indices.rend());

                for (int idx : indices)
                {
                    DragGhost g;
                    g.evt = t->events[idx];
                    g.originalTime = g.evt.time;
                    g.originalTrackId = t->id;
                    g.originalRowIndex = rowIndex;
                    g.targetTrackId = t->id;
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
        if (currentTool == ToolType::Draw && hitTrack)
        {
            Track* target = GetEffectiveTargetTrack(hitTrack);
            if (!target) target = hitTrack;

            double t = SnapToGrid(xToTime(pos.x));
            PlaceEvent(target, t);
            return;
        }
        
        if (!ctrl) selection.clear();
        
        isMarquee = true;
        marqueeStartPos = pos;
        marqueeRect = wxRect(pos, pos);
        
        if (ctrl) baseSelection = selection;
        else baseSelection.clear();
        
        Refresh();
    }
}

void TimelineView::HandleDragging(wxMouseEvent& evt, const wxPoint& pos)
{
    if (isDraggingPlayhead)
    {
        double t = SnapToGrid(xToTime(pos.x));
        SetPlayheadPosition(t);
        if (OnPlayheadScrubbed) OnPlayheadScrubbed(t);
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
        Track* startTrack = FindTrackById(dragStartTrackId);
        
        int startRow = -1;
        int curRow = -1;
        std::vector<Track*> visible = GetVisibleTracks();
        
        for (int i = 0; i < (int)visible.size(); ++i) {
            if (visible[i] == startTrack) startRow = i;
            if (visible[i] == curTrack) curRow = i;
        }
        
        int rowDelta = 0;
        if (startRow != -1 && curRow != -1) rowDelta = curRow - startRow;
        
        for (auto& g : dragGhosts)
        {
            g.evt.time = std::max(0.0, g.originalTime + timeDelta);
            
            int targetRowIndex = g.originalRowIndex + rowDelta;
            
            Track* visualTarget = nullptr;
            if (targetRowIndex >= 0 && targetRowIndex < (int)visible.size())
            {
                visualTarget = visible[targetRowIndex];
            }
            else
            {
                if (targetRowIndex < 0) targetRowIndex = 0;
                if (targetRowIndex >= (int)visible.size()) targetRowIndex = visible.size() - 1;
                visualTarget = visible[targetRowIndex];
            }
            
            Track* effectiveTarget = GetEffectiveTargetTrack(visualTarget);
            g.targetTrackId = effectiveTarget ? effectiveTarget->id : 0;
        }
        Refresh();
    }
    else if (isMarquee)
    {
        int x = std::min(marqueeStartPos.x, pos.x);
        int y = std::min(marqueeStartPos.y, pos.y);
        int w = std::abs(pos.x - marqueeStartPos.x);
        int h = std::abs(pos.y - marqueeStartPos.y);
        marqueeRect = wxRect(x, y, w, h);
        
        PerformMarqueeSelect(marqueeRect);
        Refresh();
    }
}

void TimelineView::HandleLeftUp(wxMouseEvent& evt, const wxPoint& pos)
{
    if (isDraggingPlayhead)
    {
        if (OnPlayheadScrubbed) OnPlayheadScrubbed(playheadPosition);
        isDraggingPlayhead = false;
    }
    else if (isDraggingLoop)
    {
        if (loopEnd < loopStart) std::swap(loopStart, loopEnd);
        
        if ((loopEnd - loopStart) < 0.05)
        {
            loopStart = -1.0;
            loopEnd = -1.0;
        }
        
        isDraggingLoop = false;
        Refresh();
        if (OnLoopPointsChanged) OnLoopPointsChanged(loopStart, loopEnd);
    }
    else if (isDragging)
    {
        selection.clear();
        
        // Restore originals temporarily using ID lookup
        for (const auto& g : dragGhosts) {
            Track* origTrack = FindTrackById(g.originalTrackId);
            if (!origTrack) continue;
            
            Event origEvt = g.evt;
            origEvt.time = g.originalTime; 
            origTrack->events.push_back(origEvt);
        }
        
        // Build Move Command
        std::vector<MoveEventsCommand::MoveInfo> moves;
        for (auto& g : dragGhosts)
        {
            Track* origTrack = FindTrackById(g.originalTrackId);
            Track* target = FindTrackById(g.targetTrackId != 0 ? g.targetTrackId : g.originalTrackId);
            if (!origTrack) continue;
            if (!target) target = origTrack;
            
            Event newEvt = g.evt;
            Event origEvt = g.evt;
            origEvt.time = g.originalTime;
            
            moves.push_back({origTrack, origEvt, target, newEvt});
        }
        
        auto refreshFn = [this](){ ValidateHitsounds(); Refresh(); };
        controller.GetUndoManager().PushCommand(std::make_unique<MoveEventsCommand>(moves, refreshFn));
        
        // Re-select moved events using IDs
        selection.clear();
        for (auto& m : moves) {
            // Find the event by matching the event ID
            for (const auto& evt : m.newTrack->events) {
                if (evt.id == m.newEvent.id) {
                    selection.insert({m.newTrack->id, evt.id});
                    break;
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

void TimelineView::HandleRightDown(const wxPoint& pos)
{
    HitResult hit = GetEventAt(pos);
    if (hit.isValid())
    {
        Track* t = hit.logicalTrack;
        int idx = hit.eventIndex;
        
        if (idx >= 0 && idx < (int)t->events.size()) {
            std::vector<RemoveEventsCommand::Item> items;
            items.push_back({t, t->events[idx]});
            
            auto refreshFn = [this](){ selection.clear(); ValidateHitsounds(); Refresh(); };
            controller.GetUndoManager().PushCommand(std::make_unique<RemoveEventsCommand>(items, refreshFn));
        }
    }
}

void TimelineView::OnMouseEvents(wxMouseEvent& evt)
{
    if (!project) return;
    
    wxPoint pos = evt.GetPosition();
    CalcUnscrolledPosition(pos.x, pos.y, &pos.x, &pos.y);
    
    if (evt.LeftDown())
    {
        HandleLeftDown(evt, pos);
    }
    else if (evt.Dragging())
    {
        HandleDragging(evt, pos);
    }
    else if (evt.RightDown())
    {
        HandleRightDown(pos);
    }
    else if (evt.LeftUp())
    {
        HandleLeftUp(evt, pos);
    }
    evt.Skip();
}

// -----------------------------------------------------------------------------
// Helper Functions (unchanged from original)
// -----------------------------------------------------------------------------

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
        int h = t->isChildTrack ? TrackLayout::ChildTrackHeight : TrackLayout::ParentTrackHeight;
        
        if (y >= currentY && y < currentY + h)
            return t;
        currentY += h;
    }
    return nullptr;
}

Track* TimelineView::GetEffectiveTargetTrack(Track* t)
{
    if (!t) return nullptr;
    if (!t->children.empty())
    {
        if (t->primaryChildIndex >= 0 && t->primaryChildIndex < (int)t->children.size())
            return &t->children[t->primaryChildIndex];
        return &t->children[0];
    }
    return t;
}

const Project::TimingPoint* TimelineView::GetTimingPointAt(double time)
{
    if (!project || project->timingPoints.empty()) return nullptr;
    
    const Project::TimingPoint* best = nullptr;
    
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
            break; 
        }
    }
    
    return best;
}

void TimelineView::PerformMarqueeSelect(const wxRect& rect)
{
    selection = baseSelection;
    
    std::vector<Track*> visible = GetVisibleTracks();
    int y = headerHeight;
    
    for (Track* t : visible)
    {
        int currentHeight = t->isChildTrack ? TrackLayout::ChildTrackHeight : TrackLayout::ParentTrackHeight;
        
        if (y + currentHeight < rect.GetTop() || y > rect.GetBottom())
        {
            y += currentHeight;
            continue;
        }
        
        if (!t->isExpanded && !t->children.empty())
        {
            for (int i = 0; i < (int)t->events.size(); ++i)
            {
                int ex = timeToX(t->events[i].time);
                wxRect eventRect(ex - 2, y + 2, 4, currentHeight - 4); 
                if (rect.Intersects(eventRect)) selection.insert({t->id, t->events[i].id});
            }

            for (Track& child : t->children)
            {
                for (int i = 0; i < (int)child.events.size(); ++i)
                {
                    int ex = timeToX(child.events[i].time);
                    wxRect eventRect(ex - 2, y + 2, 4, currentHeight - 4);
                    if (rect.Intersects(eventRect)) selection.insert({child.id, child.events[i].id});
                }
            }
        }
        else
        {
            for (int i = 0; i < (int)t->events.size(); ++i)
            {
                int ex = timeToX(t->events[i].time);
                wxRect eventRect(ex - 2, y + 2, 4, currentHeight - 4); 
                if (rect.Intersects(eventRect)) selection.insert({t->id, t->events[i].id});
            }
        }
        y += currentHeight;
    }
}

double TimelineView::SnapToGrid(double time)
{
    if (!project) return time;
    
    const Project::TimingPoint* tp = GetTimingPointAt(time);
    
    double beatLen = 0.5;
    double startOffset = 0.0;
    
    if (tp)
    {
        startOffset = tp->time / 1000.0;
        beatLen = tp->beatLength / 1000.0;
    }
    else if (!project->timingPoints.empty())
    {
        for (const auto& p : project->timingPoints) {
            if (p.uninherited) {
                startOffset = p.time / 1000.0;
                beatLen = p.beatLength / 1000.0;
                break;
            }
        }
    }
    
    if (beatLen <= 0.001) beatLen = 0.5;
    
    double snapStep = beatLen / gridDivisor;
    double n = std::round((time - startOffset) / snapStep);
    double snapped = startOffset + n * snapStep;
    return std::max(0.0, snapped);
}

// -----------------------------------------------------------------------------
// Keyboard and Clipboard Operations (unchanged from original)
// -----------------------------------------------------------------------------

void TimelineView::OnKeyDown(wxKeyEvent& evt)
{
    if (evt.GetKeyCode() == WXK_DELETE || evt.GetKeyCode() == WXK_BACK)
    {
        DeleteSelection();
        return;
    }
    
    if (evt.ControlDown())
    {
        switch (evt.GetKeyCode())
        {
        case 'C':
            CopySelection();
            return;
        case 'X':
            CutSelection();
            return;
        case 'V':
            PasteAtPlayhead();
            return;
        case 'A':
            SelectAll();
            return;
        case 'Z':
            if (evt.ShiftDown())
                Redo();
            else
                Undo();
            return;
        case 'Y':
            Redo();
            return;
        }
    }
    
    evt.Skip();
}

void TimelineView::CopySelection()
{
    if (selection.empty()) return;
    
    clipboard.clear();
    
    std::vector<Track*> visible = GetVisibleTracks();
    
    auto findRowIndex = [&visible](Track* t) -> int {
        for (int i = 0; i < (int)visible.size(); ++i) {
            if (visible[i] == t) return i;
            if (!visible[i]->isExpanded) {
                for (const auto& c : visible[i]->children) {
                    if (&c == t) return i;
                }
            }
        }
        return 0;
    };
    
    double minTime = std::numeric_limits<double>::max();
    int minRow = std::numeric_limits<int>::max();
    
    // First pass: find min time and row
    for (const auto& sel : selection)
    {
        Track* track = FindTrackById(sel.first);
        if (!track) continue;
        
        // Find event by ID
        for (const auto& evt : track->events) {
            if (evt.id == sel.second) {
                int row = findRowIndex(track);
                if (evt.time < minTime) minTime = evt.time;
                if (row < minRow) minRow = row;
                break;
            }
        }
    }
    
    // Second pass: build clipboard items
    for (const auto& sel : selection)
    {
        Track* track = FindTrackById(sel.first);
        if (!track) continue;
        
        for (const auto& evt : track->events) {
            if (evt.id == sel.second) {
                int row = findRowIndex(track);
                
                ClipboardItem item;
                item.evt = evt;
                item.relativeRow = row - minRow;
                item.relativeTime = evt.time - minTime;
                clipboard.push_back(item);
                break;
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
    selection.clear();
    std::vector<Track*> visible = GetVisibleTracks();
    
    for (Track* t : visible)
    {
        if (!t->isExpanded && !t->children.empty())
        {
            for (Track& child : t->children)
            {
                for (const auto& evt : child.events)
                    selection.insert({child.id, evt.id});
            }
        }
        else
        {
            for (const auto& evt : t->events)
                selection.insert({t->id, evt.id});
        }
    }
    Refresh();
}

void TimelineView::PasteAtPlayhead()
{
    if (clipboard.empty()) return;
    
    std::vector<Track*> visible = GetVisibleTracks();
    
    int anchorRow = 0;
    Track* lastFocused = FindTrackById(controller.GetLastFocusedTrackId());
    if (lastFocused)
    {
        for (int i = 0; i < (int)visible.size(); ++i)
        {
            if (visible[i] == lastFocused ||
                (!visible[i]->isExpanded && std::find_if(visible[i]->children.begin(), visible[i]->children.end(),
                    [lastFocused](const Track& c){ return &c == lastFocused; }) != visible[i]->children.end()))
            {
                anchorRow = i;
                break;
            }
        }
    }
    
    std::vector<PasteEventsCommand::PasteItem> itemsToPaste;
    
    for (const auto& ci : clipboard)
    {
        int targetRow = anchorRow + ci.relativeRow;
        if (targetRow < 0) targetRow = 0;
        if (targetRow >= (int)visible.size()) targetRow = (int)visible.size() - 1;
        
        Track* visualTarget = visible[targetRow];
        Track* actualTarget = GetEffectiveTargetTrack(visualTarget);
        if (!actualTarget) continue;
        
        Event newEvt = ci.evt;
        newEvt.time = SnapToGrid(playheadPosition + ci.relativeTime);
        
        itemsToPaste.push_back({actualTarget, newEvt});
    }
    
    if (!itemsToPaste.empty())
    {
        auto selCallback = [this](const std::vector<Track*>&) {
            // Optionally re-select pasted items
        };
        auto refreshFn = [this](){ ValidateHitsounds(); Refresh(); };
        controller.GetUndoManager().PushCommand(std::make_unique<PasteEventsCommand>(itemsToPaste, selCallback, refreshFn));
    }
}

void TimelineView::DeleteSelection()
{
    if (selection.empty()) return;
    
    std::vector<RemoveEventsCommand::Item> items;
    for (const auto& sel : selection)
    {
        Track* track = FindTrackById(sel.first);
        if (!track) continue;
        
        for (const auto& evt : track->events) {
            if (evt.id == sel.second) {
                items.push_back({track, evt});
                break;
            }
        }
    }
    
    auto refreshFn = [this](){ selection.clear(); ValidateHitsounds(); Refresh(); };
    controller.GetUndoManager().PushCommand(std::make_unique<RemoveEventsCommand>(items, refreshFn));
}

// -----------------------------------------------------------------------------
// Zoom and Virtual Size
// -----------------------------------------------------------------------------

void TimelineView::OnMouseWheel(wxMouseEvent& evt)
{
    if (evt.ControlDown())
    {
        int rotation = evt.GetWheelRotation();
        double zoomFactor = (rotation > 0) ? 1.1 : 0.9;
        
        double oldPPS = pixelsPerSecond;
        pixelsPerSecond *= zoomFactor;
        
        if (pixelsPerSecond < ZoomSettings::MinPixelsPerSecond) 
            pixelsPerSecond = ZoomSettings::MinPixelsPerSecond;
        if (pixelsPerSecond > ZoomSettings::MaxPixelsPerSecond) 
            pixelsPerSecond = ZoomSettings::MaxPixelsPerSecond;
        
        if (pixelsPerSecond != oldPPS)
        {
            int mouseX = evt.GetX();
            int scrollX, scrollY;
            GetViewStart(&scrollX, &scrollY);
            int ppuX, ppuY;
            GetScrollPixelsPerUnit(&ppuX, &ppuY);
            if (ppuX == 0) ppuX = 10;
            
            int oldPxPos = scrollX * ppuX + mouseX;
            double timeAtMouse = oldPxPos / oldPPS;
            
            UpdateVirtualSize();
            
            int newPxPos = (int)(timeAtMouse * pixelsPerSecond);
            int newScrollX = (newPxPos - mouseX) / ppuX;
            if (newScrollX < 0) newScrollX = 0;
            
            Scroll(newScrollX, scrollY);
            
            if (OnZoomChanged)
                OnZoomChanged(pixelsPerSecond);
        }
        Refresh();
    }
    else if (evt.ShiftDown())
    {
        int rotation = evt.GetWheelRotation();
        int scrollX, scrollY;
        GetViewStart(&scrollX, &scrollY);
        int ppuX, ppuY;
        GetScrollPixelsPerUnit(&ppuX, &ppuY);
        if (ppuX == 0) ppuX = 10;
        
        int delta = (rotation > 0) ? -5 : 5;
        Scroll(scrollX + delta, scrollY);
    }
    else
    {
        evt.Skip();
    }
}

void TimelineView::UpdateVirtualSize()
{
    int width = 2000;
    int height = headerHeight;
    
    if (audioDuration > 0)
    {
        width = (int)(audioDuration * pixelsPerSecond) + 200;
    }
    
    if (project)
    {
        std::function<void(const std::vector<Track>&)> calcHeight = [&](const std::vector<Track>& tracks) {
            for (const Track& t : tracks)
            {
                height += t.isChildTrack ? TrackLayout::ChildTrackHeight : TrackLayout::ParentTrackHeight;
                if (t.isExpanded)
                {
                    calcHeight(t.children);
                }
            }
        };
        calcHeight(project->tracks);
    }
    
    height += 100;
    
    SetVirtualSize(width, height);
    SetScrollRate(10, 10);
}

void TimelineView::ValidateHitsounds()
{
    if (!project) return;
    
    // Use ProjectValidator to validate hitsounds
    ProjectValidator::Validate(*project);
    
    if (OnTracksModified) OnTracksModified();
}

Track* TimelineView::FindTrackById(uint64_t id)
{
    if (!project || id == 0) return nullptr;
    
    std::function<Track*(std::vector<Track>&)> search = [&](std::vector<Track>& tracks) -> Track* {
        for (auto& t : tracks) {
            if (t.id == id) return &t;
            Track* found = search(t.children);
            if (found) return found;
        }
        return nullptr;
    };
    
    return search(project->tracks);
}

Track* TimelineView::FindOrCreateHitnormalTrack(SampleSet bank, double volume)
{
    if (!project) return nullptr;
    
    std::string setStr = (bank == SampleSet::Normal) ? "normal" : 
                        (bank == SampleSet::Soft) ? "soft" : "drum";
    std::string parentName = setStr + "-hitnormal";
    
    int volumeInt = static_cast<int>(volume * 100);
    std::string childName = parentName + " (" + std::to_string(volumeInt) + "%)";
    
    Track* parent = nullptr;
    for (auto& t : project->tracks) {
        if (t.name == parentName) {
            parent = &t;
            break;
        }
    }
    
    if (!parent) {
        Track newParent;
        newParent.name = parentName;
        newParent.sampleSet = bank;
        newParent.sampleType = SampleType::HitNormal;
        newParent.isExpanded = false;
        project->tracks.push_back(newParent);
        parent = &project->tracks.back();
    }
    
    for (auto& c : parent->children) {
        if (c.name == childName) {
            return &c;
        }
    }
    
    Track child;
    child.name = childName;
    child.sampleSet = bank;
    child.sampleType = SampleType::HitNormal;
    child.gain = volume;
    child.isChildTrack = true;
    parent->children.push_back(child);
    
    UpdateVirtualSize();
    Refresh();
    
    return &parent->children.back();
}
