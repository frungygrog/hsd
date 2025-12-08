#pragma once

#include <wx/wx.h>
#include <wx/choice.h>

class PresetDialog : public wxDialog
{
public:
    PresetDialog(wxWindow* parent);
    
    struct Result {
        bool confirmed = false;
        wxString presetName;
    };
    
    Result GetResult() const { return result; }
    
private:
    void OnOK(wxCommandEvent& evt);
    
    wxChoice* presetChoice;
    Result result;
    
    wxDECLARE_EVENT_TABLE();
};
