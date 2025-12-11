#pragma once

#include <wx/wx.h>
#include <wx/tglbtn.h>
#include <wx/choice.h>
#include <wx/bmpbuttn.h>
#include <wx/artprov.h>
#include <wx/slider.h>
#include "../model/SampleTypes.h"

class AudioEngine;
class TimelineView;


class TransportPanel : public wxPanel
{
public:
    TransportPanel(wxWindow* parent, AudioEngine* engine, TimelineView* timeline);
    
    void UpdateTime(double time);
    void SetZoomLevel(double pixelsPerSecond);
    void UpdatePlayButton();
    
    void TogglePlayback();
    void Stop();
    
    void SetDefaultBank(SampleSet bank);
    
private:
    void OnPlay(wxCommandEvent& evt);
    void OnStop(wxCommandEvent& evt);
    void OnLoop(wxCommandEvent& evt);
    
    void OnSnapChange(wxCommandEvent& evt);
    void OnDefaultBankChange(wxCommandEvent& evt);
    
    void OnToolSelect(wxCommandEvent& evt);
    void OnToolDraw(wxCommandEvent& evt);
    
    
    void OnSongVolumeChange(wxCommandEvent& evt);
    void OnEffectsVolumeChange(wxCommandEvent& evt);
    
    AudioEngine* audioEngine;
    TimelineView* timelineView;
    
    wxBitmapButton* btnPlay;
    wxBitmapButton* btnStop;
    
    
    wxToggleButton* btnToolSelect;
    wxToggleButton* btnToolDraw;
    
    wxToggleButton* btnLoop;
    
    
    wxStaticText* lblZoom;
    wxChoice* snapChoice;
    wxChoice* defaultBankChoice;
    
    
    wxSlider* songVolumeSlider;
    wxSlider* effectsVolumeSlider;

    
    
    wxBitmap LoadIcon(const std::string& filename, const wxColor& color);

    wxStaticText* lblTime;
    
    wxDECLARE_EVENT_TABLE();
};
