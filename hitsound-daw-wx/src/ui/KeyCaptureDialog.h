#pragma once
#include <wx/wx.h>

class KeyCaptureDialog : public wxDialog
{
public:
    KeyCaptureDialog(wxWindow* parent, const wxString& currentKey);

    wxAcceleratorEntry GetValue() const { return capturedEntry; }
    wxString GetKeyString() const;

private:
    void OnKeyDown(wxKeyEvent& evt);
    void OnCharHook(wxKeyEvent& evt);
    void OnOK(wxCommandEvent& evt);

    wxTextCtrl* keyDisplay;
    wxAcceleratorEntry capturedEntry;
    
    wxDECLARE_EVENT_TABLE();
};
