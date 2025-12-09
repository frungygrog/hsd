#include "TrackList.h"
#include "../model/Project.h"
#include "../Constants.h"
#include "AddTrackDialog.h"
#include "AddGroupingDialog.h"
#include "TimelineView.h" 
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <cmath>
#include <algorithm>
#include <functional>
#include <wx/bmpbndl.h>
#include <wx/filename.h>
#include <wx/file.h>

wxBEGIN_EVENT_TABLE(TrackList, wxPanel)
    EVT_PAINT(TrackList::OnPaint)
    EVT_SIZE(TrackList::OnSize)
    EVT_MOUSE_EVENTS(TrackList::OnMouseEvents)
    EVT_CONTEXT_MENU(TrackList::OnContextMenu)
    EVT_MOUSE_CAPTURE_LOST(TrackList::OnCaptureLost)
wxEND_EVENT_TABLE()


namespace {
    constexpr int kRulerHeight = TrackLayout::RulerHeight;
    constexpr int kMasterTrackHeight = TrackLayout::MasterTrackHeight;
    constexpr int kHeaderHeight = TrackLayout::HeaderHeight;
    constexpr int kParentTrackHeight = TrackLayout::ParentTrackHeight;
    constexpr int kChildTrackHeight = TrackLayout::ChildTrackHeight;
}


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





std::pair<std::string, bool> TrackList::GetAbbreviation(SampleSet s, SampleType t)
{
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
}

void TrackList::UpdateTrackNameWithVolume(Track& track, double volume)
{
    std::string& name = track.name;
    size_t parenPos = name.rfind("(");
    std::string newVol = wxString::Format("(%d%%)", (int)(volume * 100)).ToStdString();
    
    if (parenPos != std::string::npos) {
        if (name.find("%", parenPos) != std::string::npos) {
            name = name.substr(0, parenPos) + newVol;
        } else {
            name += " " + newVol;
        }
    } else {
        name += " " + newVol;
    }
}





TrackList::TrackList(wxWindow* parent)
    : wxPanel(parent)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(wxColour(40, 40, 40));
    
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
    
    int height = kHeaderHeight;
    
    std::function<void(const Track&)> calcHeight = [&](const Track& t) {
        height += t.isChildTrack ? kChildTrackHeight : kParentTrackHeight;
        if (t.isExpanded && !t.children.empty()) {
            for (const auto& c : t.children) calcHeight(c);
        }
    };
    
    for (const auto& t : project->tracks) calcHeight(t);
    
    return height;
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
    return track.isChildTrack ? kChildTrackHeight : kParentTrackHeight;
}





Track* TrackList::FindTrackAtY(int y, int& outIndent, Track** outParent)
{
    if (!project) return nullptr;
    
    int curY = kHeaderHeight;
    
    std::function<Track*(Track&, int, Track*)> findTrack = [&](Track& tr, int indent, Track* parent) -> Track* {
        int h = tr.isChildTrack ? kChildTrackHeight : kParentTrackHeight;
        if (y >= curY && y < curY + h) {
            outIndent = indent;
            if (outParent) *outParent = parent;
            return &tr;
        }
        curY += h;
        if (tr.isExpanded) {
            for (auto& c : tr.children) {
                Track* res = findTrack(c, indent + 1, &tr);
                if (res) return res;
            }
        }
        return nullptr;
    };
    
    for (auto& t : project->tracks) {
        Track* res = findTrack(t, 0, nullptr);
        if (res) return res;
    }
    return nullptr;
}





void TrackList::DrawHeader(wxDC& dc, int& y, int width)
{
    wxColour borderCol(100, 100, 100);
    
    
    wxRect rulerRect(0, y, width, kRulerHeight);
    dc.SetBrush(wxBrush(wxColour(50, 50, 50)));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(rulerRect);
    
    wxFont btnFont = dc.GetFont();
    btnFont.SetPointSize(9);
    btnFont.SetWeight(wxFONTWEIGHT_NORMAL);
    dc.SetFont(btnFont);
    dc.SetTextForeground(wxColour(200, 200, 200));
    
    int btnY = y + (kRulerHeight - 16) / 2;
    int curX = 10;
    
    
    if (trackIcon.IsOk()) {
        dc.DrawBitmap(trackIcon, curX, btnY, true);
        curX += 20;
    }
    dc.DrawText("Add Track", curX, btnY);
    curX += dc.GetTextExtent("Add Track").GetWidth() + 15;
    
    
    if (groupingIcon.IsOk()) {
        dc.DrawBitmap(groupingIcon, curX, btnY, true);
        curX += 20;
    }
    dc.DrawText("Add Grouping", curX, btnY);
    
    
    int masterY = y + kRulerHeight;
    wxRect masterRect(0, masterY, width, kMasterTrackHeight);
    
    dc.SetPen(wxPen(borderCol));
    dc.SetBrush(wxBrush(wxColour(45, 45, 60)));
    dc.DrawRectangle(masterRect);
    
    dc.SetTextForeground(*wxWHITE);
    wxFont masterTitleFont = dc.GetFont();
    masterTitleFont.SetWeight(wxFONTWEIGHT_BOLD);
    masterTitleFont.SetPointSize(11);
    dc.SetFont(masterTitleFont);
    
    
    wxString masterTitle = "Master Audio";
    if (project && (!project->artist.empty() || !project->title.empty())) {
        masterTitle = wxString::FromUTF8(project->artist) + " - " + wxString::FromUTF8(project->title);
    }
    
    int maxTitleWidth = width - 30;
    if (dc.GetTextExtent(masterTitle).GetWidth() > maxTitleWidth) {
        while (masterTitle.Length() > 3 && dc.GetTextExtent(masterTitle + "...").GetWidth() > maxTitleWidth) {
            masterTitle = masterTitle.Left(masterTitle.Length() - 1);
        }
        masterTitle += "...";
    }
    
    dc.DrawText(masterTitle, 10, masterY + 15);
    
    
    wxFont infoFont = dc.GetFont();
    infoFont.SetWeight(wxFONTWEIGHT_NORMAL);
    infoFont.SetPointSize(9);
    dc.SetFont(infoFont);
    dc.SetTextForeground(wxColour(200, 200, 200));
    
    double firstOffset = 0.0;
    if (project && !project->timingPoints.empty()) {
        for (const auto& tp : project->timingPoints) {
            if (tp.uninherited) {
                firstOffset = tp.time;
                break;
            }
        }
    }
    
    wxString bpmStr = wxString::Format("%.0f BPM", project ? project->bpm : 0.0);
    wxString offsetStr = wxString::Format("%.0fms", firstOffset);
    
    dc.DrawText(bpmStr, 10, masterY + 45);
    dc.DrawText(offsetStr, 10, masterY + 65);
    
    y += kHeaderHeight;
}

void TrackList::DrawAbbreviation(wxDC& dc, Track& track, int y, int width, int height)
{
    if (track.children.empty()) return;
    
    const Track& source = track.children[0];
    
    struct TextBlob { std::string text; bool bold; };
    std::vector<TextBlob> blobs;
    std::vector<TextBlob> bases;
    std::vector<TextBlob> adds;
    
    auto process = [&](SampleSet s, SampleType t) {
        auto p = GetAbbreviation(s, t);
        if (t == SampleType::HitNormal) bases.push_back({p.first, p.second});
        else adds.push_back({p.first, p.second});
    };
    
    if (!source.layers.empty()) {
        for (auto& l : source.layers) process(l.bank, l.type);
    } else {
        process(source.sampleSet, source.sampleType);
    }
    
    for (size_t i = 0; i < bases.size(); ++i) {
        if (i > 0) blobs.push_back({", ", false});
        blobs.push_back(bases[i]);
    }
    
    if (!bases.empty() && !adds.empty())
        blobs.push_back({" + ", false});
        
    for (size_t i = 0; i < adds.size(); ++i) {
        if (i > 0) blobs.push_back({", ", false});
        blobs.push_back(adds[i]);
    }
    
    wxFont smallFont = dc.GetFont();
    smallFont.SetPointSize(8);
    
    int totalW = 0;
    for (auto& b : blobs) {
        smallFont.SetWeight(b.bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL);
        dc.SetFont(smallFont);
        totalW += dc.GetTextExtent(b.text).GetWidth();
    }
    
    int marginX = 10;
    int marginY = 5;
    int startX = width - marginX - totalW;
    int drawY = y + height - marginY - 12;
    
    dc.SetTextForeground(wxColour(100, 100, 100));
    
    int curX = startX;
    for (auto& b : blobs) {
        smallFont.SetWeight(b.bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL);
        dc.SetFont(smallFont);
        dc.DrawText(b.text, curX, drawY);
        curX += dc.GetTextExtent(b.text).GetWidth();
    }
}

void TrackList::DrawSlider(wxDC& dc, Track& track, int y, int width)
{
    int btnY = 20;
    int sliderW = (int)(width * 0.75);
    int sliderX = (width - sliderW) / 2;
    int sliderH = 14;
    int midY = y + btnY + sliderH / 2;
    
    
    dc.SetPen(wxPen(wxColour(160, 160, 160), 2)); 
    dc.DrawLine(sliderX, midY, sliderX + sliderW, midY);
    
    
    dc.SetPen(wxPen(wxColour(160, 160, 160), 1));
    int tickH = 4;
    float ticks[] = {0.0f, 0.2f, 0.4f, 0.5f, 0.6f, 0.8f, 1.0f};
    for (float t : ticks) {
        int tx = sliderX + (int)(t * sliderW);
        dc.DrawLine(tx, midY - tickH, tx, midY + tickH);
    }
    
    
    int thumbW = 10;
    int thumbH = 14;
    int thumbX = sliderX + (int)(track.gain * sliderW) - thumbW / 2;
    
    wxRect thumbRect(thumbX, midY - thumbH/2, thumbW, thumbH);
    dc.SetBrush(wxBrush(wxColour(230, 230, 230)));
    dc.SetPen(wxPen(wxColour(100, 100, 100)));
    dc.DrawRoundedRectangle(thumbRect, 2);
}

void TrackList::DrawPrimarySelector(wxDC& dc, Track& track, int y, int width)
{
    if (track.children.empty()) return;
    
    int pH = 20; 
    int pW = 20;
    int valW = 50;
    int gap = 2;
    int rightMargin = 10;
    
    int valX = width - rightMargin - valW;
    int pX = valX - gap - pW;
    int pY = y + 25;
    
    
    wxRect pLabelRect(pX, pY, pW, pH);
    dc.SetBrush(wxBrush(wxColour(60, 60, 60)));
    dc.SetPen(wxPen(wxColour(80, 80, 80)));
    dc.DrawRoundedRectangle(pLabelRect, 2);
    
    wxFont fontP = dc.GetFont();
    fontP.SetWeight(wxFONTWEIGHT_NORMAL);
    fontP.SetPointSize(9);
    dc.SetFont(fontP);
    dc.SetTextForeground(*wxWHITE);
    wxSize tz = dc.GetTextExtent("P");
    dc.DrawText("P", pLabelRect.x + (pLabelRect.width - tz.x)/2, pLabelRect.y + (pLabelRect.height - tz.y)/2);
    
    
    wxRect valRect(valX, pY, valW, pH);
    dc.SetBrush(wxBrush(wxColour(220, 220, 220)));
    dc.SetPen(wxPen(wxColour(100, 100, 100)));
    dc.DrawRoundedRectangle(valRect, 2);
    
    wxString valStr = "---";
    if (track.primaryChildIndex >= 0 && track.primaryChildIndex < (int)track.children.size()) {
        int g = (int)(track.children[track.primaryChildIndex].gain * 100.0f);
        valStr = wxString::Format("%d%%", g);
    }
    
    dc.SetTextForeground(*wxBLACK);
    tz = dc.GetTextExtent(valStr);
    dc.DrawText(valStr, valRect.x + (valRect.width - tz.x)/2, valRect.y + (valRect.height - tz.y)/2);
    
    
    wxPoint tri[] = {
        wxPoint(valRect.GetRight() - 5, valRect.GetBottom() - 5),
        wxPoint(valRect.GetRight() - 2, valRect.GetBottom() - 5),
        wxPoint(valRect.GetRight() - 3, valRect.GetBottom() - 3)
    };
    dc.SetBrush(*wxBLACK_BRUSH);
    dc.DrawPolygon(3, tri);
}

void TrackList::DrawTrackControls(wxDC& dc, Track& track, int y, int width, int indent)
{
    if (indent == 0) {
        
        wxRect muteRect(5, y + 25, 30, 20);
        dc.SetBrush(track.mute ? wxBrush(wxColour(100, 100, 200)) : wxBrush(wxColour(200, 200, 200)));
        dc.SetPen(wxPen(wxColour(80, 80, 80)));
        dc.DrawRoundedRectangle(muteRect, 2);
        
        wxFont btnF = dc.GetFont();
        btnF.SetWeight(wxFONTWEIGHT_BOLD);
        btnF.SetPointSize(9);
        dc.SetFont(btnF);
        dc.SetTextForeground(*wxBLACK);
        
        wxSize tz = dc.GetTextExtent("M");
        dc.DrawText("M", muteRect.x + (muteRect.width - tz.x)/2, muteRect.y + (muteRect.height - tz.y)/2);
        
        
        wxRect soloRect(5, y + 50, 30, 20);
        dc.SetBrush(track.solo ? wxBrush(wxColour(100, 200, 100)) : wxBrush(wxColour(200, 200, 200)));
        dc.DrawRoundedRectangle(soloRect, 2);
        
        tz = dc.GetTextExtent("S");
        dc.DrawText("S", soloRect.x + (soloRect.width - tz.x)/2, soloRect.y + (soloRect.height - tz.y)/2);
        
        
        DrawPrimarySelector(dc, track, y, width);
        
        
        DrawAbbreviation(dc, track, y, width, kParentTrackHeight);
    } else {
        
        DrawSlider(dc, track, y, width);
    }
}

void TrackList::DrawTrackRow(wxDC& dc, Track& track, int& y, int width, int indent)
{
    int currentHeight = (indent == 0) ? kParentTrackHeight : kChildTrackHeight;
    
    wxColour borderCol(100, 100, 100);
    wxColour panelCol(192, 192, 192);
    
    
    wxRect trackRect(0, y, width, currentHeight);
    dc.SetPen(wxPen(borderCol));
    dc.SetBrush(wxBrush(panelCol));
    dc.DrawRectangle(trackRect);
    
    
    if (isDraggingTrack && &track == dragSourceTrack) {
        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::CreateFromUnknownDC(dc));
        if (gc) {
            gc->SetBrush(wxBrush(wxColour(0, 120, 255, 80)));
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->DrawRectangle(trackRect.x, trackRect.y, trackRect.width, trackRect.height);
        }
    }
    
    
    int titleH = (indent == 0) ? 20 : 15;
    wxRect titleRect(trackRect.GetX(), trackRect.GetY(), width, titleH);
    dc.SetBrush(wxBrush(wxColour(220, 220, 220))); 
    dc.DrawRectangle(titleRect);
    
    
    if (!track.children.empty()) {
        wxRect expanderRect(indent * 10 + 2, y + 2, 16, 16);
        dc.SetBrush(*wxWHITE_BRUSH);
        dc.DrawRectangle(expanderRect);
        dc.SetBrush(*wxBLACK_BRUSH);
        
        dc.DrawLine(expanderRect.GetX() + 3, expanderRect.GetY() + 8, expanderRect.GetRight() - 3, expanderRect.GetY() + 8);
        if (!track.isExpanded)
            dc.DrawLine(expanderRect.GetX() + 8, expanderRect.GetY() + 3, expanderRect.GetX() + 8, expanderRect.GetBottom() - 3);
    }

    
    dc.SetTextForeground(*wxBLACK);
    int nameX = 25;

    if (indent > 0) {
        wxFont f = dc.GetFont();
        f.SetPointSize(8);
        dc.SetFont(f);
        
        
        dc.SetBrush(wxBrush(wxColour(100, 100, 100)));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawCircle(12, y + 8, 3);
    } else {
        wxFont f = dc.GetFont();
        f.SetPointSize(10);
        f.SetWeight(wxFONTWEIGHT_BOLD);
        dc.SetFont(f);
    }
    
    dc.DrawText(track.name, nameX, y + 2);
    
    
    DrawTrackControls(dc, track, y, width, indent);
    
    y += currentHeight;
    
    
    if (track.isExpanded) {
        for (auto& child : track.children)
            DrawTrackRow(dc, child, y, width, indent + 1);
    }
}

void TrackList::DrawDropIndicator(wxDC& dc, int width, int headerHeight)
{
    if (!isDraggingTrack || !currentDropTarget.isValid) return;
    
    int targetY = headerHeight;
    
    if (currentDropTarget.parent == nullptr) {
        
        int curY = headerHeight;
        int pIdx = 0;
        for (auto& t : project->tracks) {
            if (pIdx == currentDropTarget.index) {
                targetY = curY;
                break;
            }
            int h = t.isChildTrack ? kChildTrackHeight : kParentTrackHeight;
            curY += h;
            if (t.isExpanded) {
                for (auto& c : t.children) curY += kChildTrackHeight;
            }
            pIdx++;
        }
        if (pIdx == currentDropTarget.index) targetY = curY;
    } else {
        
        int curY = headerHeight;
        for (auto& t : project->tracks) {
            int h = t.isChildTrack ? kChildTrackHeight : kParentTrackHeight;
            if (&t == currentDropTarget.parent) {
                curY += h;
                int cIdx = 0;
                for (auto& c : t.children) {
                    if (cIdx == currentDropTarget.index) {
                        targetY = curY;
                        break;
                    }
                    curY += kChildTrackHeight;
                    cIdx++;
                }
                if (cIdx == currentDropTarget.index) targetY = curY;
                break;
            }
            curY += h;
            if (t.isExpanded) {
                for (auto& c : t.children) curY += kChildTrackHeight;
            }
        }
    }
    
    dc.SetPen(wxPen(wxColour(0, 255, 255), 2));
    dc.DrawLine(0, targetY, width, targetY);
}

void TrackList::OnPaint(wxPaintEvent& evt)
{
    wxAutoBufferedPaintDC dc(this);
    
    wxSize clientSize = GetClientSize();
    dc.SetBrush(wxBrush(wxColour(160, 160, 160)));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(0, 0, clientSize.GetWidth(), clientSize.GetHeight());
    
    if (!project) return;
    
    dc.SetDeviceOrigin(0, -scrollOffsetY);
    
    int y = 0;
    int width = clientSize.GetWidth();
    
    wxFont font = dc.GetFont();
    font.SetWeight(wxFONTWEIGHT_BOLD);
    dc.SetFont(font);
    
    DrawHeader(dc, y, width);
    
    for (auto& track : project->tracks)
        DrawTrackRow(dc, track, y, width, 0);
    
    DrawDropIndicator(dc, width, kHeaderHeight);
}





void TrackList::AddChildToTrack(Track* parent)
{
    if (!parent) return;
    
    if (parent->isGrouping) {
        SampleSet targetSet = SampleSet::Normal;
        SampleType targetType = SampleType::HitNormal;
        
        if (!parent->children.empty()) {
            if (!parent->children[0].layers.empty()) {
                targetSet = parent->children[0].layers[0].bank;
                targetType = parent->children[0].layers[0].type;
            } else {
                targetSet = parent->children[0].sampleSet;
                targetType = parent->children[0].sampleType;
            }
        }
        
        Track child;
        child.name = parent->name + " (50%)";
        child.gain = 0.5f;
        
        if (!parent->children.empty()) {
            child.layers = parent->children[0].layers;
        } else {
            child.layers.push_back({targetSet, targetType});
        }
        
        child.isChildTrack = true;
        parent->children.push_back(child);
        parent->isExpanded = true;
    } else {
        Track child;
        child.sampleSet = parent->sampleSet;
        child.sampleType = parent->sampleType;
        child.gain = 0.5f;
        
        std::string sStr = (child.sampleSet == SampleSet::Normal) ? "normal" : (child.sampleSet == SampleSet::Soft ? "soft" : "drum");
        std::string tStr = (child.sampleType == SampleType::HitNormal) ? "hitnormal" : 
                          (child.sampleType == SampleType::HitWhistle) ? "hitwhistle" :
                          (child.sampleType == SampleType::HitFinish) ? "hitfinish" : "hitclap";
        
        child.name = sStr + "-" + tStr + " (50%)";
        child.isChildTrack = true;
        parent->children.push_back(child);
        parent->isExpanded = true;
    }
    
    if (timelineView) timelineView->UpdateVirtualSize();
    Refresh();
    if (GetParent()) GetParent()->Refresh();
}

void TrackList::EditTrack(Track* track)
{
    if (!track) return;
    
    if (track->isGrouping) {
        AddGroupingDialog dlg(this);
        dlg.SetEditMode(true);
        
        wxString name = track->name;
        SampleSet normalBank = SampleSet::Normal;
        SampleSet additionsBank = SampleSet::Soft;
        bool hasWhistle = false, hasFinish = false, hasClap = false;
        int volume = 100;
        
        if (!track->children.empty()) {
            auto& firstChild = track->children[0];
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
        
        if (dlg.ShowModal() == wxID_OK) {
            auto res = dlg.GetResult();
            if (res.confirmed) {
                track->name = res.name.ToStdString();
                
                if (!track->children.empty()) {
                    auto& child = track->children[0];
                    child.gain = (float)res.volume / 100.0f;
                    
                    std::string volPercent = std::to_string(res.volume) + "%";
                    child.name = res.name.ToStdString() + " (" + volPercent + ")";
                    
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
    } else {
        AddTrackDialog dlg(this);
        dlg.SetEditMode(true);
        
        int editVolume = 100;
        if (!track->children.empty()) {
            editVolume = static_cast<int>(track->children[0].gain * 100);
        }
        dlg.SetValues(track->name, track->sampleSet, track->sampleType, editVolume);
        
        if (dlg.ShowModal() == wxID_OK) {
            auto res = dlg.GetResult();
            if (res.confirmed) {
                track->name = res.name.ToStdString();
                track->sampleSet = res.bank;
                track->sampleType = res.type;
                
                Refresh();
                if (GetParent()) GetParent()->Refresh();
            }
        }
    }
}

void TrackList::DeleteTrack(Track* track, Track* parent)
{
    if (!track) return;
    
    if (parent) {
        
        int childIdx = -1;
        for (size_t i = 0; i < parent->children.size(); ++i) {
            if (&parent->children[i] == track) {
                childIdx = (int)i;
                break;
            }
        }
        
        if (childIdx != -1) {
            if (parent->primaryChildIndex == childIdx) {
                parent->primaryChildIndex = 0;
            } else if (parent->primaryChildIndex > childIdx) {
                parent->primaryChildIndex--;
            }
            
            parent->children.erase(parent->children.begin() + childIdx);
            
            if (!parent->children.empty() && parent->primaryChildIndex >= (int)parent->children.size()) {
                parent->primaryChildIndex = (int)parent->children.size() - 1;
            } else if (parent->children.empty()) {
                parent->primaryChildIndex = 0;
            }
        }
    } else {
        
        for (auto it = project->tracks.begin(); it != project->tracks.end(); ++it) {
            if (&(*it) == track) {
                project->tracks.erase(it);
                break;
            }
        }
    }
    
    Refresh();
    if (GetParent()) GetParent()->Refresh();
    if (timelineView) timelineView->UpdateVirtualSize();
}





void TrackList::ShowParentContextMenu(Track* track)
{
    if (!track) return;
    
    wxMenu menu;
    enum { ID_ADD_CHILD = 20001, ID_DELETE = 20003, ID_EDIT = 20004 };
    
    menu.Append(ID_ADD_CHILD, "Add Child");
    menu.Append(ID_EDIT, "Edit");
    menu.Append(ID_DELETE, "Delete");
    
    int sel = GetPopupMenuSelectionFromUser(menu);
    
    if (sel == ID_DELETE) {
        DeleteTrack(track, nullptr);
    } else if (sel == ID_EDIT) {
        EditTrack(track);
    } else if (sel == ID_ADD_CHILD) {
        AddChildToTrack(track);
    }
}

void TrackList::ShowChildContextMenu(Track* track)
{
    if (!track) return;
    
    
    Track* parent = nullptr;
    for (auto& t : project->tracks) {
        for (size_t i = 0; i < t.children.size(); ++i) {
            if (&t.children[i] == track) {
                parent = &t;
                break;
            }
        }
        if (parent) break;
    }
    
    wxMenu menu;
    menu.Append(20002, "Delete Child");
    
    int sel = GetPopupMenuSelectionFromUser(menu);
    if (sel == 20002) {
        DeleteTrack(track, parent);
    }
}

void TrackList::OnContextMenu(wxContextMenuEvent& evt)
{
    if (!project) return;
    
    wxPoint mousePos = evt.GetPosition();
    if (mousePos.x == -1 && mousePos.y == -1) 
        mousePos = ClientToScreen(wxPoint(0, 0));
        
    wxPoint clientPos = ScreenToClient(mousePos);
    int y = clientPos.y + scrollOffsetY;
    
    if (y < kHeaderHeight) return;
    
    int indent = 0;
    Track* parent = nullptr;
    Track* track = FindTrackAtY(y, indent, &parent);
    
    if (!track) return;
    
    
    bool isRoot = false;
    for (auto& t : project->tracks) {
        if (&t == track) { isRoot = true; break; }
    }
    
    if (isRoot) {
        ShowParentContextMenu(track);
    } else {
        ShowChildContextMenu(track);
    }
}





bool TrackList::HandleHeaderClick(int clickX, int clickY)
{
    if (clickY >= kRulerHeight) return false;
    
    int btnY = (kRulerHeight - 16) / 2;
    int curX = 10;
    
    wxClientDC dc(this);
    wxFont btnFont = dc.GetFont();
    btnFont.SetPointSize(9); 
    dc.SetFont(btnFont);
    
    int w1 = 20 + dc.GetTextExtent("Add Track").GetWidth();
    
    if (clickX >= curX && clickX <= curX + w1) {
        AddTrackDialog dlg(this);
        if (dlg.ShowModal() == wxID_OK) {
            AddTrackResult res = dlg.GetResult();
            if (res.confirmed) {
                std::string setStr = (res.bank == SampleSet::Normal) ? "normal" : (res.bank == SampleSet::Soft ? "soft" : "drum");
                std::string typeStr = (res.type == SampleType::HitNormal) ? "hitnormal" :
                                     (res.type == SampleType::HitWhistle) ? "hitwhistle" :
                                     (res.type == SampleType::HitFinish) ? "hitfinish" : "hitclap";
                std::string parentName = setStr + "-" + typeStr;
                
                Track* parent = nullptr;
                for (auto& t : project->tracks) {
                    if (t.name == parentName) { parent = &t; break; }
                }
                
                if (!parent) {
                    Track newParent;
                    newParent.name = parentName;
                    newParent.sampleSet = res.bank;
                    newParent.sampleType = res.type;
                    newParent.isExpanded = false;
                    project->tracks.push_back(newParent);
                    parent = &project->tracks.back();
                }
                
                int vol = res.volume;
                std::string childName = parentName + " (" + std::to_string(vol) + "%)";
                
                bool childExists = false;
                for (auto& c : parent->children) {
                    if (c.name == childName) { childExists = true; break; }
                }
                
                if (!childExists) {
                    Track child;
                    child.name = childName;
                    child.sampleSet = res.bank;
                    child.sampleType = res.type;
                    child.gain = (float)vol / 100.0f;
                    child.isChildTrack = true;
                    parent->children.push_back(child);
                }
                
                Refresh();
                if (timelineView) timelineView->UpdateVirtualSize();
                if (GetParent()) GetParent()->Refresh();
            }
        }
        return true;
    }
    
    curX += w1 + 15;
    
    int w2 = 20 + dc.GetTextExtent("Add Grouping").GetWidth();
    if (clickX >= curX && clickX <= curX + w2) {
        AddGroupingDialog dlg(this);
        if (dlg.ShowModal() == wxID_OK) {
            auto res = dlg.GetResult();
            if (res.confirmed) {
                Track grouping;
                grouping.name = res.name.ToStdString();
                grouping.isGrouping = true;
                grouping.isExpanded = false;
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
                child.isChildTrack = true;
                
                grouping.children.push_back(child);
                project->tracks.push_back(grouping);
                
                Refresh();
                if (timelineView) timelineView->UpdateVirtualSize();
                if (GetParent()) GetParent()->Refresh();
            }
        }
        return true;
    }
    
    return false;
}





bool TrackList::HandleTrackClick(Track& track, int clickX, int localY, int y, int width, int indent)
{
    
    if (!track.children.empty()) {
        wxRect expanderRect(indent * 10 + 2, 2, 16, 16);
        if (clickX >= expanderRect.GetX() && clickX <= expanderRect.GetRight() &&
            localY >= expanderRect.GetY() && localY <= expanderRect.GetBottom()) {
            track.isExpanded = !track.isExpanded;
            Refresh();
            if (timelineView) timelineView->UpdateVirtualSize();
            if (GetParent()) GetParent()->Refresh();
            return true;
        }
    }
    
    
    if (indent == 0) {
        wxRect muteRect(5, 25, 30, 20); 
        wxRect soloRect(5, 50, 30, 20);
        
        if (clickX >= muteRect.x && clickX <= muteRect.GetRight() &&
            localY >= muteRect.y && localY <= muteRect.GetBottom()) {
            track.mute = !track.mute;
            Refresh();
            return true;
        }
        if (clickX >= soloRect.x && clickX <= soloRect.GetRight() &&
            localY >= soloRect.y && localY <= soloRect.GetBottom()) {
            track.solo = !track.solo;
            Refresh();
            return true;
        }
    }
    
    
    if (indent > 0) {
        int btnY = 20;
        int sliderW = (int)(width * 0.75);
        int sliderX = (width - sliderW) / 2;
        int sliderH = 14;
        
        if (localY >= btnY - 5 && localY <= btnY + sliderH + 5 &&
            clickX >= sliderX - 10 && clickX <= sliderX + sliderW + 10) {
            isDraggingSlider = true;
            sliderTrack = &track;
            
            double val = (double)(clickX - sliderX) / sliderW;
            if (val < 0) val = 0;
            if (val > 1) val = 1;
            sliderTrack->gain = val;
            
            UpdateTrackNameWithVolume(*sliderTrack, val);
            Refresh();
            return true;
        }
    }
    
    
    if (indent == 0 && !track.children.empty()) {
        int pH = 20; 
        int valW = 50;
        int rightMargin = 10;
        int valX = width - rightMargin - valW;
        int btnY = 25;
        
        if (clickX >= valX && clickX <= valX + valW &&
            localY >= btnY && localY <= btnY + pH) {
            wxMenu menu;
            for (int i = 0; i < (int)track.children.size(); ++i) {
                wxString lbl = track.children[i].name;
                menu.AppendRadioItem(10000 + i, lbl);
                if (i == track.primaryChildIndex) menu.Check(10000 + i, true);
            }
            
            int selectedId = GetPopupMenuSelectionFromUser(menu);
            if (selectedId >= 10000) {
                int idx = selectedId - 10000;
                if (idx >= 0 && idx < (int)track.children.size()) {
                    if (idx > 0) {
                        Track child = std::move(track.children[idx]);
                        track.children.erase(track.children.begin() + idx);
                        track.children.insert(track.children.begin(), std::move(child));
                    }
                    track.primaryChildIndex = 0;
                }
                Refresh();
                if (GetParent()) GetParent()->Refresh();
            }
            return true;
        }
    }
    
    return false;
}





void TrackList::OnMouseEvents(wxMouseEvent& evt)
{
    if (evt.LeftDown()) {
        if (!project) return;
        
        int clickX = evt.GetX();
        int clickY = evt.GetY() + scrollOffsetY;
        int width = GetClientSize().GetWidth();
        
        
        if (clickY < kHeaderHeight) {
            if (HandleHeaderClick(clickX, clickY)) return;
            return;
        }
        
        
        int y = kHeaderHeight;
        bool handled = false;
        
        std::function<void(Track&, int)> hitTest = [&](Track& track, int indent) {
            if (handled) return;
            
            int currentHeight = (indent == 0) ? kParentTrackHeight : kChildTrackHeight;
            
            if (clickY >= y && clickY < y + currentHeight) {
                int localY = clickY - y;
                handled = HandleTrackClick(track, clickX, localY, y, width, indent);
            }
            
            y += currentHeight;
            
            if (track.isExpanded) {
                for (auto& child : track.children) {
                    hitTest(child, indent + 1);
                    if (handled) return;
                }
            }
        };

        for (auto& t : project->tracks) {
            hitTest(t, 0);
            if (handled) break;
        }
        
        
        if (!handled) {
            int indent = 0;
            Track* hit = FindTrackAtY(clickY, indent);
            
            if (hit) {
                dragSourceTrack = hit;
                isDraggingTrack = true;
                CaptureMouse();
            }
        }
    }
    else if (evt.RightDown()) {
        if (!project) return;
        
        int clickY = evt.GetY() + scrollOffsetY;
        
        int indent = 0;
        Track* hit = FindTrackAtY(clickY, indent);
        
        if (hit) {
            bool isRoot = false;
            for (auto& t : project->tracks) {
                if (&t == hit) { isRoot = true; break; }
            }
            
            if (isRoot) {
                ShowParentContextMenu(hit);
            } else {
                ShowChildContextMenu(hit);
            }
        }
    }
    else if (evt.Dragging()) {
        if (isDraggingSlider && sliderTrack) {
            int clickX = evt.GetX();
            int width = GetClientSize().GetWidth();
            
            int sliderW = (int)(width * 0.75);
            int sliderX = (width - sliderW) / 2;

            double val = (double)(clickX - sliderX) / sliderW;
            if (val < 0) val = 0;
            if (val > 1) val = 1;
            
            sliderTrack->gain = val;
            UpdateTrackNameWithVolume(*sliderTrack, val);
            Refresh();
        }
        else if (!isDraggingSlider && dragSourceTrack) {
            int y = evt.GetY() + scrollOffsetY;
            if (!isDraggingTrack) {
                if (std::abs(y - dragSourceY) > 5) {
                    isDraggingTrack = true;
                    SetCursor(wxCursor(wxCURSOR_HAND));
                }
            }
            
            if (isDraggingTrack) {
                dragCurrentY = y;
                
                
                Track* sourceParent = nullptr;
                for (auto& p : project->tracks) {
                    for (auto& c : p.children) {
                        if (&c == dragSourceTrack) { sourceParent = &p; break; }
                    }
                    if (sourceParent) break;
                }
                
                currentDropTarget.isValid = false;
                
                if (sourceParent) {
                    
                    currentDropTarget.parent = sourceParent;
                    
                    int pY = kHeaderHeight;
                    for (auto& t : project->tracks) {
                        if (&t == sourceParent) break;
                        pY += t.isChildTrack ? kChildTrackHeight : kParentTrackHeight;
                        if (t.isExpanded) pY += t.children.size() * kChildTrackHeight;
                    }
                    
                    int childStartY = pY + kParentTrackHeight;
                    int bestIdx = -1;
                    
                    if (dragCurrentY < childStartY) bestIdx = 0;
                    else {
                        for (size_t i = 0; i < sourceParent->children.size(); ++i) {
                            int midY = childStartY + kChildTrackHeight / 2;
                            if (dragCurrentY < midY) { bestIdx = i; break; }
                            childStartY += kChildTrackHeight;
                        }
                        if (bestIdx == -1) bestIdx = sourceParent->children.size();
                    }
                    
                    currentDropTarget.index = bestIdx;
                    currentDropTarget.isValid = true;
                }
                else {
                    
                    currentDropTarget.parent = nullptr;
                    int pY = kHeaderHeight;
                    int bestIdx = -1;
                    
                    for (size_t pIdx = 0; pIdx < project->tracks.size(); ++pIdx) {
                        auto& t = project->tracks[pIdx];
                        int h = kParentTrackHeight;
                        if (t.isExpanded) h += t.children.size() * kChildTrackHeight;
                        
                        if (dragCurrentY < pY + h/2) { bestIdx = pIdx; break; }
                        pY += h;
                    }
                    if (bestIdx == -1) bestIdx = project->tracks.size();
                    
                    currentDropTarget.index = bestIdx;
                    currentDropTarget.isValid = true;
                }
                
                Refresh();
            }
        }
    }
    else if (evt.LeftUp()) {
        if (isDraggingTrack && currentDropTarget.isValid && dragSourceTrack) {
            if (currentDropTarget.parent) {
                
                auto& kids = currentDropTarget.parent->children;
                int srcIdx = -1;
                for (int i = 0; i < (int)kids.size(); ++i)
                    if (&kids[i] == dragSourceTrack) { srcIdx = i; break; }
                
                if (srcIdx != -1) {
                    int destIdx = currentDropTarget.index;
                    if (srcIdx < destIdx) destIdx--;
                    
                    if (srcIdx != destIdx && destIdx >= 0 && destIdx <= (int)kids.size() - 1) {
                        Track temp = std::move(kids[srcIdx]);
                        kids.erase(kids.begin() + srcIdx);
                        kids.insert(kids.begin() + destIdx, std::move(temp));
                        
                        if (destIdx == 0) currentDropTarget.parent->primaryChildIndex = 0;
                    }
                }
            }
            else {
                
                auto& tracks = project->tracks;
                int srcIdx = -1;
                for (int i = 0; i < (int)tracks.size(); ++i)
                    if (&tracks[i] == dragSourceTrack) { srcIdx = i; break; }
                
                if (srcIdx != -1) {
                    int destIdx = currentDropTarget.index;
                    if (srcIdx < destIdx) destIdx--;
                    
                    if (srcIdx != destIdx && destIdx >= 0 && destIdx <= (int)tracks.size() - 1) {
                        Track temp = std::move(tracks[srcIdx]);
                        tracks.erase(tracks.begin() + srcIdx);
                        tracks.insert(tracks.begin() + destIdx, std::move(temp));
                    }
                }
            }
            
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
        
        if (HasCapture()) ReleaseMouse();
        
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
    if (timelineView) timelineView->Refresh();
}

void TrackList::OnSize(wxSizeEvent& evt)
{
    Refresh();
    evt.Skip();
}
