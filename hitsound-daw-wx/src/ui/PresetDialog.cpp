#include "PresetDialog.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>

wxBEGIN_EVENT_TABLE(PresetDialog, wxDialog)
    EVT_BUTTON(wxID_OK, PresetDialog::OnOK)
wxEND_EVENT_TABLE()

PresetDialog::PresetDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Load Preset", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
    result.confirmed = false;
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Description
    wxStaticText* desc = new wxStaticText(this, wxID_ANY, 
        "Select a preset to add tracks and groupings to your project:");
    mainSizer->Add(desc, 0, wxALL, 10);
    
    // Preset Dropdown
    wxBoxSizer* choiceSizer = new wxBoxSizer(wxHORIZONTAL);
    choiceSizer->Add(new wxStaticText(this, wxID_ANY, "Preset:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    
    wxArrayString presets;
    presets.Add("Generic");
    // Future presets can be added here
    
    presetChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(200, -1), presets);
    presetChoice->SetSelection(0);
    choiceSizer->Add(presetChoice, 1, wxEXPAND);
    
    mainSizer->Add(choiceSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    
    mainSizer->AddSpacer(20);
    
    // Buttons
    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->AddStretchSpacer();
    btnSizer->Add(new wxButton(this, wxID_OK, "Load"), 0, wxRIGHT, 5);
    btnSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0);
    
    mainSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 10);
    
    SetSizerAndFit(mainSizer);
    Center();
}

void PresetDialog::OnOK(wxCommandEvent& evt)
{
    result.confirmed = true;
    result.presetName = presetChoice->GetStringSelection();
    evt.Skip();
}
