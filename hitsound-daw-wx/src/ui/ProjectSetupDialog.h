#pragma once
#include <wx/wx.h>
#include <vector>
#include <string>

class ProjectSetupDialog : public wxDialog
{
public:
    enum class Action {
        OpenExisting,
        StartFromScratch
    };

    ProjectSetupDialog(wxWindow* parent, const std::vector<std::string>& osuFiles);

    Action GetSelectedAction() const;
    std::string GetSelectedFilename() const;

private:
    void OnOK(wxCommandEvent& evt);
    void OnRadioChange(wxCommandEvent& evt);

    wxRadioBox* actionRadio;
    wxComboBox* fileChoice;
    std::vector<std::string> files;
    
    Action selectedAction = Action::OpenExisting;
    std::string selectedFilename;

    wxDECLARE_EVENT_TABLE();
};
