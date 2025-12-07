#include "TrackList.h"
#include "../model/Project.h"
#include "AddTrackDialog.h"
#include <wx/dcbuffer.h>

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
    else if (evt.Dragging() && isDraggingSlider && sliderTrack)
    {
        // Handle Slider Drag - convert mouse to logical coords
        int clickX = evt.GetX();

        // Same logic: sliderX=100, sliderW=100
        int sliderX = 100;
        int sliderW = 100;

        double val = (double)(clickX - sliderX) / sliderW;
        if (val < 0) val = 0;
        if (val > 1) val = 1;
        
        sliderTrack->gain = val;
        Refresh();
    }
    else if (evt.LeftUp())
    {
        isDraggingSlider = false;
        sliderTrack = nullptr;
    }
}
