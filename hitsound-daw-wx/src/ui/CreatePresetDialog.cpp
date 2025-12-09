#include "CreatePresetDialog.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>

CreatePresetDialog::CreatePresetDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Create Preset", wxDefaultPosition, wxSize(350, 200), wxDEFAULT_DIALOG_STYLE)
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Placeholder message
    wxStaticText* label = new wxStaticText(this, wxID_ANY, 
        "Preset creation is not yet implemented.\n\nThis feature will allow you to save your current track layout as a reusable preset.",
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
    mainSizer->Add(label, 1, wxALL | wxALIGN_CENTER, 20);
    
    // Close button
    wxButton* closeBtn = new wxButton(this, wxID_CANCEL, "Close");
    mainSizer->Add(closeBtn, 0, wxALIGN_CENTER | wxBOTTOM, 15);
    
    SetSizerAndFit(mainSizer);
    Center();
}
