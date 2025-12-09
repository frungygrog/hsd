#pragma once

#include <wx/dialog.h>
#include <wx/textctrl.h>
#include "../model/Track.h"
#include <vector>

class CreatePresetDialog : public wxDialog
{
public:
    CreatePresetDialog(wxWindow* parent, const std::vector<Track>& currentTracks);
    
    struct Result {
        bool confirmed = false;
        wxString presetName;
    };
    
    Result GetResult() const { return result; }
    
    // Static helper to get presets directory
    static wxString GetPresetsDirectory();
    
private:
    void OnOK(wxCommandEvent& evt);
    bool SavePreset(const wxString& name, const std::vector<Track>& tracks);
    
    wxTextCtrl* nameInput;
    Result result;
    const std::vector<Track>& tracksToSave;
    
    wxDECLARE_EVENT_TABLE();
};
