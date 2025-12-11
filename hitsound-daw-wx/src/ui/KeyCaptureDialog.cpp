#include "KeyCaptureDialog.h"

wxBEGIN_EVENT_TABLE(KeyCaptureDialog, wxDialog)
    EVT_KEY_DOWN(KeyCaptureDialog::OnKeyDown)
    EVT_CHAR_HOOK(KeyCaptureDialog::OnCharHook)
    EVT_BUTTON(wxID_OK, KeyCaptureDialog::OnOK)
wxEND_EVENT_TABLE()

KeyCaptureDialog::KeyCaptureDialog(wxWindow* parent, const wxString& currentKey)
    : wxDialog(parent, wxID_ANY, "Press Keys...", wxDefaultPosition, wxSize(300, 150))
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    wxStaticText* lbl = new wxStaticText(this, wxID_ANY, "Press the desired key combination:");
    sizer->Add(lbl, 0, wxALL, 10);
    
    keyDisplay = new wxTextCtrl(this, wxID_ANY, currentKey, wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxTE_CENTER);
    wxFont font = keyDisplay->GetFont();
    font.SetPointSize(14);
    font.SetWeight(wxFONTWEIGHT_BOLD);
    keyDisplay->SetFont(font);
    
    sizer->Add(keyDisplay, 0, wxEXPAND | wxALL, 10);
    
    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* btnOK = new wxButton(this, wxID_OK, "OK");
    wxButton* btnCancel = new wxButton(this, wxID_CANCEL, "Cancel");
    
    btnSizer->Add(btnOK, 0, wxRIGHT, 5);
    btnSizer->Add(btnCancel, 0);
    
    sizer->Add(btnSizer, 0, wxALIGN_CENTER | wxALL, 10);
    
    SetSizer(sizer);
    CenterOnParent();
    
    // Parse initial string if possible to populate entry? 
    // Usually easier to start fresh or just display current
    capturedEntry.FromString(currentKey);
}

void KeyCaptureDialog::OnCharHook(wxKeyEvent& evt)
{
    // Capture events before they are processed by controls
    OnKeyDown(evt);
}

void KeyCaptureDialog::OnKeyDown(wxKeyEvent& evt)
{
    int keyCode = evt.GetKeyCode();
    int modifiers = evt.GetModifiers();
    
    // Ignore modifier-only key presses
    if (keyCode == WXK_SHIFT || keyCode == WXK_ALT || keyCode == WXK_CONTROL || keyCode == WXK_COMMAND)
        return;

    int flags = wxACCEL_NORMAL;
    if (modifiers & wxMOD_ALT) flags |= wxACCEL_ALT;
    if (modifiers & wxMOD_CONTROL) flags |= wxACCEL_CTRL;
    if (modifiers & wxMOD_SHIFT) flags |= wxACCEL_SHIFT;
    
    capturedEntry.Set(flags, keyCode, 0); // CMD ID irrelevant here
    
    keyDisplay->SetValue(capturedEntry.ToString());
}

void KeyCaptureDialog::OnOK(wxCommandEvent& evt)
{
    EndModal(wxID_OK);
}

wxString KeyCaptureDialog::GetKeyString() const
{
    return capturedEntry.ToString();
}
