#include "TransportPanel.h"
#include "../audio/AudioEngine.h"
#include "TimelineView.h"
#include "../model/SampleTypes.h"
#include <wx/graphics.h>
#include <wx/dcbuffer.h>
#include <wx/dcmemory.h>
#include <wx/settings.h>
#include <wx/file.h>
#include <wx/statline.h>

wxBEGIN_EVENT_TABLE(TransportPanel, wxPanel)
    EVT_BUTTON(1001, TransportPanel::OnPlay)
    EVT_BUTTON(1002, TransportPanel::OnStop)
    EVT_TOGGLEBUTTON(1003, TransportPanel::OnLoop)
    EVT_TOGGLEBUTTON(2001, TransportPanel::OnToolSelect)
    EVT_TOGGLEBUTTON(2002, TransportPanel::OnToolDraw)
    // EVT_TOGGLEBUTTON(2003, TransportPanel::OnToolErase) removed
    EVT_CHOICE(3001, TransportPanel::OnSnapChange)
    EVT_CHOICE(3002, TransportPanel::OnDefaultBankChange)
wxEND_EVENT_TABLE()

TransportPanel::TransportPanel(wxWindow* parent, AudioEngine* engine, TimelineView* timeline)
    : wxPanel(parent, wxID_ANY), audioEngine(engine), timelineView(timeline)
{
    // Main horizontal sizer for the entire panel
    wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Define Colors
    wxColor colPlay(0, 180, 0);  // Green
    wxColor colStop(180, 0, 0);  // Red
    wxColor colBlack(*wxBLACK);

    // === LEFT COLUMN: Transport buttons (top) + Tools (bottom) ===
    wxBoxSizer* leftColumnSizer = new wxBoxSizer(wxVERTICAL);
    
    // Transport buttons row
    wxBoxSizer* transportRowSizer = new wxBoxSizer(wxHORIZONTAL);
    
    btnPlay = new wxBitmapButton(this, 1001, wxBitmap(), wxDefaultPosition, wxSize(40, 40));
    btnPlay->SetBitmap(LoadIcon("play.svg", colPlay));
    btnPlay->SetToolTip("Play/Pause");
    
    btnStop = new wxBitmapButton(this, 1002, wxBitmap(), wxDefaultPosition, wxSize(40, 40));
    btnStop->SetBitmap(LoadIcon("stop.svg", colStop));
    btnStop->SetToolTip("Stop");
    
    btnLoop = new wxToggleButton(this, 1003, "", wxDefaultPosition, wxSize(40, 40));
    btnLoop->SetBitmap(LoadIcon("loop.svg", colBlack));
    btnLoop->SetToolTip("Loop");
    
    transportRowSizer->Add(btnPlay, 0, wxRIGHT, 2);
    transportRowSizer->Add(btnStop, 0, wxRIGHT, 2);
    transportRowSizer->Add(btnLoop, 0, 0);
    
    leftColumnSizer->Add(transportRowSizer, 0, wxBOTTOM, 2);
    
    // Tools row - directly below transport with minimal gap
    wxBoxSizer* toolsRowSizer = new wxBoxSizer(wxHORIZONTAL);
    
    wxStaticText* lblTools = new wxStaticText(this, wxID_ANY, "Tools:");
    toolsRowSizer->Add(lblTools, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    
    btnToolSelect = new wxToggleButton(this, 2001, "", wxDefaultPosition, wxSize(30, 30));
    btnToolSelect->SetBitmap(LoadIcon("select.svg", colBlack));
    btnToolSelect->SetToolTip("Select Tool");
    
    btnToolDraw = new wxToggleButton(this, 2002, "", wxDefaultPosition, wxSize(30, 30));
    btnToolDraw->SetBitmap(LoadIcon("pencil.svg", colBlack)); 
    btnToolDraw->SetToolTip("Draw Tool");
    
    btnToolSelect->SetValue(true);
    
    toolsRowSizer->Add(btnToolSelect, 0, wxRIGHT, 2);
    toolsRowSizer->Add(btnToolDraw, 0, 0);
    
    leftColumnSizer->Add(toolsRowSizer, 0, 0);
    
    mainSizer->Add(leftColumnSizer, 0, wxALL, 4);
    
    mainSizer->AddSpacer(10);
    
    // === TIME DISPLAY - Vertically centered ===
    lblTime = new wxStaticText(this, wxID_ANY, "00:00:000");
    wxFont timeFont = lblTime->GetFont();
    timeFont.SetPointSize(14);
    timeFont.SetWeight(wxFONTWEIGHT_BOLD);
    lblTime->SetFont(timeFont);
    mainSizer->Add(lblTime, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    
    // === VERTICAL DIVIDER after time ===
    wxStaticLine* divider = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxSize(1, 55), wxLI_VERTICAL);
    mainSizer->Add(divider, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    
    // === Bank/Snap Dropdowns - Stacked Vertically, centered ===
    wxFlexGridSizer* dropdownGrid = new wxFlexGridSizer(2, 2, 2, 5); // 2 rows, 2 cols, 2px vgap, 5px hgap
    
    // Bank Row
    wxStaticText* lblBank = new wxStaticText(this, wxID_ANY, "Bank:");
    wxArrayString bankChoices;
    bankChoices.Add("None");
    bankChoices.Add("Normal");
    bankChoices.Add("Soft");
    bankChoices.Add("Drum");
    defaultBankChoice = new wxChoice(this, 3002, wxDefaultPosition, wxSize(70, -1), bankChoices);
    defaultBankChoice->SetSelection(0);
    defaultBankChoice->SetToolTip("Auto-place HitNormal when placing additions");
    
    dropdownGrid->Add(lblBank, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    dropdownGrid->Add(defaultBankChoice, 0, wxALIGN_CENTER_VERTICAL);
    
    // Snap Row
    wxStaticText* lblSnap = new wxStaticText(this, wxID_ANY, "Snap:");
    wxArrayString choices;
    choices.Add("1/1");
    choices.Add("1/2");
    choices.Add("1/3");
    choices.Add("1/4");
    choices.Add("1/6");
    choices.Add("1/8");
    choices.Add("1/12");
    choices.Add("1/16");
    snapChoice = new wxChoice(this, 3001, wxDefaultPosition, wxSize(70, -1), choices);
    snapChoice->SetSelection(3); // Default 1/4
    
    dropdownGrid->Add(lblSnap, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    dropdownGrid->Add(snapChoice, 0, wxALIGN_CENTER_VERTICAL);
    
    mainSizer->Add(dropdownGrid, 0, wxALIGN_CENTER_VERTICAL);
    
    mainSizer->AddStretchSpacer(1);
    
    // === ZOOM - Bottom right corner ===
    // Wrap in a vertical sizer to push to bottom
    wxBoxSizer* zoomSizer = new wxBoxSizer(wxVERTICAL);
    zoomSizer->AddStretchSpacer(1);
    
    lblZoom = new wxStaticText(this, wxID_ANY, "100%");
    lblZoom->SetForegroundColour(wxColour(128, 128, 128)); // Gray
    zoomSizer->Add(lblZoom, 0, wxALIGN_RIGHT);
    
    mainSizer->Add(zoomSizer, 0, wxEXPAND | wxALL, 6);
    
    SetSizer(mainSizer);
}




void TransportPanel::OnLoop(wxCommandEvent& evt)
{
    if (audioEngine)
    {
        bool loop = btnLoop->GetValue();
        audioEngine->SetLooping(loop);
        
        // Update Icon Color
        // Update Icon Color
        btnLoop->SetBitmap(LoadIcon("loop.svg", loop ? wxColor(0, 180, 0) : *wxBLACK));
    }
}

void TransportPanel::UpdatePlayButton()
{
    bool isPlaying = audioEngine && audioEngine->IsPlaying();
    wxString filename = isPlaying ? "pause.svg" : "play.svg";
    btnPlay->SetBitmap(LoadIcon(filename.ToStdString(), isPlaying ? wxColor(0, 0, 180) : wxColor(0, 180, 0)));
}

void TransportPanel::UpdateTime(double time)
{
    // Format: MM:SS:ms
    int ms = (int)(time * 1000.0);
    int m = (ms / 1000) / 60;
    int s = (ms / 1000) % 60;
    int mill = ms % 1000;
    
    lblTime->SetLabel(wxString::Format("%02d:%02d:%03d", m, s, mill));
}

void TransportPanel::SetZoomLevel(double pixelsPerSecond)
{
    // 100 pixels/second is baseline (100%)
    int zoomPercent = static_cast<int>((pixelsPerSecond / 100.0) * 100.0);
    lblZoom->SetLabel(wxString::Format("%d%%", zoomPercent));
}

void TransportPanel::OnPlay(wxCommandEvent& evt)
{
    if (audioEngine)
    {
        if (audioEngine->IsPlaying())
        {
            // Pause behavior: Stop but don't reset position?
            // Since we don't have explicit Pause, we'll Stop. 
            // Position is preserved by AudioEngine unless SetPosition(0) called.
            audioEngine->Stop();
        }
        else
        {
            audioEngine->Start();
        }
        UpdatePlayButton();
    }
}

void TransportPanel::OnStop(wxCommandEvent& evt)
{
    if (audioEngine)
    {
        audioEngine->Stop();
        audioEngine->SetPosition(0.0);
        UpdatePlayButton();
    }
}



void TransportPanel::OnToolSelect(wxCommandEvent& evt)
{
    btnToolSelect->SetValue(true);
    btnToolDraw->SetValue(false);
    // btnToolErase->SetValue(false);
    if (timelineView) timelineView->SetTool(ToolType::Select);
}

void TransportPanel::OnToolDraw(wxCommandEvent& evt)
{
    btnToolSelect->SetValue(false);
    btnToolDraw->SetValue(true);
    // btnToolErase->SetValue(false);
    if (timelineView) timelineView->SetTool(ToolType::Draw);
}

// OnToolErase removed

void TransportPanel::OnSnapChange(wxCommandEvent& evt)
{
    if (!timelineView) return;
    
    int sel = snapChoice->GetSelection();
    int divisor = 4;
    switch (sel)
    {
        case 0: divisor = 1; break;
        case 1: divisor = 2; break;
        case 2: divisor = 3; break;
        case 3: divisor = 4; break;
        case 4: divisor = 6; break;
        case 5: divisor = 8; break;
        case 6: divisor = 12; break;
        case 7: divisor = 16; break;
        default: divisor = 4; break;
    }
    
    timelineView->SetGridDivisor(divisor);
}

void TransportPanel::OnDefaultBankChange(wxCommandEvent& evt)
{
    if (!timelineView) return;
    
    int sel = defaultBankChoice->GetSelection();
    
    if (sel == 0) {
        // "None" - disable auto-hitnormal
        timelineView->SetDefaultHitnormalBank(std::nullopt);
    } else {
        // 1=Normal, 2=Soft, 3=Drum
        SampleSet bank;
        switch (sel) {
            case 1: bank = SampleSet::Normal; break;
            case 2: bank = SampleSet::Soft; break;
            case 3: bank = SampleSet::Drum; break;
            default: bank = SampleSet::Normal; break;
        }
        timelineView->SetDefaultHitnormalBank(bank);
    }
}

wxBitmap TransportPanel::LoadIcon(const std::string& filename, const wxColor& color)
{
    wxSize iconSize(18, 18);
    
    wxString path = "Resources/icon/" + filename;
    if (!wxFileExists(path)) {
            if (wxFileExists("../Resources/icon/" + filename)) path = "../Resources/icon/" + filename;
            else if (wxFileExists("../../Resources/icon/" + filename)) path = "../../Resources/icon/" + filename;
    }
    
    if (!wxFileExists(path)) return wxBitmap();

    wxFile file(path);
    wxString svgContent;
    if (file.IsOpened()) file.ReadAll(&svgContent);
    
    wxString hex = color.GetAsString(wxC2S_HTML_SYNTAX);
    svgContent.Replace("#000000", hex);
    svgContent.Replace("black", hex);
    
    wxBitmapBundle bundle = wxBitmapBundle::FromSVG(svgContent.ToStdString().c_str(), iconSize);
    return bundle.GetBitmap(iconSize);
}

void TransportPanel::TogglePlayback()
{
    wxCommandEvent evt;
    OnPlay(evt);
}

void TransportPanel::Stop()
{
    wxCommandEvent evt;
    OnStop(evt);
}
