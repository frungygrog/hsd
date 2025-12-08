#pragma once

#include <wx/wx.h>
#include <wx/splitter.h>

#include "TrackList.h"
#include "TimelineView.h"
#include "TransportPanel.h"
#include "../audio/AudioEngine.h"
#include "../io/OsuParser.h"

class MainFrame : public wxFrame
{
public:
    MainFrame();

    enum {
        ID_OPEN_FOLDER = 10001,
        ID_PLAYBACK_TIMER = 10002,
        
        // Edit
        ID_UNDO = 10100,
        ID_REDO,
        ID_CUT,
        ID_COPY,
        ID_PASTE,
        ID_DELETE_SELECTION,
        ID_SELECT_ALL,
        
        // Transport
        ID_PLAY_STOP = 10200,
        ID_REWIND,
        
        // Track
        ID_LOAD_PRESET = 10300
    };
    
private:
    void BuildMenu();

    // Event Handlers
    void OnOpen(wxCommandEvent& evt);
    void OnOpenFolder(wxCommandEvent& evt);
    void OnScrollTimeline(wxScrollWinEvent& evt);
    void OnTimer(wxTimerEvent& evt);
    void OnLoadPreset(wxCommandEvent& evt);
    void ApplyPreset(const std::string& presetName);

    // UI Elements
    wxSplitterWindow* splitter;
    TrackList* trackList;
    TimelineView* timelineView;
    TransportPanel* transportPanel;

    wxTimer playbackTimer;

    // Core Systems
    AudioEngine audioEngine;
    Project project;

    wxDECLARE_EVENT_TABLE();
};
