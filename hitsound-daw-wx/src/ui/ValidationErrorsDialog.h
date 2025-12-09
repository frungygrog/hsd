#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <vector>
#include "../model/ProjectValidator.h"

class ValidationErrorsDialog : public wxDialog
{
public:
    ValidationErrorsDialog(wxWindow* parent, const std::vector<ProjectValidator::ValidationError>& errors);

    
    bool IsIgnored() const { return ignored; }

private:
    wxListCtrl* listCtrl;
    bool ignored = false;
    
    void OnIgnore(wxCommandEvent& evt);
    void OnCancel(wxCommandEvent& evt);
};
