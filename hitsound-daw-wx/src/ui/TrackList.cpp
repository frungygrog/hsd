#include "TrackList.h"
#include "../model/Project.h"
#include "AddTrackDialog.h"
// Include TimelineView header for full definition
#include "TimelineView.h" 
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <cmath>
#include <algorithm>
#include <functional>

wxBEGIN_EVENT_TABLE(TrackList, wxPanel)
    EVT_PAINT(TrackList::OnPaint)
    EVT_MOUSE_EVENTS(TrackList::OnMouseEvents)
wxEND_EVENT_TABLE()

TrackList::TrackList(wxWindow* parent)
    : wxPanel(parent)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(wxColour(40, 40, 40));
}

void TrackList::SetProject(Project* p)
{
    project = p;
    Refresh();
}

void TrackList::SetVerticalScrollOffset(int y)
{
    scrollOffsetY = y;
    Refresh();
}

int TrackList::GetTotalContentHeight() const
{
    if (!project) return 200;
    
    int height = 130; // Header (30 Ruler + 100 Master)
    
    std::function<void(const Track&)> calcHeight = [&](const Track& t) {
        height += (t.name.find("%)") != std::string::npos) ? 40 : 80;
        if (t.isExpanded && !t.children.empty()) {
            for (const auto& c : t.children) calcHeight(c);
        }
    };
    
    for (const auto& t : project->tracks) calcHeight(t);
    
    return height + 50; // Plus Add Track button space
}

std::vector<Track*> TrackList::GetVisibleTracks()
{
    std::vector<Track*> visible;
    if (project)
    {
        std::function<void(Track&)> traverse = [&](Track& t) {
            visible.push_back(&t);
            if (t.isExpanded)
            {
                for (auto& c : t.children) traverse(c);
            }
        };
        for (auto& t : project->tracks) traverse(t);
    }
    return visible;
}

int TrackList::GetTrackHeight(const Track& track)
{
    if (track.name.find("%)") != std::string::npos) return 40;
    return 80;
}

void TrackList::OnPaint(wxPaintEvent& evt)
{
    wxAutoBufferedPaintDC dc(this);
    
    // Fill the entire client area with background color first
    wxSize clientSize = GetClientSize();
    dc.SetBrush(wxBrush(wxColour(160, 160, 160)));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(0, 0, clientSize.GetWidth(), clientSize.GetHeight());
    
    if (!project) return;
    
    // Apply scroll offset - shift all drawing up by scrollOffsetY
    dc.SetDeviceOrigin(0, -scrollOffsetY);
    
    int y = 0;
    int width = clientSize.GetWidth();
    
    // Colors
    wxColour borderCol(100, 100, 100);
    wxColour panelCol(192, 192, 192); 
    
    wxFont font = dc.GetFont();
    font.SetWeight(wxFONTWEIGHT_BOLD);
    dc.SetFont(font);
    
    // -------------------------------------------------------------------------
    // Header for Master Track + Ruler
    // -------------------------------------------------------------------------
    int rulerHeight = 30;
    int masterTrackHeight = 100;
    int headerHeight = rulerHeight + masterTrackHeight;
    
    // Draw Ruler (Blank for now in TrackList)
    wxRect rulerRect(0, y, width, rulerHeight);
    dc.SetBrush(wxBrush(wxColour(50, 50, 50)));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(rulerRect);
    
    // Master Track
    int masterY = y + rulerHeight;
    wxRect masterRect(0, masterY, width, masterTrackHeight);
    
    dc.SetPen(wxPen(borderCol));
    dc.SetBrush(wxBrush(wxColour(30, 30, 35)));
    dc.DrawRectangle(masterRect);
    
    dc.SetTextForeground(*wxWHITE);
    dc.DrawText("Master Audio", 10, masterY + 40);
    
    y += headerHeight;
    
    // Indentation Helper
    std::function<void(Track&, int)> drawTrack = [&](Track& track, int indent) {
        
        int currentHeight = (indent == 0) ? 80 : 40; 
        
        // Track Panel
        wxRect trackRect(0, y, width, currentHeight);
        
        dc.SetPen(wxPen(borderCol));
        dc.SetBrush(wxBrush(panelCol));
        dc.DrawRectangle(trackRect);
        
        // Highlight Source Track if Dragging
        if (isDraggingTrack && &track == dragSourceTrack)
        {
            std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::CreateFromUnknownDC(dc));
            if (gc)
            {
                gc->SetBrush(wxBrush(wxColour(0, 120, 255, 80))); // Low opacity blue
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->DrawRectangle(trackRect.x, trackRect.y, trackRect.width, trackRect.height);
            }
        }
        
        // Title Rect
        int titleH = (indent == 0) ? 20 : 15;
        wxRect titleRect(trackRect.GetX(), trackRect.GetY(), width, titleH);
        dc.SetBrush(wxBrush(wxColour(220, 220, 220))); 
        dc.DrawRectangle(titleRect);
        
        // Expander Button
        if (!track.children.empty())
        {
            wxRect expanderRect(indent * 10 + 2, y + 2, 16, 16);
            dc.SetBrush(*wxWHITE_BRUSH);
            dc.DrawRectangle(expanderRect);
            dc.SetBrush(*wxBLACK_BRUSH);
            
            // Draw (+) or (-)
            dc.DrawLine(expanderRect.GetX() + 3, expanderRect.GetY() + 8, expanderRect.GetRight() - 3, expanderRect.GetY() + 8);
            if (!track.isExpanded)
                dc.DrawLine(expanderRect.GetX() + 8, expanderRect.GetY() + 3, expanderRect.GetX() + 8, expanderRect.GetBottom() - 3);
        }

        // Name
        dc.SetTextForeground(*wxBLACK);
        if (indent > 0) 
        {
             wxFont f = dc.GetFont();
             f.SetPointSize(8);
             dc.SetFont(f);
        }
        else
        {
             wxFont f = dc.GetFont();
             f.SetPointSize(10);
             f.SetWeight(wxFONTWEIGHT_BOLD);
             dc.SetFont(f);
        }
        
        dc.DrawText(track.name, indent * 10 + 25, y + 2);
        
        // Controls
        int btnWidth = 40;
        int btnHeight = (indent == 0) ? 18 : 14;
        int btnY = y + ((indent == 0) ? 25 : 20); 
        
        wxRect muteRect(5, btnY, btnWidth, btnHeight);
        wxRect soloRect(5 + btnWidth + 5, btnY, btnWidth, btnHeight);
        
        // Mute/Solo - Only for Parents
        if (indent == 0)
        {
            // Mute
            dc.SetBrush(track.mute ? wxBrush(wxColour(100, 100, 200)) : wxBrush(wxColour(200, 200, 200)));
            dc.DrawRoundedRectangle(muteRect, 2);
            dc.DrawText("Mute", muteRect.GetX() + 5, muteRect.GetY() + (indent==0?0:-2));
            
            // Solo
            dc.SetBrush(track.solo ? wxBrush(wxColour(100, 200, 100)) : wxBrush(wxColour(200, 200, 200)));
            dc.DrawRoundedRectangle(soloRect, 2);
            dc.DrawText("Solo", soloRect.GetX() + 5, soloRect.GetY() + (indent==0?0:-2));
        }
        
        // Playback Controls (Volume Slider) for Child Tracks
        if (indent > 0)
        {
             // Draw Slider
             int sliderX = soloRect.GetRight() + 10;
             int sliderW = 100;
             int sliderH = 14;
             wxRect sliderRect(sliderX, btnY, sliderW, sliderH);
             
             // Track Background
             dc.SetBrush(wxBrush(wxColour(50, 50, 50)));
             dc.SetPen(*wxTRANSPARENT_PEN);
             dc.DrawRoundedRectangle(sliderRect, 2);
             
             // Fill based on gain (0.0 - 1.0)
             int fillW = (int)(track.gain * sliderW);
             if (fillW > sliderW) fillW = sliderW;
             if (fillW < 0) fillW = 0;
             
             wxRect fillRect(sliderX, btnY, fillW, sliderH);
             dc.SetBrush(wxBrush(wxColour(100, 200, 255)));
             dc.DrawRoundedRectangle(fillRect, 2);
             
             // Text Value
             wxString volStr = wxString::Format("%d%%", (int)(track.gain * 100));
             dc.SetTextForeground(*wxWHITE);
             wxFont vFont = dc.GetFont();
             vFont.SetPointSize(8);
             vFont.SetWeight(wxFONTWEIGHT_NORMAL);
             dc.SetFont(vFont);
             dc.DrawText(volStr, sliderX + sliderW + 5, btnY);
        }
        
        // Primary Track Selector (Only for parents)
        if (!track.children.empty())
        {
            int primaryX = soloRect.GetRight() + 10;
            wxRect primaryRect(primaryX, btnY, 120, btnHeight);
            
            dc.SetBrush(wxBrush(wxColour(200, 200, 220)));
            dc.DrawRoundedRectangle(primaryRect, 2);
            
            wxString primaryName = "Primary: <None>";
            if (track.primaryChildIndex >= 0 && track.primaryChildIndex < (int)track.children.size())
            {
                primaryName = "Primary: " + track.children[track.primaryChildIndex].name;
            }
            
            // Clip name
             wxString clippedName = primaryName;
             if (clippedName.Length() > 15) clippedName = clippedName.substr(0, 12) + "...";
            
             dc.SetTextForeground(*wxBLACK); 
            dc.DrawText(clippedName, primaryRect.GetX() + 5, primaryRect.GetY());
        }
        
        y += currentHeight;
        
        if (track.isExpanded)
        {
            for (auto& child : track.children)
                drawTrack(child, indent + 1);
        }
    };
    
    for (auto& track : project->tracks)
        drawTrack(track, 0);
        
    // Draw Add Track Button
    int btnH = 30;
    wxRect addBtnRect(10, y + 10, width - 20, btnH);
    
    dc.SetBrush(wxBrush(wxColour(60, 60, 60)));
    dc.SetPen(wxPen(wxColour(100, 100, 100)));
    dc.DrawRoundedRectangle(addBtnRect, 4);
    
    dc.SetTextForeground(*wxWHITE);
    wxFont btnFont = dc.GetFont();
    btnFont.SetWeight(wxFONTWEIGHT_BOLD);
    dc.SetFont(btnFont);
    
    wxString label = "+ Add Track";
    wxSize txtSize = dc.GetTextExtent(label);
    dc.DrawText(label, addBtnRect.GetX() + (addBtnRect.GetWidth() - txtSize.GetWidth())/2, 
                       addBtnRect.GetY() + (addBtnRect.GetHeight() - txtSize.GetHeight())/2);
                       
    // Draw Drag Insertion Line
    if (isDraggingTrack && currentDropTarget.isValid)
    {
        int targetY = headerHeight;
        bool found = false;
        
        std::function<void(Track&)> findY = [&](Track& t) {
            if (found) return;
            
            // Check if this is the target parent's first child location
            if (currentDropTarget.parent == &t && currentDropTarget.index == 0) {
                 targetY += 80; // After parent header
                 found = true;
                 return;
            }
            
            // Check if we are inserting before 't'
            // To do this strictly, we need to match pointer or indices.
            // Simplified: we rely on iteration order matches.
        };
        
        // Retraverse to find screen Y of the target index
        int curY = headerHeight;
        // Logic: Iterate linear visible tracks. 
        // If DropTarget is (Parent=nullptr, Index=N), we want the Nth parent top Y.
        // If DropTarget is (Parent=P, Index=N), we want the Nth child of P Y.
        
        if (currentDropTarget.parent == nullptr)
        {
             // Root level
             int pIdx = 0;
             for (auto& t : project->tracks) {
                 if (pIdx == currentDropTarget.index) {
                     targetY = curY;
                     break;
                 }
                 int h = (t.name.find("%)") != std::string::npos) ? 40 : 80;
                 curY += h;
                 if (t.isExpanded) {
                     for (auto& c : t.children) curY += 40; // Children are 40
                 }
                 pIdx++;
             }
             if (pIdx == currentDropTarget.index) targetY = curY; // End of list
        }
        else
        {
             // Child level
             for (auto& t : project->tracks) {
                 int h = (t.name.find("%)") != std::string::npos) ? 40 : 80;
                 if (&t == currentDropTarget.parent)
                 {
                     curY += h; // Skip parent itself
                     int cIdx = 0;
                     for (auto& c : t.children) {
                         if (cIdx == currentDropTarget.index) {
                             targetY = curY;
                             goto found_y;
                         }
                         curY += 40;
                         cIdx++;
                     }
                     if (cIdx == currentDropTarget.index) targetY = curY; // End of child list
                     goto found_y;
                 }
                 curY += h;
                 if (t.isExpanded) {
                     for (auto& c : t.children) curY += 40;
                 }
             }
        }
        
        found_y:
        dc.SetPen(wxPen(wxColour(0, 255, 255), 2));
        dc.DrawLine(0, targetY, width, targetY);
    }
}

void TrackList::OnMouseEvents(wxMouseEvent& evt)
{
    if (evt.LeftDown())
    {
        if (!project) return;
        
        // Convert mouse to logical coordinates (add scroll offset)
        int clickX = evt.GetX();
        int clickY = evt.GetY() + scrollOffsetY;
        
        int rulerHeight = 30;
        int masterTrackHeight = 100;
        int headerHeight = rulerHeight + masterTrackHeight;
        
        int y = headerHeight; 
        int width = GetClientSize().GetWidth();
        
        bool handled = false;
        std::function<void(Track&, int)> hitTest = [&](Track& track, int indent) {
            if (handled) return; 
            
            int currentHeight = (indent == 0) ? 80 : 40;
            
            if (clickY >= y && clickY < y + currentHeight)
            {
                int localY = clickY - y;
                
                // 1. Expander
                if (!track.children.empty())
                {
                    wxRect expanderRect(indent * 10 + 2, 2, 16, 16);
                    if (clickX >= expanderRect.GetX() && clickX <= expanderRect.GetRight() &&
                        localY >= expanderRect.GetY() && localY <= expanderRect.GetBottom())
                    {
                        track.isExpanded = !track.isExpanded;
                        Refresh();
                        GetParent()->Refresh(); 
                        handled = true;
                        return;
                    }
                }
                
                // 2. Buttons (Mute/Solo) - Only for Parents
                if (indent == 0)
                {
                    int btnWidth = 40;
                    int btnHeight = (indent == 0) ? 18 : 14;
                    int btnY = (indent == 0) ? 25 : 20;
                    
                    wxRect muteRect(5, btnY, btnWidth, btnHeight);
                    wxRect soloRect(5 + btnWidth + 5, btnY, btnWidth, btnHeight);
                    
                    if (localY >= btnY && localY <= btnY + btnHeight)
                    {
                         if (clickX >= muteRect.GetX() && clickX <= muteRect.GetRight())
                         {
                             track.mute = !track.mute;
                             Refresh(); 
                             handled = true;
                             return;
                         }
                         if (clickX >= soloRect.GetX() && clickX <= soloRect.GetRight())
                         {
                             track.solo = !track.solo;
                             Refresh();
                             handled = true;
                             return;
                         }
                    }
                }
                
                // 3. Slider (Child only)
                if (indent > 0)
                {
                    int btnY = 20;
                    int sliderX = 100;
                    int sliderW = 100;
                    int sliderH = 14;
                    
                    if (localY >= btnY && localY <= btnY + sliderH &&
                        clickX >= sliderX && clickX <= sliderX + sliderW + 30)
                    {
                        isDraggingSlider = true;
                        sliderTrack = &track;
                        
                        double val = (double)(clickX - sliderX) / sliderW;
                         if (val < 0) val = 0;
                         if (val > 1) val = 1;
                         sliderTrack->gain = val;
                         Refresh();
                         handled = true;
                         return;
                    }
                }
                
                // 4. Primary Selector (Parent only)
                if (indent == 0 && !track.children.empty())
                {
                    int btnY = 25;
                    int btnHeight = 18;
                    int primaryX = 100; 
                    wxRect primaryRect(primaryX, btnY, 120, btnHeight);
                    
                    if (clickX >= primaryRect.GetX() && clickX <= primaryRect.GetRight() &&
                        localY >= btnY && localY <= btnY + btnHeight)
                    {
                        track.primaryChildIndex++;
                        if (track.primaryChildIndex >= (int)track.children.size())
                            track.primaryChildIndex = 0;
                        Refresh();
                        GetParent()->Refresh(); 
                        handled = true;
                        return;
                    }
                }
            }
            
            y += currentHeight;
            
            if (track.isExpanded)
            {
                 for (auto& child : track.children)
                 {
                     hitTest(child, indent + 1);
                     if (handled) return;
                 }
            }
        };

        for (auto& t : project->tracks)
        {
            hitTest(t, 0);
            if (handled) break;
        }
        
        if (!handled)
        {
            // Possibly Start Drag Candidate if clicked on a track body
            // Identify track under mouse
             std::function<Track*(Track&, int, int&)> findTrack = [&](Track& tr, int indent, int& curY) -> Track* {
                 int h = (indent==0 ? 80 : 40);
                 if (clickY >= curY && clickY < curY + h) return &tr;
                 curY += h;
                 if (tr.isExpanded) {
                     for (auto& c : tr.children) {
                         Track* res = findTrack(c, indent+1, curY);
                         if (res) return res;
                     }
                 }
                 return nullptr;
             };
             
             int trackScanY = 130;
             Track* hit = nullptr;
             for (auto& t : project->tracks) {
                 hit = findTrack(t, 0, trackScanY);
                 if (hit) break;
             }
             
             if (hit)
             {
                 dragSourceTrack = hit;
                 dragSourceY = clickY; 
                 // Don't set isDraggingTrack yet, wait for threshold
             }
        }
            
        // Check "Add Track" Button
        if (!handled)
        {
            int btnH = 30;
            wxRect addBtnRect(10, y + 10, width - 20, btnH);
            
            if (clickX >= addBtnRect.GetX() && clickX <= addBtnRect.GetRight() &&
                clickY >= addBtnRect.GetY() && clickY <= addBtnRect.GetBottom())
            {
             // Open Dialog
             AddTrackDialog dlg(this);
             if (dlg.ShowModal() == wxID_OK)
             {
                 AddTrackResult res = dlg.GetResult();
                 if (res.confirmed)
                 {
                     // Logic to add track
                     std::string setStr = (res.bank==SampleSet::Normal)?"normal":(res.bank==SampleSet::Soft?"soft":"drum");
                     std::string typeStr = (res.type==SampleType::HitNormal)?"hitnormal":(res.type==SampleType::HitWhistle?"hitwhistle":(res.type==SampleType::HitFinish?"hitfinish":"hitclap"));
                     std::string parentName = setStr + "-" + typeStr;
                     
                     Track* parent = nullptr;
                     for (auto& t : project->tracks) {
                         if (t.name == parentName) { parent = &t; break; }
                     }
                     
                     if (!parent)
                     {
                         Track newParent;
                         newParent.name = parentName;
                         newParent.sampleSet = res.bank;
                         newParent.sampleType = res.type;
                         newParent.isExpanded = true;
                         project->tracks.push_back(newParent);
                         parent = &project->tracks.back();
                     }
                     
                     // Check child volume
                     int vol = res.volume;
                     std::string childName = parentName + " (" + std::to_string(vol) + "%)";
                     
                     bool childExists = false;
                     for (auto& c : parent->children) {
                         if (c.name == childName) { childExists = true; break; }
                     }
                     
                     if (!childExists)
                     {
                         Track child;
                         child.name = childName;
                         child.sampleSet = res.bank;
                         child.sampleType = res.type;
                         child.gain = (float)vol / 100.0f;
                         parent->children.push_back(child);
                     }
                     
                     Refresh();
                     GetParent()->Refresh(); // Refresh Timeline
                 }
             }
        }
        }
    }
    else if (evt.Dragging())
    {
        if (isDraggingSlider && sliderTrack)
        {
            // Handle Slider Drag - convert mouse to logical coords
            int clickX = evt.GetX();
            int sliderX = 100;
            int sliderW = 100;

            double val = (double)(clickX - sliderX) / sliderW;
            if (val < 0) val = 0;
            if (val > 1) val = 1;
            
            sliderTrack->gain = val;
            Refresh();
        }
        else if (!isDraggingSlider && dragSourceTrack)
        {
            // Track Drag Logic
            int y = evt.GetY() + scrollOffsetY;
            if (!isDraggingTrack)
            {
                if (std::abs(y - dragSourceY) > 5)
                {
                    isDraggingTrack = true;
                    SetCursor(wxCursor(wxCURSOR_HAND));
                }
            }
            
            if (isDraggingTrack)
            {
                dragCurrentY = y;
                
                // Determine Drop Target
                int curY = 130; 
                bool isSourceChild = (!dragSourceTrack->children.empty() == false && dragSourceTrack->name.find("%)") != std::string::npos); // Heuristic or check project
                // Better check: is it in a children vector?
                // Logic: Find source in structure
                
                Track* sourceParent = nullptr;
                for (auto& p : project->tracks) {
                    for (auto& c : p.children) {
                        if (&c == dragSourceTrack) { sourceParent = &p; break; }
                    }
                    if (sourceParent) break;
                }
                
                // Reset valid
                currentDropTarget.isValid = false;
                
                // Hit test for Y
                // We iterate to find where 'y' falls in the list
                
                if (sourceParent)
                {
                     // Dragging a Child
                     // Can only drop within sourceParent's children
                     currentDropTarget.parent = sourceParent;
                     
                     // Find Y range of parent's children
                     // Find parent Y
                     int pY = 130;
                     for (auto& t : project->tracks) {
                         if (&t == sourceParent) break;
                         pY += (t.name.find("%)") != std::string::npos ? 40 : 80); // Should be 80 for parent
                         if (t.isExpanded) pY += t.children.size() * 40;
                     }
                     
                     // Start checking child slots
                     int childStartY = pY + 80;
                     int cIdx = 0;
                     int bestIdx = -1;
                     
                     // If we are above the children, insert at 0
                     if (dragCurrentY < childStartY) bestIdx = 0;
                     else
                     {
                         for (size_t i=0; i < sourceParent->children.size(); ++i)
                         {
                             int midY = childStartY + 40/2;
                             if (dragCurrentY < midY) { bestIdx = i; break; }
                             childStartY += 40;
                         }
                         if (bestIdx == -1) bestIdx = sourceParent->children.size();
                     }
                     
                     currentDropTarget.index = bestIdx;
                     currentDropTarget.isValid = true;
                }
                else
                {
                    // Dragging a Parent
                    currentDropTarget.parent = nullptr;
                    int pIdx = 0;
                    int bestIdx = -1;
                    int pY = 130;
                    
                    for (auto& t : project->tracks)
                    {
                        // Calculate total height of this block
                        int h = 80;
                        if (t.isExpanded) h += t.children.size() * 40;
                        
                        // Check if y is in upper half of this block? No, simpler: Insert *before* block if Y < center?
                        // Actually easier to just check discrete boundaries.
                        // Let's assume insertion point is between blocks.
                        // If y < pY + h/2, insert before t.
                        if (dragCurrentY < pY + h/2) { bestIdx = pIdx; break; }
                        
                        pY += h;
                        pIdx++;
                    }
                    if (bestIdx == -1) bestIdx = project->tracks.size();
                    
                    currentDropTarget.index = bestIdx;
                    currentDropTarget.isValid = true;
                }
                
                Refresh();
            }
        }
    }
    else if (evt.LeftUp())
    {
        if (isDraggingTrack && currentDropTarget.isValid && dragSourceTrack)
        {
             // Execute Move
             if (currentDropTarget.parent) // Moving Child
             {
                 auto& kids = currentDropTarget.parent->children;
                 // Find source index
                 int srcIdx = -1;
                 for(int i=0; i<(int)kids.size(); ++i) if (&kids[i] == dragSourceTrack) { srcIdx=i; break; }
                 
                 if (srcIdx != -1)
                 {
                     int destIdx = currentDropTarget.index;
                     
                     // Adjust for shift if moving downwards
                     if (srcIdx < destIdx) destIdx--;
                     
                     if (srcIdx != destIdx && destIdx >= 0 && destIdx <= (int)kids.size()-1)
                     {
                         Track temp = std::move(kids[srcIdx]);
                         kids.erase(kids.begin() + srcIdx);
                         kids.insert(kids.begin() + destIdx, std::move(temp));
                         
                         // Rule: If moved to top (index 0), reset primaryChildIndex ?
                         if (destIdx == 0) currentDropTarget.parent->primaryChildIndex = 0;
                         // Else if we moved old primary away? Logic is complex based on user req.
                         // User: "Move to top -> New Primary".
                         // So if destIdx == 0, we effectively want that track to be primary.
                     }
                 }
             }
             else // Moving Parent
             {
                 auto& tracks = project->tracks;
                 int srcIdx = -1;
                 for(int i=0; i<(int)tracks.size(); ++i) if (&tracks[i] == dragSourceTrack) { srcIdx=i; break; }
                 
                 if (srcIdx != -1)
                 {
                     int destIdx = currentDropTarget.index;
                     if (srcIdx < destIdx) destIdx--;
                     
                     if (srcIdx != destIdx && destIdx >= 0 && destIdx <= (int)tracks.size()-1)
                     {
                         Track temp = std::move(tracks[srcIdx]);
                         tracks.erase(tracks.begin() + srcIdx);
                         tracks.insert(tracks.begin() + destIdx, std::move(temp));
                     }
                 }
             }
             
             // Refresh Everything
             Refresh();
             if (timelineView) timelineView->Refresh();
        }
        
        isDraggingSlider = false;
        sliderTrack = nullptr;
        isDraggingTrack = false;
        dragSourceTrack = nullptr;
        SetCursor(wxCursor(wxCURSOR_ARROW));
        Refresh();
    }
}
