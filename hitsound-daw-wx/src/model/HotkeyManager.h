#pragma once
#include <wx/wx.h>
#include <map>
#include <vector>
#include <string>

class HotkeyManager
{
public:
    static HotkeyManager& Get();

    struct CommandInfo {
        int id;
        std::string name;
        std::string description;
        wxAcceleratorEntry defaultEntry;
    };

    void Initialize(const std::vector<CommandInfo>& commands);
    
    // Returns the current accelerator entry for a given command ID
    wxAcceleratorEntry GetAccelerator(int commandId) const;

    // Returns the default accelerator for a given command ID
    wxAcceleratorEntry GetDefaultAccelerator(int commandId) const;
    
    // Updates the binding for a command
    void SetAccelerator(int commandId, const wxAcceleratorEntry& entry);
    
    // Applies the current accelerator table to the window
    void ApplyTo(wxWindow* window);

    // Returns the display string (e.g., "Ctrl+Z") for a command
    wxString GetShortcutLabel(int commandId) const;
    
    const std::map<int, wxAcceleratorEntry>& GetBindings() const { return currentBindings; }
    const std::vector<CommandInfo>& GetCommands() const { return registeredCommands; }

private:
    HotkeyManager() = default;
    
    std::vector<CommandInfo> registeredCommands;
    std::map<int, wxAcceleratorEntry> currentBindings;
};
