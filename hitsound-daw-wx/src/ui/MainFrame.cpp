#include "MainFrame.h"
#include "../model/ProjectValidator.h"
#include "../Constants.h"
#include "PresetDialog.h"
#include "CreatePresetDialog.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include "ProjectSetupDialog.h"
#include "ProjectSetupDialog.h"
#include "../io/OsuParser.h"
#include "../io/ProjectSaver.h"
#include "ValidationErrorsDialog.h"
#include <wx/filename.h>

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_MENU(wxID_OPEN, MainFrame::OnOpen)
    EVT_MENU(ID_OPEN_FOLDER, MainFrame::OnOpenFolder)
    EVT_TIMER(ID_PLAYBACK_TIMER, MainFrame::OnTimer)
    EVT_CLOSE(MainFrame::OnClose)
wxEND_EVENT_TABLE()

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, "Hitsound DAW (wx + JUCE)", wxDefaultPosition, wxSize(1200, 800))
    , playbackTimer(this, ID_PLAYBACK_TIMER)
{
    audioEngine.initialize();
    
    
    
    juce::File exeDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
    juce::File resourcesDir;
    
    
    juce::File cwd = juce::File::getCurrentWorkingDirectory();
    if (cwd.getChildFile("Resources").exists())
        resourcesDir = cwd.getChildFile("Resources");
        
    
    else if (exeDir.getParentDirectory().getParentDirectory().getChildFile("Resources").exists())
        resourcesDir = exeDir.getParentDirectory().getParentDirectory().getChildFile("Resources");
        
    
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
    
    
    splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);
    
    
    trackList = new TrackList(splitter);
    
    
    timelineView = new TimelineView(splitter);
    
    
    trackList->SetTimelineView(timelineView);
    
    
    transportPanel = new TransportPanel(this, &audioEngine, timelineView);
    mainSizer->Add(transportPanel, 0, wxEXPAND | wxALL, 5);
    
    
    splitter->SplitVertically(trackList, timelineView, 250);
    splitter->SetMinimumPaneSize(100);
    
    
    
    
    
    auto doSync = [this]() {
        int x, y;
        timelineView->GetViewStart(&x, &y);
        int scrollUnitX, scrollUnitY;
        timelineView->GetScrollPixelsPerUnit(&scrollUnitX, &scrollUnitY);
        int scrollPxY = y * scrollUnitY;
        trackList->SetVerticalScrollOffset(scrollPxY);
    };
    
    
    auto syncScrollEvt = [this, doSync](wxScrollWinEvent& evt) {
        
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
    
    
    timelineView->Bind(wxEVT_MOUSEWHEEL, [this, doSync](wxMouseEvent& evt) {
        CallAfter(doSync);
        evt.Skip();
    });
    
    
    timelineView->Bind(wxEVT_PAINT, [this, doSync](wxPaintEvent& evt) {
        doSync();
        evt.Skip();
    });
    
    
    mainSizer->Add(splitter, 1, wxEXPAND);
    SetSizer(mainSizer);
    
    
    timelineView->OnLoopPointsChanged = [this](double start, double end) {
        audioEngine.SetLoopPoints(start, end);
    };
    
    
    timelineView->OnZoomChanged = [this](double pixelsPerSecond) {
        transportPanel->SetZoomLevel(pixelsPerSecond);
    };
    
    
    timelineView->OnTracksModified = [this]() {
        audioEngine.NotifyTracksChanged();
    };
    
    
    timelineView->OnPlayheadScrubbed = [this](double time) {
        audioEngine.SetPosition(time);
        transportPanel->UpdateTime(time);
    };

    playbackTimer.Start(30); 
}

void MainFrame::BuildMenu()
{
    wxMenuBar* menuBar = new wxMenuBar;
    
    
    wxMenu* fileMenu = new wxMenu;
    fileMenu->Append(wxID_OPEN, "&Open .osu File...\tCtrl+O", "Open an osu! file directly");
    fileMenu->Append(ID_OPEN_FOLDER, "Open Project &Folder...\tCtrl+Shift+O", "Open a beatmap folder");
    fileMenu->AppendSeparator();
    fileMenu->Append(ID_SAVE, "&Save\tCtrl+S", "Save the project");
    fileMenu->Append(ID_SAVE_AS, "Save &As...\tCtrl+Shift+S", "Save the project as a new file");
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, "E&xit\tAlt+X", "Quit this program");
    menuBar->Append(fileMenu, "&File");
    
    
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
    
    
    wxMenu* transportMenu = new wxMenu;
    transportMenu->Append(ID_PLAY_STOP, "Play/Stop\tSpace");
    transportMenu->Append(ID_REWIND, "Rewind to Start\tHome");
    menuBar->Append(transportMenu, "&Transport");
    
    
    wxMenu* trackMenu = new wxMenu;
    
    
    wxMenu* presetSubMenu = new wxMenu;
    presetSubMenu->Append(ID_PRESET_GENERIC, "Generic", "Load the generic hitsound preset");
    trackMenu->AppendSubMenu(presetSubMenu, "Load Preset");
    
    trackMenu->Append(ID_CREATE_PRESET, "Create Preset...", "Create a new preset from current tracks");
    menuBar->Append(trackMenu, "Trac&k");
    
    SetMenuBar(menuBar);
    
    
    Bind(wxEVT_MENU, [this](wxCommandEvent&){ timelineView->Undo(); }, ID_UNDO);
    Bind(wxEVT_MENU, [this](wxCommandEvent&){ timelineView->Redo(); }, ID_REDO);
    Bind(wxEVT_MENU, [this](wxCommandEvent&){ timelineView->CutSelection(); }, ID_CUT);
    Bind(wxEVT_MENU, [this](wxCommandEvent&){ timelineView->CopySelection(); }, ID_COPY);
    Bind(wxEVT_MENU, [this](wxCommandEvent&){ timelineView->PasteAtPlayhead(); }, ID_PASTE);
    Bind(wxEVT_MENU, [this](wxCommandEvent&){ timelineView->DeleteSelection(); }, ID_DELETE_SELECTION);
    Bind(wxEVT_MENU, [this](wxCommandEvent&){ timelineView->SelectAll(); }, ID_SELECT_ALL);
    
    Bind(wxEVT_MENU, [this](wxCommandEvent&){ transportPanel->TogglePlayback(); }, ID_PLAY_STOP);
    Bind(wxEVT_MENU, [this](wxCommandEvent&){ transportPanel->Stop(); }, ID_REWIND); 
    
    Bind(wxEVT_MENU, &MainFrame::OnLoadPreset, this, ID_LOAD_PRESET);
    Bind(wxEVT_MENU, [this](wxCommandEvent&){ ApplyPreset("Generic"); }, ID_PRESET_GENERIC);
    Bind(wxEVT_MENU, &MainFrame::OnCreatePreset, this, ID_CREATE_PRESET);
    Bind(wxEVT_MENU, &MainFrame::OnSave, this, ID_SAVE);
    Bind(wxEVT_MENU, &MainFrame::OnSaveAs, this, ID_SAVE_AS);
}

void MainFrame::OnOpen(wxCommandEvent& evt)
{
    wxFileDialog openFileDialog(this, _("Open .osu file"), "", "",
                                "osu! files (*.osu)|*.osu", wxFD_OPEN|wxFD_FILE_MUST_EXIST);
                                
    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;
        
    juce::File file(openFileDialog.GetPath().ToStdString());
    project = OsuParser::parse(file);
    
    
    ProjectValidator::Validate(project);
    
    trackList->SetProject(&project);
    timelineView->SetProject(&project);
    
    
    audioEngine.SetTracks(&project.tracks);
    
    
    juce::File audioFile = file.getParentDirectory().getChildFile(juce::String(project.audioFilename));
    if (audioFile.existsAsFile())
    {
        audioEngine.LoadMasterTrack(audioFile.getFullPathName().toStdString());
        
        
        
        int w = 4000; 
        auto peaks = audioEngine.GetWaveform(w); 
        double duration = audioEngine.GetDuration();
        timelineView->SetWaveform(peaks, duration);
    }
    
    
    
    
    
    
    int totalHeight = 2000; 
    
    
    int calculatedHeight = TrackLayout::HeaderHeight; 
    std::function<void(Track&)> calcH = [&](Track& t) {
        calculatedHeight += TrackLayout::GetTrackHeight(t.isChildTrack);
        if (t.isExpanded && !t.children.empty()) {
            for (auto& c : t.children) calcH(c);
        }
    };
    for (auto& t : project.tracks) calcH(t);
    totalHeight = calculatedHeight + 200; 

    double duration = 300.0;
    if (audioEngine.GetDuration() > 0) duration = audioEngine.GetDuration();
    if (duration < 300.0) duration = 300.0; 
    
    int totalWidth = static_cast<int>(duration * 100.0) + 2000; 
    timelineView->SetVirtualSize(totalWidth, totalHeight);
    timelineView->SetScrollRate(10, 10);
    
}

void MainFrame::OnOpenFolder(wxCommandEvent& evt)
{
    wxDirDialog dirDialog(this, "Select Project Folder (Unzipped .osk/.osz)", "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);

    if (dirDialog.ShowModal() == wxID_CANCEL)
        return;

    juce::File dir(dirDialog.GetPath().ToStdString());
    
    
    auto osuFiles = dir.findChildFiles(juce::File::findFiles, false, "*.osu");
    
    if (osuFiles.isEmpty())
    {
        wxMessageBox("No .osu files found in the selected directory.", "Error", wxICON_ERROR);
        return;
    }
    
    std::vector<std::string> fileNames;
    for (const auto& f : osuFiles)
        fileNames.push_back(f.getFileName().toStdString());

    ProjectSetupDialog dlg(this, fileNames);
    if (dlg.ShowModal() != wxID_OK)
        return;

    std::string refFilename = dlg.GetSelectedFilename();
    juce::File referenceFile = dir.getChildFile(refFilename);

    if (dlg.GetSelectedAction() == ProjectSetupDialog::Action::OpenExisting)
    {
        project = OsuParser::parse(referenceFile);
    }
    else
    {
        
        
        Project refProject = OsuParser::parse(referenceFile);
        
        std::string newFilename = refProject.artist + " - " + refProject.title + " (" + refProject.creator + ") [Hitsounds].osu";
        
        
        juce::File targetFile = dir.getChildFile(newFilename);
        
        if (targetFile.existsAsFile())
        {
            int res = wxMessageBox("Target file '" + newFilename + "' already exists. Overwrite?", "File Exists", wxYES_NO | wxICON_QUESTION);
            if (res != wxYES) return;
        }

        if (OsuParser::CreateHitsoundDiff(referenceFile, targetFile))
        {
            project = OsuParser::parse(targetFile);
        }
        else
        {
            wxMessageBox("Failed to create hitsound difficulty.", "Error", wxICON_ERROR);
            return;
        }
    }
    
    
    ProjectValidator::Validate(project);
    trackList->SetProject(&project);
    timelineView->SetProject(&project);
    
    
    audioEngine.SetTracks(&project.tracks);
    
    juce::File audioFile = dir.getChildFile(juce::String(project.audioFilename));
    if (audioFile.existsAsFile())
    {
        audioEngine.LoadMasterTrack(audioFile.getFullPathName().toStdString());
        auto peaks = audioEngine.GetWaveform(4000);
        double duration = audioEngine.GetDuration();
        timelineView->SetWaveform(peaks, duration);
    }

    
    int calculatedHeight = TrackLayout::HeaderHeight; 
    std::function<void(Track&)> calcH = [&](Track& t) {
        calculatedHeight += TrackLayout::GetTrackHeight(t.isChildTrack);
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



void MainFrame::OnTimer(wxTimerEvent& evt)
{
    if (audioEngine.IsPlaying())
    {
        double pos = audioEngine.GetPosition();
        transportPanel->UpdateTime(pos);
        timelineView->SetPlayheadPosition(pos);
    }
}

void MainFrame::OnLoadPreset(wxCommandEvent& evt)
{
    PresetDialog dlg(this);
    if (dlg.ShowModal() == wxID_OK)
    {
        auto result = dlg.GetResult();
        if (result.confirmed)
        {
            if (result.isBuiltIn)
            {
                
                if (result.presetName.StartsWith("Generic"))
                    ApplyPreset("Generic");
            }
            else
            {
                
                wxString presetsDir = CreatePresetDialog::GetPresetsDirectory();
                wxString filepath = presetsDir + wxFileName::GetPathSeparator() + result.presetName + ".preset";
                
                std::vector<Track> loadedTracks = PresetDialog::LoadPresetFromFile(filepath);
                
                if (!loadedTracks.empty())
                {
                    for (auto& t : loadedTracks)
                        project.tracks.push_back(t);
                    
                    trackList->SetProject(&project);
                    timelineView->SetProject(&project);
                    timelineView->UpdateVirtualSize();
                    
                    wxMessageBox(wxString::Format("Preset '%s' loaded successfully!\n\n%zu tracks added.", 
                                result.presetName, loadedTracks.size()), 
                                "Preset Loaded", wxICON_INFORMATION);
                }
                else
                {
                    wxMessageBox("Failed to load preset or preset is empty.", "Error", wxICON_ERROR);
                }
            }
        }
    }
}

void MainFrame::OnCreatePreset(wxCommandEvent& evt)
{
    CreatePresetDialog dlg(this, project.tracks);
    dlg.ShowModal();
}

void MainFrame::ApplyPreset(const std::string& presetName)
{
    if (presetName == "Generic")
    {
        
        auto createTrack = [](const std::string& name, SampleSet bank, SampleType type) -> Track {
            Track parent;
            parent.name = name;
            parent.sampleSet = bank;
            parent.sampleType = type;
            parent.gain = 1.0;
            parent.isExpanded = false;
            parent.primaryChildIndex = 0;
            
            
            std::string sStr = (bank == SampleSet::Normal) ? "normal" : (bank == SampleSet::Soft ? "soft" : "drum");
            std::string tStr;
            switch (type) {
                case SampleType::HitNormal: tStr = "hitnormal"; break;
                case SampleType::HitWhistle: tStr = "hitwhistle"; break;
                case SampleType::HitFinish: tStr = "hitfinish"; break;
                case SampleType::HitClap: tStr = "hitclap"; break;
                default: tStr = "hitnormal"; break;
            }
            
            Track child;
            child.name = sStr + "-" + tStr + " (60%)";
            child.sampleSet = bank;
            child.sampleType = type;
            child.gain = 0.6;
            child.isChildTrack = true;
            
            parent.children.push_back(child);
            return parent;
        };
        
        
        project.tracks.push_back(createTrack("Kick", SampleSet::Normal, SampleType::HitNormal));
        project.tracks.push_back(createTrack("Hi-Hat", SampleSet::Drum, SampleType::HitNormal));
        project.tracks.push_back(createTrack("Default", SampleSet::Soft, SampleType::HitNormal));
        project.tracks.push_back(createTrack("Whistle", SampleSet::Soft, SampleType::HitWhistle));
        project.tracks.push_back(createTrack("Clap", SampleSet::Soft, SampleType::HitClap));
        project.tracks.push_back(createTrack("Crash", SampleSet::Soft, SampleType::HitFinish));
        
        
        Track snare;
        snare.name = "Snare";
        snare.isGrouping = true;
        snare.isExpanded = false;
        snare.primaryChildIndex = 0;
        snare.gain = 1.0;
        
        
        Track snareChild;
        snareChild.name = "Snare (60%)";
        snareChild.sampleSet = SampleSet::Soft;
        snareChild.sampleType = SampleType::HitNormal;
        snareChild.gain = 0.6;
        snareChild.isChildTrack = true;
        snareChild.layers.push_back({SampleSet::Soft, SampleType::HitNormal});
        snareChild.layers.push_back({SampleSet::Soft, SampleType::HitClap});
        
        snare.children.push_back(snareChild);
        project.tracks.push_back(snare);
        
        
        trackList->SetProject(&project);
        timelineView->SetProject(&project);
        timelineView->UpdateVirtualSize();
        
        wxMessageBox("Generic preset loaded successfully!\n\n6 Tracks + 1 Grouping added.", "Preset Loaded", wxICON_INFORMATION);
    }
}

void MainFrame::OnSave(wxCommandEvent& evt)
{
    
    if (project.creator == "hsd" && !project.projectFilePath.empty())
    {
        PerformSave(juce::File(project.projectFilePath));
    }
    else
    {
        
        OnSaveAs(evt);
    }
}

void MainFrame::OnSaveAs(wxCommandEvent& evt)
{
    wxFileDialog saveFileDialog(this, _("Save Project As"), "", "",
                                "osu! files (*.osu)|*.osu", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (saveFileDialog.ShowModal() == wxID_CANCEL)
        return;

    juce::File file(saveFileDialog.GetPath().ToStdString());

    
    project.projectFilePath = file.getFullPathName().toStdString();
    project.projectDirectory = file.getParentDirectory().getFullPathName().toStdString();
    
    
    project.creator = "hsd";

    PerformSave(file);
}

bool MainFrame::PerformSave(const juce::File& file)
{
    
    auto errors = ProjectValidator::Validate(project);
    
    
    if (!errors.empty()) {
        wxMessageBox(wxString::Format("Found %llu errors.", errors.size()));
    } else {
        
    }
    
    
    timelineView->Refresh();
    
    if (!errors.empty())
    {
        ValidationErrorsDialog dlg(this, errors);
        int result = dlg.ShowModal();
        if (!dlg.IsIgnored()) 
        {
            return false;
        }
    }
    
    
    if (ProjectSaver::SaveProject(project, file))
    {
        timelineView->GetUndoManager().MarkClean();
        wxMessageBox("Project saved successfully!", "Success", wxICON_INFORMATION);
        return true;
    }
    else
    {
        wxMessageBox("Failed to save project. Check file permissions or disk space.", "Error", wxICON_ERROR);
        return false;
    }
}

void MainFrame::OnClose(wxCloseEvent& evt)
{
    
    if (timelineView && timelineView->GetUndoManager().IsDirty())
    {
        int result = wxMessageBox(
            "You have unsaved changes. Do you want to save before closing?",
            "Unsaved Changes",
            wxYES_NO | wxCANCEL | wxICON_WARNING
        );
        
        if (result == wxYES)
        {
            
            wxCommandEvent dummy;
            OnSave(dummy);
            
            
            if (timelineView->GetUndoManager().IsDirty())
            {
                
                evt.Veto();
                return;
            }
        }
        else if (result == wxCANCEL)
        {
            evt.Veto();
            return;
        }
        
    }
    
    evt.Skip(); 
}
