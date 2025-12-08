#include "TransportPanel.h"
#include "../audio/AudioEngine.h"
#include "TimelineView.h"
#include "../model/SampleTypes.h"
#include <wx/graphics.h>
#include <wx/dcbuffer.h>
#include <wx/dcmemory.h>
#include <wx/settings.h>
#include <wx/file.h>

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
    wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // LoadIcon helper moved to member function

    
    // Define Colors
    wxColor colPlay(0, 180, 0);  // Green
    wxColor colPause(0, 0, 180); // Blue
    wxColor colStop(180, 0, 0);  // Red
    wxColor colBlack(*wxBLACK);
    wxColor colActive(0, 120, 215); // Standard highlighting blue? Or Green? 
    // User said "Same with loop for when it's selected" -> imply distinct color.
    // Let's use Green for Loop enabled?
    wxColor colLoop(0, 180, 0);

    // Transport Controls
    // Play - Green
    btnPlay = new wxBitmapButton(this, 1001, wxBitmap(), wxDefaultPosition, wxSize(40, 40));
    btnPlay->SetBitmap(LoadIcon("play.svg", colPlay));
    btnPlay->SetToolTip("Play/Pause");
    
    // Stop - Red (Square)
    btnStop = new wxBitmapButton(this, 1002, wxBitmap(), wxDefaultPosition, wxSize(40, 40));
    btnStop->SetBitmap(LoadIcon("stop.svg", colStop));
    btnStop->SetToolTip("Stop");
    
    // Loop - Default Black
    btnLoop = new wxToggleButton(this, 1003, "", wxDefaultPosition, wxSize(40, 40));
    btnLoop->SetBitmap(LoadIcon("loop.svg", colBlack));
    btnLoop->SetToolTip("Loop");
    
    mainSizer->Add(btnPlay, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
    mainSizer->Add(btnStop, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
    mainSizer->Add(btnLoop, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
    
    mainSizer->AddSpacer(15);
    
    // Tools - Black by default
    btnToolSelect = new wxToggleButton(this, 2001, "", wxDefaultPosition, wxSize(30, 30));
    btnToolSelect->SetBitmap(LoadIcon("select.svg", colBlack));
    btnToolSelect->SetToolTip("Select Tool");
    
    btnToolDraw = new wxToggleButton(this, 2002, "", wxDefaultPosition, wxSize(30, 30));
    btnToolDraw->SetBitmap(LoadIcon("pencil.svg", colBlack)); 
    btnToolDraw->SetToolTip("Draw Tool");
    
    // Erase Removed
    
    btnToolSelect->SetValue(true);
    
    mainSizer->Add(btnToolSelect, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
    mainSizer->Add(btnToolDraw, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
    
    mainSizer->AddSpacer(15);
    
    // Snap Control
    wxArrayString choices;
    choices.Add("1/1");
    choices.Add("1/2");
    choices.Add("1/3");
    choices.Add("1/4");
    choices.Add("1/6");
    choices.Add("1/8");
    choices.Add("1/12");
    choices.Add("1/16");
    
    snapChoice = new wxChoice(this, 3001, wxDefaultPosition, wxSize(60, 30), choices);
    snapChoice->SetSelection(3); // Default 1/4
    mainSizer->Add(snapChoice, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
    
    mainSizer->AddSpacer(10);
    
    // Default Bank Selector
    wxStaticText* lblBank = new wxStaticText(this, wxID_ANY, "Bank:");
    mainSizer->Add(lblBank, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
    
    wxArrayString bankChoices;
    bankChoices.Add("None");
    bankChoices.Add("Normal");
    bankChoices.Add("Soft");
    bankChoices.Add("Drum");
    
    defaultBankChoice = new wxChoice(this, 3002, wxDefaultPosition, wxSize(70, 30), bankChoices);
    defaultBankChoice->SetSelection(0); // Default: None
    defaultBankChoice->SetToolTip("Auto-place HitNormal when placing additions");
    mainSizer->Add(defaultBankChoice, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
    
    // Time Display
    lblTime = new wxStaticText(this, wxID_ANY, "00:00:000");
    wxFont font = lblTime->GetFont();
    font.SetPointSize(14);
    font.SetWeight(wxFONTWEIGHT_BOLD);
    lblTime->SetFont(font);
    
    mainSizer->Add(lblTime, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    
    mainSizer->AddStretchSpacer(1);
    
    // Metadata Panel
    lblZoom = new wxStaticText(this, wxID_ANY, "Zoom: 100%");
    
    wxBoxSizer* metaSizer = new wxBoxSizer(wxVERTICAL);
    metaSizer->Add(lblZoom, 0, wxALIGN_RIGHT, 2);
    
    mainSizer->Add(metaSizer, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
    
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
    lblZoom->SetLabel(wxString::Format("Zoom: %d%%", zoomPercent));
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
