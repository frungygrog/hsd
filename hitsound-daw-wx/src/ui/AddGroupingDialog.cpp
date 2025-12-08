#include "AddGroupingDialog.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/slider.h>
#include <wx/button.h>
#include <wx/textctrl.h>

wxBEGIN_EVENT_TABLE(AddGroupingDialog, wxDialog)
    EVT_BUTTON(wxID_OK, AddGroupingDialog::OnOK)
wxEND_EVENT_TABLE()

AddGroupingDialog::AddGroupingDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Add Grouping", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    // Initialize result
    result.confirmed = false;
    result.volume = 100;

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Name
    wxBoxSizer* nameSizer = new wxBoxSizer(wxHORIZONTAL);
    nameSizer->Add(new wxStaticText(this, wxID_ANY, "Name:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    nameCtrl = new wxTextCtrl(this, wxID_ANY, "New Grouping");
    nameSizer->Add(nameCtrl, 1, wxEXPAND | wxALL, 5);
    mainSizer->Add(nameSizer, 0, wxEXPAND | wxALL, 5);

    wxArrayString banks;
    banks.Add("Normal");
    banks.Add("Soft");
    banks.Add("Drum");

    // --- HitNormal Section ---
    wxStaticBoxSizer* normalBox = new wxStaticBoxSizer(wxVERTICAL, this, "HitNormal");
    wxBoxSizer* nbSizer = new wxBoxSizer(wxHORIZONTAL);
    nbSizer->Add(new wxStaticText(this, wxID_ANY, "Bank:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    normalBankChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, banks);
    normalBankChoice->SetSelection(0); // Default Normal
    nbSizer->Add(normalBankChoice, 1, wxEXPAND | wxALL, 5);
    normalBox->Add(nbSizer, 0, wxEXPAND | wxALL, 0);
    // Note: HitNormal is always included.
    normalBox->Add(new wxStaticText(this, wxID_ANY, "(Always Included)"), 0, wxALIGN_LEFT | wxALL, 5);
    
    mainSizer->Add(normalBox, 0, wxEXPAND | wxALL, 5);
    
    // --- Additions Section ---
    wxStaticBoxSizer* addBox = new wxStaticBoxSizer(wxVERTICAL, this, "Additions");
    
    // Bank
    wxBoxSizer* abSizer = new wxBoxSizer(wxHORIZONTAL);
    abSizer->Add(new wxStaticText(this, wxID_ANY, "Bank:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    additionsBankChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, banks);
    additionsBankChoice->SetSelection(1); // Default Soft? Or Normal? Let's obey user preference or default.
    abSizer->Add(additionsBankChoice, 1, wxEXPAND | wxALL, 5);
    addBox->Add(abSizer, 0, wxEXPAND | wxALL, 0);
    
    // Checkboxes
    wxBoxSizer* cbSizer = new wxBoxSizer(wxHORIZONTAL);
    chkWhistle = new wxCheckBox(this, wxID_ANY, "Whistle");
    chkFinish = new wxCheckBox(this, wxID_ANY, "Finish");
    chkClap = new wxCheckBox(this, wxID_ANY, "Clap");
    
    cbSizer->Add(chkWhistle, 1, wxALL, 5);
    cbSizer->Add(chkFinish, 1, wxALL, 5);
    cbSizer->Add(chkClap, 1, wxALL, 5);
    addBox->Add(cbSizer, 0, wxEXPAND | wxALL, 0);
    
    mainSizer->Add(addBox, 0, wxEXPAND | wxALL, 5);
    
    // Volume
    mainSizer->Add(new wxStaticText(this, wxID_ANY, "Volume:"), 0, wxLEFT | wxTOP, 10);
    volumeSlider = new wxSlider(this, wxID_ANY, 100, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
    mainSizer->Add(volumeSlider, 0, wxEXPAND | wxALL, 10);
    
    // Buttons
    mainSizer->AddStretchSpacer();
    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    
    wxButton* okBtn = new wxButton(this, wxID_OK, "Add");
    wxButton* cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");
    
    btnSizer->AddStretchSpacer();
    btnSizer->Add(okBtn, 0, wxALL, 5);
    btnSizer->Add(cancelBtn, 0, wxALL, 5);
    
    mainSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 10);
    
    SetSizerAndFit(mainSizer);
    Center();
}

void AddGroupingDialog::OnOK(wxCommandEvent& evt)
{
    result.confirmed = true;
    result.name = nameCtrl->GetValue();
    if (result.name.IsEmpty()) result.name = "Group";

    // Helper to map index to SampleSet
    auto getBank = [](int idx) {
        if (idx == 1) return SampleSet::Soft;
        if (idx == 2) return SampleSet::Drum;
        return SampleSet::Normal;
    };

    result.hitNormalBank = getBank(normalBankChoice->GetSelection());
    result.additionsBank = getBank(additionsBankChoice->GetSelection());
    
    result.hasWhistle = chkWhistle->GetValue();
    result.hasFinish = chkFinish->GetValue();
    result.hasClap = chkClap->GetValue();
    
    result.volume = volumeSlider->GetValue();
    
    evt.Skip(); 
}
