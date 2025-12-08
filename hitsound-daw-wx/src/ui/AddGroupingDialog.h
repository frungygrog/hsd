#pragma once
#include <wx/wx.h>
#include "../model/SampleTypes.h"

struct AddGroupingResult
{
    bool confirmed = false;
    wxString name;
    
    SampleSet hitNormalBank;
    
    SampleSet additionsBank;
    bool hasWhistle = false;
    bool hasFinish = false;
    bool hasClap = false;
    
    int volume = 100;
};

class AddGroupingDialog : public wxDialog
{
public:
    AddGroupingDialog(wxWindow* parent);
    AddGroupingResult GetResult() const { return result; }
    
private:
    void OnOK(wxCommandEvent& evt);
    
    wxTextCtrl* nameCtrl;
    wxChoice* normalBankChoice;
    wxChoice* additionsBankChoice;
    
    wxCheckBox* chkWhistle;
    wxCheckBox* chkFinish;
    wxCheckBox* chkClap;
    
    wxSlider* volumeSlider;
    
    AddGroupingResult result;
    
    wxDECLARE_EVENT_TABLE();
};
