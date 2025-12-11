#include "HotkeyManager.h"

HotkeyManager& HotkeyManager::Get()
{
    static HotkeyManager instance;
    return instance;
}

void HotkeyManager::Initialize(const std::vector<CommandInfo>& commands)
{
    registeredCommands = commands;
    currentBindings.clear();
    
    for (const auto& cmd : commands)
    {
        currentBindings[cmd.id] = cmd.defaultEntry;
    }
}

wxAcceleratorEntry HotkeyManager::GetAccelerator(int commandId) const
{
    auto it = currentBindings.find(commandId);
    if (it != currentBindings.end())
        return it->second;
        
    return wxAcceleratorEntry();
}

wxAcceleratorEntry HotkeyManager::GetDefaultAccelerator(int commandId) const
{
    for (const auto& cmd : registeredCommands)
    {
        if (cmd.id == commandId)
        {
            return cmd.defaultEntry;
        }
    }
    return wxAcceleratorEntry(); 
}

void HotkeyManager::SetAccelerator(int commandId, const wxAcceleratorEntry& entry)
{
    currentBindings[commandId] = entry;
}

void HotkeyManager::ApplyTo(wxWindow* window)
{
    std::vector<wxAcceleratorEntry> entries;
    entries.reserve(currentBindings.size());
    
    for (const auto& pair : currentBindings)
    {
        // Only add if the binding is valid/active
        if (pair.second.GetKeyCode() != 0)
        {
            entries.push_back(pair.second);
        }
    }
    
    if (!entries.empty())
    {
        wxAcceleratorTable table(entries.size(), entries.data());
        window->SetAcceleratorTable(table);
    }
}

wxString HotkeyManager::GetShortcutLabel(int commandId) const
{
    auto it = currentBindings.find(commandId);
    if (it != currentBindings.end())
    {
        return it->second.ToString();
    }
    return "";
}
