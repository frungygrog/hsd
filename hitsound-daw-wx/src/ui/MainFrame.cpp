#include "MainFrame.h"
#include "../model/ProjectValidator.h"
#include <juce_audio_devices/juce_audio_devices.h>

#include "MainFrame.h"
#include <juce_audio_devices/juce_audio_devices.h>

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_MENU(wxID_OPEN, MainFrame::OnOpen)
    EVT_MENU(ID_OPEN_FOLDER, MainFrame::OnOpenFolder)
    EVT_TIMER(ID_PLAYBACK_TIMER, MainFrame::OnTimer)
wxEND_EVENT_TABLE()

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, "Hitsound DAW (wx + JUCE)", wxDefaultPosition, wxSize(1200, 800))
    , playbackTimer(this, ID_PLAYBACK_TIMER)
{
    audioEngine.initialize();
    
    // Load Default Samples
    // Robustly find Resources folder
    juce::File exeDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
    juce::File resourcesDir;
    
    // Check 1: CWD
    juce::File cwd = juce::File::getCurrentWorkingDirectory();
    if (cwd.getChildFile("Resources").exists())
        resourcesDir = cwd.getChildFile("Resources");
        
    // Check 2: Up from Exe (build/Debug/../../Resources)
    else if (exeDir.getParentDirectory().getParentDirectory().getChildFile("Resources").exists())
        resourcesDir = exeDir.getParentDirectory().getParentDirectory().getChildFile("Resources");
        
    // Check 3: Up from Exe one Level (build/Resources?)
    else if (exeDir.getParentDirectory().getChildFile("Resources").exists())
        resourcesDir = exeDir.getParentDirectory().getChildFile("Resources");
        
    if (resourcesDir.isDirectory())
    {
         audioEngine.GetSampleRegistry().loadDefaultSamples(resourcesDir);
    }
    else
    {
        wxMessageBox("Could not find 'Resources' folder containing samples.", "Warning", wxICON_WARNING);
    }
    
    BuildMenu();
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Splitter
    splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);
    
    // Left: TrackList
    trackList = new TrackList(splitter);
    
    // Right: TimelineView
    timelineView = new TimelineView(splitter);
    
    // Connect TrackList to TimelineView for reordering updates
    trackList->SetTimelineView(timelineView);
    
    // Transport Panel (Top)
    transportPanel = new TransportPanel(this, &audioEngine, timelineView);
    mainSizer->Add(transportPanel, 0, wxEXPAND | wxALL, 5);
    
    // Split
    splitter->SplitVertically(trackList, timelineView, 250);
    splitter->SetMinimumPaneSize(100);
    
    // Sync Scrolling: TrackList mirrors TimelineView's vertical scroll position
    // We use multiple event types to ensure complete coverage
    
    // Helper lambda to sync TrackList to TimelineView's scroll position
    auto doSync = [this]() {
        int x, y;
        timelineView->GetViewStart(&x, &y);
        int scrollUnitX, scrollUnitY;
        timelineView->GetScrollPixelsPerUnit(&scrollUnitX, &scrollUnitY);
        int scrollPxY = y * scrollUnitY;
        trackList->SetVerticalScrollOffset(scrollPxY);
    };
    
    // Scroll events
    auto syncScrollEvt = [this, doSync](wxScrollWinEvent& evt) {
        // Use CallAfter to ensure scroll position is updated before we read it
        CallAfter(doSync);
        evt.Skip();
    };
    
    timelineView->Bind(wxEVT_SCROLLWIN_TOP, syncScrollEvt);
    timelineView->Bind(wxEVT_SCROLLWIN_BOTTOM, syncScrollEvt);
    timelineView->Bind(wxEVT_SCROLLWIN_LINEUP, syncScrollEvt);
    timelineView->Bind(wxEVT_SCROLLWIN_LINEDOWN, syncScrollEvt);
    timelineView->Bind(wxEVT_SCROLLWIN_PAGEUP, syncScrollEvt);
    timelineView->Bind(wxEVT_SCROLLWIN_PAGEDOWN, syncScrollEvt);
    timelineView->Bind(wxEVT_SCROLLWIN_THUMBTRACK, syncScrollEvt);
    timelineView->Bind(wxEVT_SCROLLWIN_THUMBRELEASE, syncScrollEvt);
    
    // Mouse wheel - this is often missed by SCROLLWIN events
    timelineView->Bind(wxEVT_MOUSEWHEEL, [this, doSync](wxMouseEvent& evt) {
        CallAfter(doSync);
        evt.Skip();
    });
    
    // Paint event as failsafe - ensures sync whenever timeline repaints
    timelineView->Bind(wxEVT_PAINT, [this, doSync](wxPaintEvent& evt) {
        doSync();
        evt.Skip();
    });
    
    // Add splitter
    mainSizer->Add(splitter, 1, wxEXPAND);
    SetSizer(mainSizer);
    
    // Loop Callback
    timelineView->OnLoopPointsChanged = [this](double start, double end) {
        audioEngine.SetLoopPoints(start, end);
    };

    playbackTimer.Start(30); // 30ms interval (~30fps)
}

void MainFrame::BuildMenu()
{
    wxMenuBar* menuBar = new wxMenuBar;
    
    // File
    wxMenu* fileMenu = new wxMenu;
    fileMenu->Append(wxID_OPEN, "&Open .osu File...\tCtrl+O", "Open an osu! file directly");
    fileMenu->Append(ID_OPEN_FOLDER, "Open Project &Folder...\tCtrl+Shift+O", "Open a beatmap folder");
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, "E&xit\tAlt+X", "Quit this program");
    menuBar->Append(fileMenu, "&File");
    
    // Edit
    wxMenu* editMenu = new wxMenu;
    editMenu->Append(ID_UNDO, "&Undo\tCtrl+Z");
    editMenu->Append(ID_REDO, "&Redo\tCtrl+Y");
    editMenu->AppendSeparator();
    editMenu->Append(ID_CUT, "Cu&t\tCtrl+X");
    editMenu->Append(ID_COPY, "&Copy\tCtrl+C");
    editMenu->Append(ID_PASTE, "&Paste\tCtrl+V");
    editMenu->Append(ID_DELETE_SELECTION, "&Delete\tDel");
    editMenu->AppendSeparator();
    editMenu->Append(ID_SELECT_ALL, "Select &All\tCtrl+A");
    menuBar->Append(editMenu, "&Edit");
    
    // Transport
    wxMenu* transportMenu = new wxMenu;
    transportMenu->Append(ID_PLAY_STOP, "Play/Stop\tSpace");
    transportMenu->Append(ID_REWIND, "Rewind to Start\tHome");
    menuBar->Append(transportMenu, "&Transport");
    
    SetMenuBar(menuBar);
    
    // Bind Events
    Bind(wxEVT_MENU, [this](wxCommandEvent&){ timelineView->Undo(); }, ID_UNDO);
    Bind(wxEVT_MENU, [this](wxCommandEvent&){ timelineView->Redo(); }, ID_REDO);
    Bind(wxEVT_MENU, [this](wxCommandEvent&){ timelineView->CutSelection(); }, ID_CUT);
    Bind(wxEVT_MENU, [this](wxCommandEvent&){ timelineView->CopySelection(); }, ID_COPY);
    Bind(wxEVT_MENU, [this](wxCommandEvent&){ timelineView->PasteAtPlayhead(); }, ID_PASTE);
    Bind(wxEVT_MENU, [this](wxCommandEvent&){ timelineView->DeleteSelection(); }, ID_DELETE_SELECTION);
    Bind(wxEVT_MENU, [this](wxCommandEvent&){ timelineView->SelectAll(); }, ID_SELECT_ALL);
    
    Bind(wxEVT_MENU, [this](wxCommandEvent&){ transportPanel->TogglePlayback(); }, ID_PLAY_STOP);
    Bind(wxEVT_MENU, [this](wxCommandEvent&){ transportPanel->Stop(); }, ID_REWIND); // Reusing Stop logic which rewinds
}

void MainFrame::OnOpen(wxCommandEvent& evt)
{
    wxFileDialog openFileDialog(this, _("Open .osu file"), "", "",
                                "osu! files (*.osu)|*.osu", wxFD_OPEN|wxFD_FILE_MUST_EXIST);
                                
    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;
        
    juce::File file(openFileDialog.GetPath().ToStdString());
    project = OsuParser::parse(file);
    
    // Validate
    ProjectValidator::Validate(project);
    
    trackList->SetProject(&project);
    timelineView->SetProject(&project);
    
    // Critical: Update AudioEngine
    audioEngine.SetTracks(&project.tracks);
    
    // Load Audio
    juce::File audioFile = file.getParentDirectory().getChildFile(juce::String(project.audioFilename));
    if (audioFile.existsAsFile())
    {
        audioEngine.LoadMasterTrack(audioFile.getFullPathName().toStdString());
        
        // Generate Waveform
        // Calculate width needed: 4000 px for now
        int w = 4000; 
        auto peaks = audioEngine.GetWaveform(w); 
        double duration = audioEngine.GetDuration();
        timelineView->SetWaveform(peaks, duration);
        
        transportPanel->SetProjectDetails(project.bpm, project.offset);
    }
    
    // Total height
    // Total height calculation
    // We need to be more precise for Scrollbar to work nicely.
    // traverse to count expanded height?
    // For now, heuristic: 
    int totalHeight = 2000; // Default buffer
    // Better: Helper in Project or TrackList to calculate total pixels?
    // Let's iterate simply
    int calculatedHeight = 130; // Header (30 Ruler + 100 Master)
    std::function<void(Track&)> calcH = [&](Track& t) {
        calculatedHeight += (t.name.find("%)") != std::string::npos) ? 40 : 80;
        if (t.isExpanded && !t.children.empty()) {
            for (auto& c : t.children) calcH(c);
        }
    };
    for (auto& t : project.tracks) calcH(t);
    totalHeight = calculatedHeight + 200; // Buffer

    double duration = 300.0;
    if (audioEngine.GetDuration() > 0) duration = audioEngine.GetDuration();
    if (duration < 300.0) duration = 300.0; // Min size
    
    int totalWidth = static_cast<int>(duration * 100.0) + 2000; // Extra buffer
    timelineView->SetVirtualSize(totalWidth, totalHeight);
    timelineView->SetScrollRate(10, 10);
    // Note: TrackList is now a plain wxPanel that mirrors TimelineView's scroll via SetVerticalScrollOffset
}

void MainFrame::OnOpenFolder(wxCommandEvent& evt)
{
    wxDirDialog dirDialog(this, "Select Project Folder (Unzipped .osk/.osz)", "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);

    if (dirDialog.ShowModal() == wxID_CANCEL)
        return;

    juce::File dir(dirDialog.GetPath().ToStdString());
    
    // Find .osu files
    auto osuFiles = dir.findChildFiles(juce::File::findFiles, false, "*.osu");
    
    if (osuFiles.isEmpty())
    {
        wxMessageBox("No .osu files found in the selected directory.", "Error", wxICON_ERROR);
        return;
    }
    
    // Ideally user chooses, but for now pick first
    juce::File fileToLoad = osuFiles[0];
    
    // If multiple, maybe ask? 
    if (osuFiles.size() > 1)
    {
        // Simple logic: prefer the one with most recent modification? Or just prompt (too complex for now?)
        // Let's just load the first one and notify user?
        // Or if we can, use a wxSingleChoiceDialog
        wxArrayString choices;
        for (const auto& f : osuFiles)
            choices.Add(f.getFileName().toStdString());
            
        wxSingleChoiceDialog choiceDialog(this, "Select a difficulty:", "Multiple .osu files found", choices);
        if (choiceDialog.ShowModal() == wxID_OK)
        {
            int sel = choiceDialog.GetSelection();
            if (sel >= 0 && sel < osuFiles.size())
                fileToLoad = osuFiles[sel];
        }
        else
            return;
    }
    
    project = OsuParser::parse(fileToLoad);
    ProjectValidator::Validate(project);
    trackList->SetProject(&project);
    timelineView->SetProject(&project);
    
    // Critical: Update AudioEngine
    audioEngine.SetTracks(&project.tracks);
    
    juce::File audioFile = dir.getChildFile(juce::String(project.audioFilename));
    if (audioFile.existsAsFile())
    {
        audioEngine.LoadMasterTrack(audioFile.getFullPathName().toStdString());
        auto peaks = audioEngine.GetWaveform(4000);
        double duration = audioEngine.GetDuration();
        timelineView->SetWaveform(peaks, duration);
        
        transportPanel->SetProjectDetails(project.bpm, project.offset);
    }

    // Recalculate Height
    int calculatedHeight = 130; 
    std::function<void(Track&)> calcH = [&](Track& t) {
        calculatedHeight += (t.name.find("%)") != std::string::npos) ? 40 : 80;
        if (t.isExpanded && !t.children.empty()) {
            for (auto& c : t.children) calcH(c);
        }
    };
    for (auto& t : project.tracks) calcH(t);
    int totalHeight = calculatedHeight + 200;

    double duration = 300.0;
    if (audioEngine.GetDuration() > 0) duration = audioEngine.GetDuration();
    if (duration < 300.0) duration = 300.0;
    
    int totalWidth = static_cast<int>(duration * 100.0) + 2000;
    timelineView->SetVirtualSize(totalWidth, totalHeight);
    timelineView->SetScrollRate(10, 10);
}

// void MainFrame::OnScrollTimeline(wxScrollWinEvent& evt) { ... } Removed/Replaced by lambda

void MainFrame::OnTimer(wxTimerEvent& evt)
{
    if (audioEngine.IsPlaying())
    {
        double pos = audioEngine.GetPosition();
        transportPanel->UpdateTime(pos);
        timelineView->SetPlayheadPosition(pos);
    }
}
