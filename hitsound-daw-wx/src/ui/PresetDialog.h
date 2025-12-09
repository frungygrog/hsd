#pragma once

#include <wx/wx.h>
#include <wx/choice.h>
#include "../model/Track.h"
#include <vector>

class PresetDialog : public wxDialog
{
public:
    PresetDialog(wxWindow* parent);
    
    struct Result {
        bool confirmed = false;
        wxString presetName;
        bool isBuiltIn = true;  
    };
    
    Result GetResult() const { return result; }
    
    
    static std::vector<Track> LoadPresetFromFile(const wxString& filepath);
    
private:
    void OnOK(wxCommandEvent& evt);
    void RefreshPresetList();
    
    wxChoice* presetChoice;
    Result result;
    wxArrayString presetNames;
    wxArrayString presetPaths;  
    
    wxDECLARE_EVENT_TABLE();
};
