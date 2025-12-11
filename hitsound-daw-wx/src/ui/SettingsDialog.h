#pragma once
#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/listctrl.h>

class SettingsDialog : public wxDialog
{
public:
    SettingsDialog(wxWindow* parent);

private:
    void CreateControls();
    void BuildThemeTab(wxWindow* parent);
    void BuildHotkeysTab(wxWindow* parent);
    
    void PopulateHotkeys();
    void OnHotkeySelected(wxListEvent& evt);
    void OnEditHotkey(wxCommandEvent& evt);
    void OnResetSelected(wxCommandEvent& evt);
    void OnResetDefaults(wxCommandEvent& evt);

    wxListCtrl* hotkeyList;
    wxButton* btnEditHotkey;
    wxButton* btnResetSelected;
    int selectedHotkeyIndex = -1;
    
    wxDECLARE_EVENT_TABLE();
};
