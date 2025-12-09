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
        bool isBuiltIn = true;  // True for "Generic", false for custom presets
    };
    
    Result GetResult() const { return result; }
    
    // Load a custom preset from file - returns tracks to add
    static std::vector<Track> LoadPresetFromFile(const wxString& filepath);
    
private:
    void OnOK(wxCommandEvent& evt);
    void RefreshPresetList();
    
    wxChoice* presetChoice;
    Result result;
    wxArrayString presetNames;
    wxArrayString presetPaths;  // Empty for built-in
    
    wxDECLARE_EVENT_TABLE();
};
