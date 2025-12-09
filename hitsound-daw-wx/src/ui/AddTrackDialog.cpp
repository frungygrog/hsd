#include "AddTrackDialog.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/slider.h>
#include <wx/button.h>
#include <wx/textctrl.h>

wxBEGIN_EVENT_TABLE(AddTrackDialog, wxDialog)
    EVT_BUTTON(wxID_OK, AddTrackDialog::OnOK)
wxEND_EVENT_TABLE()

AddTrackDialog::AddTrackDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Add Track", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    
    result.confirmed = false;
    result.bank = SampleSet::Normal;
    result.type = SampleType::HitNormal;
    result.volume = 100;

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    
    wxBoxSizer* nameSizer = new wxBoxSizer(wxHORIZONTAL);
    nameSizer->Add(new wxStaticText(this, wxID_ANY, "Name:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    nameCtrl = new wxTextCtrl(this, wxID_ANY, "");
    nameSizer->Add(nameCtrl, 1, wxEXPAND | wxALL, 5);
    mainSizer->Add(nameSizer, 0, wxEXPAND | wxALL, 5);
    
    
    wxBoxSizer* bankSizer = new wxBoxSizer(wxHORIZONTAL);
    bankSizer->Add(new wxStaticText(this, wxID_ANY, "Bank:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    wxString banks[] = { "Normal", "Soft", "Drum" };
    bankChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 3, banks);
    bankChoice->SetSelection(0);
    bankSizer->Add(bankChoice, 1, wxEXPAND | wxALL, 5);
    mainSizer->Add(bankSizer, 0, wxEXPAND | wxALL, 10);
    
    
    wxBoxSizer* typeSizer = new wxBoxSizer(wxHORIZONTAL);
    typeSizer->Add(new wxStaticText(this, wxID_ANY, "Type:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    wxString types[] = { "Normal", "Whistle", "Clap", "Finish" };
    typeChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 4, types);
    typeChoice->SetSelection(0);
    typeSizer->Add(typeChoice, 1, wxEXPAND | wxALL, 5);
    mainSizer->Add(typeSizer, 0, wxEXPAND | wxALL, 10);
    
    
    mainSizer->Add(new wxStaticText(this, wxID_ANY, "Volume:"), 0, wxLEFT | wxTOP, 15);
    volumeSlider = new wxSlider(this, wxID_ANY, 100, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
    mainSizer->Add(volumeSlider, 0, wxEXPAND | wxALL, 10);
    
    
    mainSizer->AddStretchSpacer();
    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    
    okBtn = new wxButton(this, wxID_OK, "Add");
    wxButton* cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");
    
    btnSizer->AddStretchSpacer();
    btnSizer->Add(okBtn, 0, wxALL, 5);
    btnSizer->Add(cancelBtn, 0, wxALL, 5);
    
    mainSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 10);
    
    SetSizerAndFit(mainSizer);
    Center();
}

void AddTrackDialog::SetValues(const wxString& name, SampleSet bank, SampleType type, int volume)
{
    nameCtrl->SetValue(name);
    
    
    switch (bank) {
        case SampleSet::Normal: bankChoice->SetSelection(0); break;
        case SampleSet::Soft: bankChoice->SetSelection(1); break;
        case SampleSet::Drum: bankChoice->SetSelection(2); break;
    }
    
    
    switch (type) {
        case SampleType::HitNormal: typeChoice->SetSelection(0); break;
        case SampleType::HitWhistle: typeChoice->SetSelection(1); break;
        case SampleType::HitClap: typeChoice->SetSelection(2); break;
        case SampleType::HitFinish: typeChoice->SetSelection(3); break;
    }
    
    volumeSlider->SetValue(volume);
}

void AddTrackDialog::SetEditMode(bool edit)
{
    isEditMode = edit;
    if (edit) {
        SetTitle("Edit Track");
        okBtn->SetLabel("Save");
    } else {
        SetTitle("Add Track");
        okBtn->SetLabel("Add");
    }
}

void AddTrackDialog::OnOK(wxCommandEvent& evt)
{
    result.confirmed = true;
    result.name = nameCtrl->GetValue();
    
    int bankIdx = bankChoice->GetSelection();
    if (bankIdx == 0) result.bank = SampleSet::Normal;
    else if (bankIdx == 1) result.bank = SampleSet::Soft;
    else result.bank = SampleSet::Drum;
    
    int typeIdx = typeChoice->GetSelection();
    if (typeIdx == 0) result.type = SampleType::HitNormal;
    else if (typeIdx == 1) result.type = SampleType::HitWhistle;
    else if (typeIdx == 2) result.type = SampleType::HitClap;
    else result.type = SampleType::HitFinish;
    
    result.volume = volumeSlider->GetValue();
    
    evt.Skip(); 
}
