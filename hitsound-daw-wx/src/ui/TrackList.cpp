#include "TrackList.h"
#include "../model/Project.h"
#include "AddTrackDialog.h"
#include "AddGroupingDialog.h"
// Include TimelineView header for full definition
#include "TimelineView.h" 
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <cmath>
#include <algorithm>
#include <functional>
#include <wx/bmpbndl.h>
#include <wx/filename.h>

wxBEGIN_EVENT_TABLE(TrackList, wxPanel)
    EVT_PAINT(TrackList::OnPaint)
    EVT_SIZE(TrackList::OnSize)
    EVT_MOUSE_EVENTS(TrackList::OnMouseEvents)
    EVT_CONTEXT_MENU(TrackList::OnContextMenu)
    EVT_MOUSE_CAPTURE_LOST(TrackList::OnCaptureLost)
wxEND_EVENT_TABLE()

#include <wx/file.h>

// Helper to load and recolor SVGs robustly (borrowed from TransportPanel logic)
static wxBitmap LoadIconHelper(const wxString& name, const wxColor& color)
{
    wxString path = "Resources/icon/" + name;
    if (!wxFileExists(path)) {
            if (wxFileExists("../Resources/icon/" + name)) path = "../Resources/icon/" + name;
            else if (wxFileExists("../../Resources/icon/" + name)) path = "../../Resources/icon/" + name;
    }
    
    if (!wxFileExists(path)) return wxBitmap();

    wxFile file(path);
    wxString svgContent;
    if (file.IsOpened()) file.ReadAll(&svgContent);
    
    wxString hex = color.GetAsString(wxC2S_HTML_SYNTAX);
    svgContent.Replace("#000000", hex);
    svgContent.Replace("black", hex);
    
    wxBitmapBundle bundle = wxBitmapBundle::FromSVG(svgContent.ToStdString().c_str(), wxSize(16, 16));
    if (bundle.IsOk()) return bundle.GetBitmap(wxSize(16, 16));
    return wxBitmap();
}

TrackList::TrackList(wxWindow* parent)
    : wxPanel(parent)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(wxColour(40, 40, 40));
    
    // Load Icons
    wxColour iconColor(200, 200, 200);
    trackIcon = LoadIconHelper("track.svg", iconColor);
    groupingIcon = LoadIconHelper("grouping.svg", iconColor);
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
    
    return height; // Removed +400 for button space
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
    
    // Draw Ruler (Blank for now in TrackList -> Now holds buttons)
    wxRect rulerRect(0, y, width, rulerHeight);
    dc.SetBrush(wxBrush(wxColour(50, 50, 50)));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(rulerRect);
    
    // Draw "Add Track" and "Add Grouping" Buttons in Ruler Area
    // Layout: [Icon] Add Track  [Icon] Add Grouping
    // Font size small?
    
    wxFont btnFont = dc.GetFont();
    btnFont.SetPointSize(9);
    btnFont.SetWeight(wxFONTWEIGHT_NORMAL);
    dc.SetFont(btnFont);
    dc.SetTextForeground(wxColour(200, 200, 200));
    
    int btnY = y + (rulerHeight - 16) / 2; // Center vertically (16px icon)
    int curX = 10;
    
    // 1. Add Track
    if (trackIcon.IsOk())
    {
        dc.DrawBitmap(trackIcon, curX, btnY, true);
        curX += 20; // Icon width + spacing
    }
    dc.DrawText("Add Track", curX, btnY);
    curX += dc.GetTextExtent("Add Track").GetWidth() + 15; // Spacing
    
    // 2. Add Grouping
    if (groupingIcon.IsOk())
    {
        dc.DrawBitmap(groupingIcon, curX, btnY, true);
        curX += 20;
    }
    dc.DrawText("Add Grouping", curX, btnY);
    
    // Master Track
    int masterY = y + rulerHeight;
    wxRect masterRect(0, masterY, width, masterTrackHeight);
    
    dc.SetPen(wxPen(borderCol));
    // Distinct Master Color
    dc.SetBrush(wxBrush(wxColour(45, 45, 60)));
    dc.DrawRectangle(masterRect);
    
    dc.SetTextForeground(*wxWHITE);
    wxFont masterTitleFont = dc.GetFont();
    masterTitleFont.SetWeight(wxFONTWEIGHT_BOLD);
    masterTitleFont.SetPointSize(11);
    dc.SetFont(masterTitleFont);
    
    // Display "Artist - Title" with truncation if too long
    wxString masterTitle = "Master Audio";
    if (project && (!project->artist.empty() || !project->title.empty()))
    {
        masterTitle = wxString::FromUTF8(project->artist) + " - " + wxString::FromUTF8(project->title);
    }
    
    // Truncate with ellipsis if text is too long (leave room for offset display)
    int maxTitleWidth = width - 30; // 10px margin on each side, plus some buffer
    wxSize titleSize = dc.GetTextExtent(masterTitle);
    if (titleSize.GetWidth() > maxTitleWidth)
    {
        // Binary search for truncation point
        while (masterTitle.Length() > 3 && dc.GetTextExtent(masterTitle + "...").GetWidth() > maxTitleWidth)
        {
            masterTitle = masterTitle.Left(masterTitle.Length() - 1);
        }
        masterTitle += "...";
    }
    
    dc.DrawText(masterTitle, 10, masterY + 15);
    
    // Show offset from first timing point
    wxFont infoFont = dc.GetFont();
    infoFont.SetWeight(wxFONTWEIGHT_NORMAL);
    infoFont.SetPointSize(9);
    dc.SetFont(infoFont);
    dc.SetTextForeground(wxColour(200, 200, 200));
    
    double firstOffset = 0.0;
    if (project && !project->timingPoints.empty())
    {
        // Find first uninherited timing point
        for (const auto& tp : project->timingPoints)
        {
            if (tp.uninherited)
            {
                firstOffset = tp.time;
                break;
            }
        }
    }
    
    // Display BPM and Offset on separate lines
    wxString bpmStr = wxString::Format("%.0f BPM", project ? project->bpm : 0.0);
    wxString offsetStr = wxString::Format("%.0fms", firstOffset);
    
    dc.DrawText(bpmStr, 10, masterY + 45);
    dc.DrawText(offsetStr, 10, masterY + 65);
    
    y += headerHeight;
    
    // Helper for Abbrev
    auto getAbbrev = [](SampleSet s, SampleType t) -> std::pair<std::string, bool> {
        std::string sStr = (s == SampleSet::Normal) ? "n" : (s == SampleSet::Soft ? "s" : "d");
        std::string tStr;
        switch(t) {
            case SampleType::HitNormal: tStr = "hn"; break;
            case SampleType::HitWhistle: tStr = "hw"; break;
            case SampleType::HitFinish: tStr = "hf"; break;
            case SampleType::HitClap:   tStr = "hc"; break;
            default: tStr = "??"; break;
        }
        return {sStr + "-" + tStr, (t == SampleType::HitNormal)};
    };

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
        // Name
        dc.SetTextForeground(*wxBLACK);
        int nameX = 25; // Default X

        if (indent > 0) 
        {
             wxFont f = dc.GetFont();
             f.SetPointSize(8);
             dc.SetFont(f);
             
             // Bullet Point
             int dotX = 12;
             int dotY = y + 8; // Align roughly with text
             
             dc.SetBrush(wxBrush(wxColour(100, 100, 100)));
             dc.SetPen(*wxTRANSPARENT_PEN);
             dc.DrawCircle(dotX, dotY, 3);
             
             nameX = 25; // Fixed offset next to bullet
        }
        else
        {
             wxFont f = dc.GetFont();
             f.SetPointSize(10);
             f.SetWeight(wxFONTWEIGHT_BOLD);
             dc.SetFont(f);
             nameX = 25; // Parent indent
        }
        
        dc.DrawText(track.name, nameX, y + 2);
        
        // -------------------------------------------------------------------------
        // Abbreviation Rendering (Bottom Right of Parents)
        // -------------------------------------------------------------------------
        if (indent == 0 && !track.children.empty())
        {
            // Analyze content of first child (assuming homogeneity)
            const Track& source = track.children[0];
            
            struct TextBlob { std::string text; bool bold; };
            std::vector<TextBlob> blobs;
            
            std::vector<TextBlob> bases;
            std::vector<TextBlob> adds;
            
            auto process = [&](SampleSet s, SampleType t) {
                auto p = getAbbrev(s, t);
                if (t == SampleType::HitNormal) bases.push_back({p.first, p.second});
                else adds.push_back({p.first, p.second});
            };
            
            if (!source.layers.empty())
            {
                for (auto& l : source.layers) process(l.bank, l.type);
            }
            else
            {
                process(source.sampleSet, source.sampleType);
            }
            
            // Construct final list: Base1, Base2 + Add1, Add2
            for (size_t i=0; i<bases.size(); ++i) {
                if (i > 0) blobs.push_back({", ", false});
                blobs.push_back(bases[i]);
            }
            
            if (!bases.empty() && !adds.empty())
                blobs.push_back({" + ", false});
                
            for (size_t i=0; i<adds.size(); ++i) {
                if (i > 0) blobs.push_back({", ", false});
                blobs.push_back(adds[i]);
            }
            
            // Measure total width
            wxFont smallFont = dc.GetFont();
            smallFont.SetPointSize(8);
            
            int totalW = 0;
            for (auto& b : blobs) {
                smallFont.SetWeight(b.bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL);
                dc.SetFont(smallFont);
                totalW += dc.GetTextExtent(b.text).GetWidth();
            }
            
            // Draw Right Aligned
            int marginX = 10;
            int marginY = 5;
            int startX = width - marginX - totalW;
            int drawY = y + currentHeight - marginY - 12; // 12 approx height
            
            dc.SetTextForeground(wxColour(100, 100, 100)); // Gray text
            
            int curX = startX;
            for (auto& b : blobs) {
                smallFont.SetWeight(b.bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL);
                dc.SetFont(smallFont);
                dc.DrawText(b.text, curX, drawY);
                curX += dc.GetTextExtent(b.text).GetWidth();
            }
        }
        
        // Controls
        int btnWidth = 40;
        int btnHeight = (indent == 0) ? 18 : 14;
        int btnY = y + ((indent == 0) ? 25 : 20); 
        
        wxRect muteRect(5, btnY, btnWidth, btnHeight);
        wxRect soloRect(5 + btnWidth + 5, btnY, btnWidth, btnHeight);
        
        // Mute/Solo - Only for Parents
        // Mute/Solo - Only for Parents
        if (indent == 0)
        {
            // Vertical Stack
            // Previous height was 18. Let's make them roughly square-ish or slightly rectangular.
            // Parent height is 80. Title uses ~20. Available ~60.
            // Let's place nicely.
            
            // Mute
            wxRect muteRect(5, y + 25, 30, 20); // Width 30, Height 20
            dc.SetBrush(track.mute ? wxBrush(wxColour(100, 100, 200)) : wxBrush(wxColour(200, 200, 200)));
            dc.SetPen(wxPen(wxColour(80, 80, 80)));
            dc.DrawRoundedRectangle(muteRect, 2);
            
            wxFont btnF = dc.GetFont();
            btnF.SetWeight(wxFONTWEIGHT_BOLD);
            btnF.SetPointSize(9); // Ensure size
            dc.SetFont(btnF);
            dc.SetTextForeground(*wxBLACK);
            
            // Center 'M'
            wxSize tz = dc.GetTextExtent("M");
            dc.DrawText("M", muteRect.x + (muteRect.width - tz.x)/2, muteRect.y + (muteRect.height - tz.y)/2);
            
            // Solo
            wxRect soloRect(5, y + 50, 30, 20);
            dc.SetBrush(track.solo ? wxBrush(wxColour(100, 200, 100)) : wxBrush(wxColour(200, 200, 200)));
            dc.DrawRoundedRectangle(soloRect, 2);
            
            tz = dc.GetTextExtent("S");
            dc.DrawText("S", soloRect.x + (soloRect.width - tz.x)/2, soloRect.y + (soloRect.height - tz.y)/2);
        }
        
        // Playback Controls (Volume Slider) for Child Tracks
        if (indent > 0)
        {
             // Draw Slider Visuals
             // Dynamic resizing: 75% of track width
             int sliderW = (int)(width * 0.75); // 75% of available width
             // Centered
             int sliderX = (width - sliderW) / 2;
             
             int sliderH = 14;
             int midY = btnY + sliderH / 2;
             
             // 1. Thin Track
             dc.SetPen(wxPen(wxColour(160, 160, 160), 2)); 
             dc.DrawLine(sliderX, midY, sliderX + sliderW, midY);
             
             // 2. Tick Marks (0, 20, 40, 50, 60, 80, 100)
             dc.SetPen(wxPen(wxColour(160, 160, 160), 1));
             int tickH = 4;
             
             // Use a simple array for ticks
             float ticks[] = {0.0f, 0.2f, 0.4f, 0.5f, 0.6f, 0.8f, 1.0f};
             for (float t : ticks) {
                 int tx = sliderX + (int)(t * sliderW);
                 dc.DrawLine(tx, midY - tickH, tx, midY + tickH);
             }
             
             // 3. Thumb (Rectangle)
             int thumbW = 10;
             int thumbH = 14;
             // Map gain to [0, sliderW] relative to X
             int thumbX = sliderX + (int)(track.gain * sliderW) - thumbW / 2;
             
             wxRect thumbRect(thumbX, midY - thumbH/2, thumbW, thumbH);
             
             // Draw Thumb
             dc.SetBrush(wxBrush(wxColour(230, 230, 230)));
             dc.SetPen(wxPen(wxColour(100, 100, 100)));
             dc.DrawRoundedRectangle(thumbRect, 2);
        }
        
        // Primary Track Selector (Only for parents)
        if (indent == 0 && !track.children.empty())
        {
            // Height 20px, Right Aligned
            int pH = 20; 
            int pW = 20;
            int valW = 50;
            int gap = 2;
            int rightMargin = 10;
            
            // Layout: [P][ValueBox] | (Right Edge)
            int valX = width - rightMargin - valW;
            int pX = valX - gap - pW;
            int pY = y + 25; // Same Y as before (aligned with Mute button)
            
            // 1. P Label
            wxRect pLabelRect(pX, pY, pW, pH);
            dc.SetBrush(wxBrush(wxColour(60, 60, 60))); // Dark inactive
            dc.SetPen(wxPen(wxColour(80, 80, 80)));
            dc.DrawRoundedRectangle(pLabelRect, 2);
            
            wxFont fontP = dc.GetFont();
            fontP.SetWeight(wxFONTWEIGHT_NORMAL);
            fontP.SetPointSize(9);
            dc.SetFont(fontP);
            dc.SetTextForeground(*wxWHITE);
            wxSize tz = dc.GetTextExtent("P");
            dc.DrawText("P", pLabelRect.x + (pLabelRect.width - tz.x)/2, pLabelRect.y + (pLabelRect.height - tz.y)/2);
            
            // 2. Value Box
            wxRect valRect(valX, pY, valW, pH);
            dc.SetBrush(wxBrush(wxColour(220, 220, 220)));
            dc.SetPen(wxPen(wxColour(100, 100, 100)));
            dc.DrawRoundedRectangle(valRect, 2);
            
            // Text: % of primary
            wxString valStr = "---";
            if (track.primaryChildIndex >= 0 && track.primaryChildIndex < (int)track.children.size())
            {
                int g = (int)(track.children[track.primaryChildIndex].gain * 100.0f);
                valStr = wxString::Format("%d%%", g);
            }
            
            dc.SetTextForeground(*wxBLACK);
            tz = dc.GetTextExtent(valStr);
            dc.DrawText(valStr, valRect.x + (valRect.width - tz.x)/2, valRect.y + (valRect.height - tz.y)/2);
            
            // Small triangle
            wxPoint tri[] = {
                 wxPoint(valRect.GetRight() - 5, valRect.GetBottom() - 5),
                 wxPoint(valRect.GetRight() - 2, valRect.GetBottom() - 5),
                 wxPoint(valRect.GetRight() - 3, valRect.GetBottom() - 3)
            };
            dc.SetBrush(*wxBLACK_BRUSH);
            dc.DrawPolygon(3, tri);
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

void TrackList::OnContextMenu(wxContextMenuEvent& evt)
{
    if (!project) return;
    
    // 1. Calculate hit point
    wxPoint mousePos = evt.GetPosition();
    if (mousePos.x == -1 && mousePos.y == -1) 
        mousePos = ClientToScreen(wxPoint(0, 0));
        
    wxPoint clientPos = ScreenToClient(mousePos);
    int y = clientPos.y + scrollOffsetY;
    
    // 2. Hit Test to find Track
    int rulerHeight = 30;
    int masterTrackHeight = 100;
    int headerHeight = rulerHeight + masterTrackHeight;
    
    // If in header, ignore
    if (y < headerHeight) return;
    
    int curY = headerHeight;
    
    Track* foundTrack = nullptr;
    Track* foundParent = nullptr; 
    bool isChild = false;
    
    for (auto& t : project->tracks)
    {
        int h = (t.name.find("%)") != std::string::npos) ? 40 : 80;
        
        if (y >= curY && y < curY + h)
        {
            foundTrack = &t;
            isChild = false;
            break;
        }
        curY += h;
        
        if (t.isExpanded)
        {
            for (auto& c : t.children)
            {
                if (y >= curY && y < curY + 40)
                {
                    foundTrack = &c;
                    foundParent = &t;
                    isChild = true;
                    goto found;
                }
                curY += 40;
            }
        }
    }
found:

    if (!foundTrack) return;
    
    // 3. Build Menu
    wxMenu menu;
    
    enum {
        ID_ADD_CHILD = 10001,
        ID_DELETE_TRACK
    };
    
    if (isChild)
    {
        menu.Append(ID_DELETE_TRACK, "Delete Child Track");
    }
    else
    {
        menu.Append(ID_ADD_CHILD, "Add Child"); // Immediate action, no details needed
        menu.AppendSeparator();
        menu.Append(ID_DELETE_TRACK, "Delete Parent Track"); 
    }
    
    // 4. Handle Selection
    int selected = GetPopupMenuSelectionFromUser(menu);
    
    if (selected == ID_DELETE_TRACK)
    {
        if (isChild && foundParent)
        {
             auto it = foundParent->children.begin();
             while (it != foundParent->children.end()) {
                 if (&(*it) == foundTrack) {
                     it = foundParent->children.erase(it);
                     break; 
                 } else ++it;
             }
        }
        else if (!isChild)
        {
             auto it = project->tracks.begin();
             while (it != project->tracks.end()) {
                 if (&(*it) == foundTrack) {
                     it = project->tracks.erase(it);
                     break;
                 } else ++it;
             }
        }
        
        if (timelineView) timelineView->UpdateVirtualSize();
        Refresh();
        if (GetParent()) GetParent()->Refresh();
    }
    else if (selected == ID_ADD_CHILD)
    {
         // Auto-Add Logic (Migrated from OnMouseEvents)
         // Definition: 50% volume, inherited types
         
         if (foundTrack->isGrouping)
         {
             // Grouping Logic
             SampleSet targetSet = SampleSet::Normal;
             SampleType targetType = SampleType::HitNormal;
             
             if (!foundTrack->children.empty()) {
                 if (!foundTrack->children[0].layers.empty()) {
                     targetSet = foundTrack->children[0].layers[0].bank;
                     targetType = foundTrack->children[0].layers[0].type;
                 } else {
                     targetSet = foundTrack->children[0].sampleSet;
                     targetType = foundTrack->children[0].sampleType;
                 }
             }
             
             Track child;
             child.name = foundTrack->name + " (50%)"; // Inherit name from group?
             child.gain = 0.5f;
             
             // Copy structure
             if (!foundTrack->children.empty()) {
                 child.layers = foundTrack->children[0].layers;
             } else {
                 child.layers.push_back({targetSet, targetType});
             }
             
             foundTrack->children.push_back(child);
             foundTrack->isExpanded = true;
         }
         else
         {
             // Standard Track Logic
             Track child;
             child.sampleSet = foundTrack->sampleSet;
             child.sampleType = foundTrack->sampleType;
             child.gain = 0.5f; 
             
             std::string sStr = (child.sampleSet==SampleSet::Normal)?"normal":(child.sampleSet==SampleSet::Soft?"soft":"drum");
             std::string tStr = (child.sampleType==SampleType::HitNormal)?"hitnormal":(child.sampleType==SampleType::HitWhistle?"hitwhistle":(child.sampleType==SampleType::HitFinish?"hitfinish":"hitclap"));
             
             child.name = sStr + "-" + tStr + " (50%)";
             
             foundTrack->children.push_back(child);
             foundTrack->isExpanded = true;
         }
         
         if (timelineView) timelineView->UpdateVirtualSize();
         Refresh();
         if (GetParent()) GetParent()->Refresh();
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
        
        // Header Hit Testing (Ruler Area for Buttons)
        if (clickY < headerHeight)
        {
             // Check if in Ruler part (0 to 30)
             if (clickY < 30) 
             {
                 int btnY = (30 - 16) / 2;
                 int curX = 10;
                 
                 // Add Track
                 wxClientDC dc(this);
                 wxFont btnFont = dc.GetFont();
                 btnFont.SetPointSize(9); 
                 dc.SetFont(btnFont);
                 
                 int w1 = 20 + dc.GetTextExtent("Add Track").GetWidth();
                 
                 if (clickX >= curX && clickX <= curX + w1)
                 {
                     AddTrackDialog dlg(this);
                     if (dlg.ShowModal() == wxID_OK)
                     {
                         AddTrackResult res = dlg.GetResult();
                         if (res.confirmed)
                         {
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
                             if (timelineView) timelineView->UpdateVirtualSize();
                             GetParent()->Refresh(); 
                         }
                     }
                     return; 
                 }
                 
                 curX += w1 + 15;
                 
                 // Add Grouping
                 int w2 = 20 + dc.GetTextExtent("Add Grouping").GetWidth();
                 if (clickX >= curX && clickX <= curX + w2)
                 {
                     AddGroupingDialog dlg(this);
                     if (dlg.ShowModal() == wxID_OK)
                     {
                         auto res = dlg.GetResult();
                         if (res.confirmed)
                         {
                             Track grouping;
                             grouping.name = res.name.ToStdString();
                             grouping.isGrouping = true;
                             grouping.isExpanded = true;
                             grouping.mute = false;
                             grouping.solo = false;
                             grouping.gain = 1.0; 
                             
                             float vol = (float)res.volume / 100.0f;
                             std::string volPercent = std::to_string(res.volume) + "%";
                             
                             Track child;
                             child.name = res.name.ToStdString() + " (" + volPercent + ")";
                             child.gain = vol;
                             
                             child.layers.push_back({res.hitNormalBank, SampleType::HitNormal});
                             if (res.hasWhistle) child.layers.push_back({res.additionsBank, SampleType::HitWhistle});
                             if (res.hasFinish) child.layers.push_back({res.additionsBank, SampleType::HitFinish});
                             if (res.hasClap) child.layers.push_back({res.additionsBank, SampleType::HitClap});
                             
                             child.sampleSet = res.hitNormalBank;
                             child.sampleType = SampleType::HitNormal;
                             
                             grouping.children.push_back(child);
                             
                             project->tracks.push_back(grouping);
                             
                             Refresh();
                             if (timelineView) timelineView->UpdateVirtualSize();
                             GetParent()->Refresh(); 
                         }
                     }
                     return;
                 }
             }
        }

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
                        if (timelineView) timelineView->UpdateVirtualSize();
                        GetParent()->Refresh(); 
                        handled = true;
                        return;
                    }
                }
                
                // 2. Buttons (Mute/Solo) - Only for Parents
                if (indent == 0)
                {
                    wxRect muteRect(5, 25, 30, 20); 
                    wxRect soloRect(5, 50, 30, 20);
                    
                    if (clickX >= muteRect.x && clickX <= muteRect.GetRight() &&
                        localY >= muteRect.y && localY <= muteRect.GetBottom())
                    {
                         track.mute = !track.mute;
                         Refresh();
                         handled = true;
                         return;
                    }
                    if (clickX >= soloRect.x && clickX <= soloRect.GetRight() &&
                        localY >= soloRect.y && localY <= soloRect.GetBottom())
                    {
                         track.solo = !track.solo;
                         Refresh();
                         handled = true;
                         return;
                    }
                }
                
                // 3. Slider (Child only)
                if (indent > 0)
                {
                    int btnY = 20;
                    int sliderW = (int)(width * 0.75);
                    int sliderX = (width - sliderW) / 2;
                    int sliderH = 14;
                    
                    if (localY >= btnY - 5 && localY <= btnY + sliderH + 5 &&
                        clickX >= sliderX - 10 && clickX <= sliderX + sliderW + 10)
                    {
                        isDraggingSlider = true;
                        sliderTrack = &track;
                        
                        double val = (double)(clickX - sliderX) / sliderW;
                         if (val < 0) val = 0;
                         if (val > 1) val = 1;
                         sliderTrack->gain = val;
                         
                         std::string& name = sliderTrack->name;
                         size_t parenPos = name.rfind("(");
                         std::string newVol = wxString::Format("(%d%%)", (int)(val * 100)).ToStdString();
                         
                         if (parenPos != std::string::npos) {
                             if (name.find("%", parenPos) != std::string::npos) {
                                  name = name.substr(0, parenPos) + newVol;
                             } else {
                                  name += " " + newVol;
                             }
                         } else {
                             name += " " + newVol;
                         }
                         
                         Refresh();
                         handled = true;
                         return;
                    }
                }
                
                // 4. Primary Selector (Parent only)
                if (indent == 0 && !track.children.empty())
                {
                    int pH = 20; 
                    int pW = 20;
                    int valW = 50;
                    int rightMargin = 10;
                    
                    int valX = width - rightMargin - valW;
                    int btnY = 25;
                    
                    if (clickX >= valX && clickX <= valX + valW &&
                        localY >= btnY && localY <= btnY + pH)
                    {
                        wxMenu menu;
                        for (int i=0; i<(int)track.children.size(); ++i) {
                            wxString lbl = track.children[i].name;
                            menu.AppendRadioItem(10000 + i, lbl);
                            if (i == track.primaryChildIndex) menu.Check(10000 + i, true);
                        }
                        
                        int selectedId = GetPopupMenuSelectionFromUser(menu);
                        if (selectedId >= 10000)
                        {
                            int idx = selectedId - 10000;
                            if (idx >= 0 && idx < (int)track.children.size())
                            {
                                if (idx > 0)
                                {
                                    Track child = std::move(track.children[idx]);
                                    track.children.erase(track.children.begin() + idx);
                                    track.children.insert(track.children.begin(), std::move(child));
                                }
                                track.primaryChildIndex = 0;
                            }
                            Refresh();
                            if (GetParent()) GetParent()->Refresh();
                        }
                        
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
                 isDraggingTrack = true;
                 CaptureMouse();
             }
        }
    }
    else if (evt.RightDown())
    {
        if (!project) return;
        
        int clickX = evt.GetX();
        int clickY = evt.GetY() + scrollOffsetY;
        
        // Find Track
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
            // Only show context menu for Parents (Grouping or Folder)
            // Identify if hit is a child
            bool isChild = (hit->name.find("%)") != std::string::npos); // Robust check? 
            // Better: Check if it's in project->tracks directly?
            // "hit" pointer is stable?
            // Let's use the scan function's "indent" if we can access it.
            // But we already found "hit".
            
            // Re-verify if it's a root track
            bool isRoot = false;
            for(auto& t : project->tracks) {
                if (&t == hit) { isRoot = true; break; }
            }
            
            if (isRoot)
            {
                // Show Context Menu for Parent Track
                wxMenu menu;
                menu.Append(20001, "Add Child");
                menu.Append(20004, "Edit");
                menu.Append(20003, "Delete");
                
                int sel = GetPopupMenuSelectionFromUser(menu);
                if (sel == 20003)
                {
                    // Delete entire parent track (including children)
                    for (auto it = project->tracks.begin(); it != project->tracks.end(); ++it)
                    {
                        if (&(*it) == hit)
                        {
                            project->tracks.erase(it);
                            break;
                        }
                    }
                    
                    Refresh();
                    if (GetParent()) GetParent()->Refresh();
                    if (timelineView) timelineView->UpdateVirtualSize();
                }
                else if (sel == 20004)
                {
                    // Edit the track/grouping
                    if (hit->isGrouping)
                    {
                        // Open AddGroupingDialog in edit mode
                        AddGroupingDialog dlg(this);
                        dlg.SetEditMode(true);
                        
                        // Extract current values from the grouping
                        // Name is the grouping name (without the volume suffix)
                        wxString name = hit->name;
                        
                        // Get values from first child if available
                        SampleSet normalBank = SampleSet::Normal;
                        SampleSet additionsBank = SampleSet::Soft;
                        bool hasWhistle = false, hasFinish = false, hasClap = false;
                        int volume = 100;
                        
                        if (!hit->children.empty()) {
                            auto& firstChild = hit->children[0];
                            volume = (int)(firstChild.gain * 100);
                            
                            for (auto& layer : firstChild.layers) {
                                if (layer.type == SampleType::HitNormal) {
                                    normalBank = layer.bank;
                                } else {
                                    additionsBank = layer.bank;
                                    if (layer.type == SampleType::HitWhistle) hasWhistle = true;
                                    if (layer.type == SampleType::HitFinish) hasFinish = true;
                                    if (layer.type == SampleType::HitClap) hasClap = true;
                                }
                            }
                        }
                        
                        dlg.SetValues(name, normalBank, additionsBank, hasWhistle, hasFinish, hasClap, volume);
                        
                        if (dlg.ShowModal() == wxID_OK)
                        {
                            auto res = dlg.GetResult();
                            if (res.confirmed)
                            {
                                // Update grouping properties
                                hit->name = res.name.ToStdString();
                                
                                // Update first child if exists
                                if (!hit->children.empty()) {
                                    auto& child = hit->children[0];
                                    child.gain = (float)res.volume / 100.0f;
                                    
                                    // Update name with new volume
                                    std::string volPercent = std::to_string(res.volume) + "%";
                                    child.name = res.name.ToStdString() + " (" + volPercent + ")";
                                    
                                    // Rebuild layers
                                    child.layers.clear();
                                    child.layers.push_back({res.hitNormalBank, SampleType::HitNormal});
                                    if (res.hasWhistle) child.layers.push_back({res.additionsBank, SampleType::HitWhistle});
                                    if (res.hasFinish) child.layers.push_back({res.additionsBank, SampleType::HitFinish});
                                    if (res.hasClap) child.layers.push_back({res.additionsBank, SampleType::HitClap});
                                    
                                    child.sampleSet = res.hitNormalBank;
                                    child.sampleType = SampleType::HitNormal;
                                }
                                
                                Refresh();
                                if (GetParent()) GetParent()->Refresh();
                            }
                        }
                    }
                    else
                    {
                        // Open AddTrackDialog in edit mode for standard track
                        AddTrackDialog dlg(this);
                        dlg.SetEditMode(true);
                        dlg.SetValues(hit->name, hit->sampleSet, hit->sampleType, 100);
                        
                        if (dlg.ShowModal() == wxID_OK)
                        {
                            auto res = dlg.GetResult();
                            if (res.confirmed)
                            {
                                // Update track properties
                                hit->name = res.name.ToStdString();
                                hit->sampleSet = res.bank;
                                hit->sampleType = res.type;
                                
                                Refresh();
                                if (GetParent()) GetParent()->Refresh();
                            }
                        }
                    }
                }
                else if (sel == 20001)
                {
                    // Add Child logic
                    if (hit->isGrouping)
                    {
                        SampleSet targetSet = SampleSet::Normal;
                        SampleType targetType = SampleType::HitNormal;
                        
                        if (!hit->children.empty()) {
                            if (!hit->children[0].layers.empty()) {
                                targetSet = hit->children[0].layers[0].bank;
                                targetType = hit->children[0].layers[0].type;
                            } else {
                                targetSet = hit->children[0].sampleSet;
                                targetType = hit->children[0].sampleType;
                            }
                        }
                        
                        Track child;
                        child.name = hit->name + " (50%)";
                        child.gain = 0.5f;
                        
                        if (!hit->children.empty()) {
                            child.layers = hit->children[0].layers;
                        } else {
                            child.layers.push_back({targetSet, targetType});
                        }
                        
                        hit->children.push_back(child);
                        hit->isExpanded = true;
                    }
                    else
                    {
                        // Standard Track
                        Track child;
                        
                        child.sampleSet = hit->sampleSet;
                        child.sampleType = hit->sampleType;
                        child.gain = 0.5f;
                        
                        std::string sStr = (child.sampleSet==SampleSet::Normal)?"normal":(child.sampleSet==SampleSet::Soft?"soft":"drum");
                        std::string tStr = (child.sampleType==SampleType::HitNormal)?"hitnormal":(child.sampleType==SampleType::HitWhistle?"hitwhistle":(child.sampleType==SampleType::HitFinish?"hitfinish":"hitclap"));
                        
                        child.name = sStr + "-" + tStr + " (50%)";
                        
                        hit->children.push_back(child);
                        hit->isExpanded = true;
                    }
                    
                    Refresh();
                    if (GetParent()) GetParent()->Refresh();
                }
            }
            else
            {
                // Child Track - Logic to Delete
                wxMenu menu;
                menu.Append(20002, "Delete Child");
                
                int sel = GetPopupMenuSelectionFromUser(menu);
                if (sel == 20002)
                {
                    // Find Parent of this child
                    Track* parent = nullptr;
                    int childIdx = -1;
                    
                    for(auto& t : project->tracks) {
                        for(size_t i=0; i<t.children.size(); ++i) {
                            if (&t.children[i] == hit) {
                                parent = &t;
                                childIdx = (int)i;
                                break;
                            }
                        }
                        if (parent) break;
                    }
                    
                    if (parent && childIdx != -1)
                    {
                        parent->children.erase(parent->children.begin() + childIdx);
                        if (parent->primaryChildIndex >= parent->children.size())
                            parent->primaryChildIndex = std::max(0, (int)parent->children.size() - 1);
                            
                        Refresh();
                        if (GetParent()) GetParent()->Refresh();
                        if (timelineView) timelineView->UpdateVirtualSize();
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
            int width = GetClientSize().GetWidth();
            
            // Re-calculate slider dimensions based on sliderTrack, but...
            // dragging logic doesn't know 'indent'.
            // However, we can approximate or find indent. 
            // OR: we can ignore indent precision if it's small shift, 
            // but for correct 0-100%, we need sliderX.
            // sliderX = indent * 10 + 25. 
            // We need to find the track to know indent.
            
            // Let's iterate to find indent.
            int indent = 1; // Default assumption for child?
             std::function<void(Track&, int)> findIndent = [&](Track& t, int d) {
                 if (&t == sliderTrack) { indent = d; return; }
                 for(auto& c : t.children) findIndent(c, d+1);
             };
             for(auto& t : project->tracks) findIndent(t, 0);

            int sliderW = (int)(width * 0.75);
            int sliderX = (width - sliderW) / 2;

            double val = (double)(clickX - sliderX) / sliderW;
            if (val < 0) val = 0;
            if (val > 1) val = 1;
            
            sliderTrack->gain = val;
            
            // Update Name with new Volume
            std::string& name = sliderTrack->name;
            size_t parenPos = name.rfind("(");
            std::string newVol = wxString::Format("(%d%%)", (int)(val * 100)).ToStdString();
            
            if (parenPos != std::string::npos) {
                // Check if it looks like a volume tag
                if (name.find("%", parenPos) != std::string::npos) {
                     name = name.substr(0, parenPos) + newVol;
                } else {
                     name += " " + newVol;
                }
            } else {
                name += " " + newVol;
            }
            
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
             if (timelineView) {
                 timelineView->Refresh();
                 timelineView->UpdateVirtualSize();
             }
        }
        
        isDraggingSlider = false;
        sliderTrack = nullptr;
        isDraggingTrack = false;
        dragSourceTrack = nullptr;
        
        if (HasCapture())
        {
            ReleaseMouse();
        }
        
        SetCursor(wxCursor(wxCURSOR_ARROW));
        Refresh();
    }
}

void TrackList::OnCaptureLost(wxMouseCaptureLostEvent& evt)
{
    isDraggingSlider = false;
    sliderTrack = nullptr;
    isDraggingTrack = false;
    dragSourceTrack = nullptr;
    SetCursor(wxCursor(wxCURSOR_ARROW));
    Refresh();
    if (timelineView) 
    {
        timelineView->Refresh();
    }
}

void TrackList::OnSize(wxSizeEvent& evt)
{
    Refresh();
    evt.Skip();
}
