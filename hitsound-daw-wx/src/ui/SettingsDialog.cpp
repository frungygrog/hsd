#include "SettingsDialog.h"
#include "../model/HotkeyManager.h"
#include "MainFrame.h" // For IDs
#include "KeyCaptureDialog.h"

enum {
    ID_HOTKEY_LIST = 20001,
    ID_BTN_EDIT_HOTKEY,
    ID_BTN_RESET_SELECTED,
    ID_BTN_RESET_DEFAULTS
};

wxBEGIN_EVENT_TABLE(SettingsDialog, wxDialog)
    EVT_LIST_ITEM_SELECTED(ID_HOTKEY_LIST, SettingsDialog::OnHotkeySelected)
    EVT_BUTTON(ID_BTN_EDIT_HOTKEY, SettingsDialog::OnEditHotkey)
    EVT_BUTTON(ID_BTN_RESET_SELECTED, SettingsDialog::OnResetSelected)
    EVT_BUTTON(ID_BTN_RESET_DEFAULTS, SettingsDialog::OnResetDefaults)
wxEND_EVENT_TABLE()

SettingsDialog::SettingsDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Settings", wxDefaultPosition, wxSize(600, 450), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    CreateControls();
    CenterOnParent();
}

void SettingsDialog::CreateControls()
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    wxNotebook* notebook = new wxNotebook(this, wxID_ANY);
    
    wxPanel* themePanel = new wxPanel(notebook);
    BuildThemeTab(themePanel);
    notebook->AddPage(themePanel, "Theme");
    
    wxPanel* hotkeysPanel = new wxPanel(notebook);
    BuildHotkeysTab(hotkeysPanel);
    notebook->AddPage(hotkeysPanel, "Hotkeys");
    
    mainSizer->Add(notebook, 1, wxEXPAND | wxALL, 5);
    
    // Close button at bottom
    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* btnClose = new wxButton(this, wxID_OK, "Close");
    btnSizer->Add(btnClose, 0);
    mainSizer->Add(btnSizer, 0, wxALIGN_RIGHT | wxALL, 10);
    
    SetSizer(mainSizer);
}

void SettingsDialog::BuildThemeTab(wxWindow* parent)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    wxStaticText* lblInfo = new wxStaticText(parent, wxID_ANY, "Theme selection coming soon...");
    
    wxStaticBoxSizer* group = new wxStaticBoxSizer(wxVERTICAL, parent, "Appearance");
    wxRadioButton* radLight = new wxRadioButton(parent, wxID_ANY, "Light Theme", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    wxRadioButton* radDark = new wxRadioButton(parent, wxID_ANY, "Dark Theme (Default)");
    
    radDark->SetValue(true);
    radLight->Enable(false); // Placeholder
    
    group->Add(radLight, 0, wxALL, 5);
    group->Add(radDark, 0, wxALL, 5);
    
    sizer->Add(lblInfo, 0, wxALL, 10);
    sizer->Add(group, 0, wxEXPAND | wxALL, 10);
    
    parent->SetSizer(sizer);
}

void SettingsDialog::BuildHotkeysTab(wxWindow* parent)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    hotkeyList = new wxListCtrl(parent, ID_HOTKEY_LIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    hotkeyList->InsertColumn(0, "Command", wxLIST_FORMAT_LEFT, 200);
    hotkeyList->InsertColumn(1, "Key Binding", wxLIST_FORMAT_LEFT, 150);
    hotkeyList->InsertColumn(2, "Description", wxLIST_FORMAT_LEFT, 250); // Added description column
    
    PopulateHotkeys();
    
    sizer->Add(hotkeyList, 1, wxEXPAND | wxALL, 5);
    
    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnEditHotkey = new wxButton(parent, ID_BTN_EDIT_HOTKEY, "Edit Binding...");
    btnResetSelected = new wxButton(parent, ID_BTN_RESET_SELECTED, "Reset Binding"); // New button
    
    btnEditHotkey->Enable(false);
    btnResetSelected->Enable(false);
    
    wxButton* btnReset = new wxButton(parent, ID_BTN_RESET_DEFAULTS, "Reset All to Defaults");
    
    btnSizer->Add(btnEditHotkey, 0, wxRIGHT, 5);
    btnSizer->Add(btnResetSelected, 0, wxRIGHT, 5);
    btnSizer->AddStretchSpacer(1);
    btnSizer->Add(btnReset, 0);
    
    sizer->Add(btnSizer, 0, wxEXPAND | wxALL, 5);
    
    parent->SetSizer(sizer);
}

void SettingsDialog::PopulateHotkeys()
{
    hotkeyList->DeleteAllItems();
    
    auto& mgr = HotkeyManager::Get();
    const auto& commands = mgr.GetCommands();
    
    for (const auto& cmd : commands)
    {
        long index = hotkeyList->InsertItem(hotkeyList->GetItemCount(), cmd.name);
        
        wxString shortcut = mgr.GetShortcutLabel(cmd.id);
        hotkeyList->SetItem(index, 1, shortcut);
        hotkeyList->SetItem(index, 2, cmd.description); // Set description
        
        hotkeyList->SetItemData(index, cmd.id);
    }
}

void SettingsDialog::OnHotkeySelected(wxListEvent& evt)
{
    selectedHotkeyIndex = evt.GetIndex();
    bool hasSel = selectedHotkeyIndex != -1;
    btnEditHotkey->Enable(hasSel);
    btnResetSelected->Enable(hasSel);
}

void SettingsDialog::OnEditHotkey(wxCommandEvent& evt)
{
    if (selectedHotkeyIndex == -1) return;
    
    int commandId = (int)hotkeyList->GetItemData(selectedHotkeyIndex);
    
    wxString currentKey = hotkeyList->GetItemText(selectedHotkeyIndex, 1);
    
    // Use KeyCaptureDialog instead of TextEntryDialog
    KeyCaptureDialog dlg(this, currentKey);
    
    if (dlg.ShowModal() == wxID_OK)
    {
        wxAcceleratorEntry entry = dlg.GetValue();
        entry.Set(entry.GetFlags(), entry.GetKeyCode(), commandId); // Ensure ID is correct
        
        HotkeyManager::Get().SetAccelerator(commandId, entry);
        
        // Refresh list
        hotkeyList->SetItem(selectedHotkeyIndex, 1, HotkeyManager::Get().GetShortcutLabel(commandId));
        
        // Apply immediately to parent
        if (GetParent())
            HotkeyManager::Get().ApplyTo(GetParent());
    }
}

void SettingsDialog::OnResetSelected(wxCommandEvent& evt)
{
    if (selectedHotkeyIndex == -1) return;
    
    int commandId = (int)hotkeyList->GetItemData(selectedHotkeyIndex);
    
    wxAcceleratorEntry defaultEntry = HotkeyManager::Get().GetDefaultAccelerator(commandId);
    HotkeyManager::Get().SetAccelerator(commandId, defaultEntry);
    
    // Refresh list
    hotkeyList->SetItem(selectedHotkeyIndex, 1, HotkeyManager::Get().GetShortcutLabel(commandId));
    
    // Apply immediately to parent
    if (GetParent())
        HotkeyManager::Get().ApplyTo(GetParent());
}

void SettingsDialog::OnResetDefaults(wxCommandEvent& evt)
{
    // Re-initialize with original defaults
    // In a real app we'd probably separate "LoadDefaults" logic.
    // For now, we assume MainFrame setup will be re-run or we can just iterate defaults:
    
    auto& mgr = HotkeyManager::Get();
    // This is destructive if we modified 'registeredCommands' in place, but we didn't.
    // However, HotkeyManager::Initialize resets to defaults stored in CommandInfo.
    
    // So we just call Initialize again with the same list? 
    // Wait, MainFrame typically initializes it.
    // Let's just manually reset for now for the items we know.
    
    // Better approach: HotkeyManager::ResetToDefaults();
    // I'll add a quick local loop since I can access the defaults map from the manager if I exposed it, or just use registeredCommands.
    
    for (const auto& cmd : mgr.GetCommands())
    {
        mgr.SetAccelerator(cmd.id, cmd.defaultEntry);
    }
    
    PopulateHotkeys();
    if (GetParent()) mgr.ApplyTo(GetParent());
}
