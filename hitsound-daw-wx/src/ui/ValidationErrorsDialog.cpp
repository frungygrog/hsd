#include "ValidationErrorsDialog.h"
#include <algorithm>
#include <iomanip>
#include <sstream>

ValidationErrorsDialog::ValidationErrorsDialog(wxWindow* parent, const std::vector<ProjectValidator::ValidationError>& errors)
    : wxDialog(parent, wxID_ANY, "Validation Errors", wxDefaultPosition, wxSize(500, 400), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Info Text
    wxStaticText* info = new wxStaticText(this, wxID_ANY, 
        "The following issues were found in your project.\nSaving with these errors may result in unexpected playback in osu!.\n"
        "It is recommended to resolve them before saving.");
    mainSizer->Add(info, 0, wxALL | wxEXPAND, 10);
    
    // List Control
    listCtrl = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SUNKEN);
    listCtrl->InsertColumn(0, "Time", wxLIST_FORMAT_LEFT, 80);
    listCtrl->InsertColumn(1, "Error Message", wxLIST_FORMAT_LEFT, 350);
    
    int index = 0;
    for (const auto& err : errors)
    {
        std::stringstream timeStr;
        // Format time as MM:SS.ms
        int totalMs = (int)(err.time * 1000);
        int mins = totalMs / 60000;
        int secs = (totalMs % 60000) / 1000;
        int ms = totalMs % 1000;
        
        timeStr << std::setfill('0') << std::setw(2) << mins << ":" 
                << std::setw(2) << secs << "." 
                << std::setw(3) << ms;
                
        long item = listCtrl->InsertItem(index, timeStr.str());
        listCtrl->SetItem(item, 1, err.message);
        
        // Optional: color items? wxListCtrl isn't great at simple coloring without custom draw.
        // We'll stick to text.
        index++;
    }
    
    mainSizer->Add(listCtrl, 1, wxALL | wxEXPAND, 10);
    
    // Buttons
    wxStdDialogButtonSizer* btnSizer = new wxStdDialogButtonSizer();
    
    wxButton* btnIgnore = new wxButton(this, wxID_ANY, "Ignore && Save"); 
    wxButton* btnCancel = new wxButton(this, wxID_ANY, "Cancel");
    
    // Use custom handling
    btnIgnore->Bind(wxEVT_BUTTON, &ValidationErrorsDialog::OnIgnore, this);
    btnCancel->Bind(wxEVT_BUTTON, &ValidationErrorsDialog::OnCancel, this);
    
    btnSizer->AddButton(btnIgnore);
    btnSizer->AddButton(btnCancel);
    btnSizer->Realize();
    
    mainSizer->Add(btnSizer, 0, wxALL | wxALIGN_RIGHT, 10);
    
    SetSizer(mainSizer);
    Layout();
    CenterOnParent();
}

void ValidationErrorsDialog::OnIgnore(wxCommandEvent& evt)
{
    ignored = true;
    EndModal(wxID_OK);
}

void ValidationErrorsDialog::OnCancel(wxCommandEvent& evt)
{
    ignored = false;
    EndModal(wxID_CANCEL);
}
