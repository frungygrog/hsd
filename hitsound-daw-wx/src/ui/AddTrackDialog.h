#pragma once
#include <wx/wx.h>
#include "../model/SampleTypes.h"


struct AddTrackResult {
    wxString name;
    SampleSet bank;
    SampleType type;
    int volume;
    bool confirmed;
};

class AddTrackDialog : public wxDialog
{
public:
    AddTrackDialog(wxWindow* parent);
    
    AddTrackResult GetResult() const { return result; }
    
    
    void SetValues(const wxString& name, SampleSet bank, SampleType type, int volume);
    void SetEditMode(bool edit);
    
private:
    void OnOK(wxCommandEvent& evt);
    
    wxTextCtrl* nameCtrl;
    wxChoice* bankChoice;
    wxChoice* typeChoice;
    wxSlider* volumeSlider;
    wxButton* okBtn;
    
    bool isEditMode = false;
    AddTrackResult result;
    
    wxDECLARE_EVENT_TABLE();
};
