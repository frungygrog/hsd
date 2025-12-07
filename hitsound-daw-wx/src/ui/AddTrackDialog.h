#pragma once
#include <wx/wx.h>
#include "../model/SampleTypes.h"

// Define a struct or pass pointers to return selected values
struct AddTrackResult {
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
    
private:
    void OnOK(wxCommandEvent& evt);
    
    wxChoice* bankChoice;
    wxChoice* typeChoice;
    wxSlider* volumeSlider;
    
    AddTrackResult result;
    
    wxDECLARE_EVENT_TABLE();
};
