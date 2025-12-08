#include "ProjectSetupDialog.h"

wxBEGIN_EVENT_TABLE(ProjectSetupDialog, wxDialog)
    EVT_BUTTON(wxID_OK, ProjectSetupDialog::OnOK)
    EVT_RADIOBOX(wxID_ANY, ProjectSetupDialog::OnRadioChange)
wxEND_EVENT_TABLE()

ProjectSetupDialog::ProjectSetupDialog(wxWindow* parent, const std::vector<std::string>& osuFiles)
    : wxDialog(parent, wxID_ANY, "Project Setup", wxDefaultPosition, wxSize(400, 300)),
      files(osuFiles)
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Description
    wxStaticText* label = new wxStaticText(this, wxID_ANY, "How would you like to start?");
    mainSizer->Add(label, 0, wxALL, 10);
    
    // Radio Box
    wxArrayString choices;
    choices.Add("Open Existing Difficulty");
    choices.Add("Start from Scratch (Hitsounds)");
    
    actionRadio = new wxRadioBox(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, choices, 1, wxRA_SPECIFY_COLS);
    mainSizer->Add(actionRadio, 0, wxEXPAND | wxALL, 10);
    
    // File Selection
    wxStaticText* fileLabel = new wxStaticText(this, wxID_ANY, "Select Difficulty / Reference:");
    mainSizer->Add(fileLabel, 0, wxLEFT | wxRIGHT | wxTOP, 10);
    
    wxArrayString fileChoices;
    for (const auto& f : files) fileChoices.Add(f);
    
    fileChoice = new wxComboBox(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, fileChoices, wxCB_READONLY);
    if (!files.empty()) fileChoice->SetSelection(0);
    
    mainSizer->Add(fileChoice, 0, wxEXPAND | wxALL, 10);
    
    // Buttons
    wxSizer* buttonSizer = CreateButtonSizer(wxOK | wxCANCEL);
    mainSizer->Add(buttonSizer, 0, wxALIGN_RIGHT | wxALL, 10);
    
    SetSizer(mainSizer);
    Layout();
    Center();
}

ProjectSetupDialog::Action ProjectSetupDialog::GetSelectedAction() const
{
    return selectedAction;
}

std::string ProjectSetupDialog::GetSelectedFilename() const
{
    return selectedFilename;
}

void ProjectSetupDialog::OnRadioChange(wxCommandEvent& evt)
{
    int sel = actionRadio->GetSelection();
    if (sel == 0) selectedAction = Action::OpenExisting;
    else selectedAction = Action::StartFromScratch;
}

void ProjectSetupDialog::OnOK(wxCommandEvent& evt)
{
    // Store selection
    int sel = actionRadio->GetSelection();
    if (sel == 0) selectedAction = Action::OpenExisting;
    else selectedAction = Action::StartFromScratch;
    
    int fileSel = fileChoice->GetSelection();
    if (fileSel >= 0 && fileSel < files.size())
        selectedFilename = files[fileSel];
        
    evt.Skip(); // Allow dialog to close
}
