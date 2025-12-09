#include "CreatePresetDialog.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/msgdlg.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/file.h>
#include <wx/dir.h>
#include <sstream>

wxBEGIN_EVENT_TABLE(CreatePresetDialog, wxDialog)
    EVT_BUTTON(wxID_OK, CreatePresetDialog::OnOK)
wxEND_EVENT_TABLE()

wxString CreatePresetDialog::GetPresetsDirectory()
{
    
    wxString userDir = wxStandardPaths::Get().GetUserDataDir();
    wxString presetsDir = userDir + wxFileName::GetPathSeparator() + "Presets";
    
    
    if (!wxDirExists(presetsDir))
    {
        wxDir::Make(presetsDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    }
    
    return presetsDir;
}

CreatePresetDialog::CreatePresetDialog(wxWindow* parent, const std::vector<Track>& currentTracks)
    : wxDialog(parent, wxID_ANY, "Create Preset", wxDefaultPosition, wxSize(400, 200), wxDEFAULT_DIALOG_STYLE)
    , tracksToSave(currentTracks)
{
    result.confirmed = false;
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    
    wxStaticText* desc = new wxStaticText(this, wxID_ANY, 
        "Save your current track layout as a reusable preset.\nThe preset will include all tracks, groupings, and volume settings.");
    mainSizer->Add(desc, 0, wxALL, 15);
    
    
    wxBoxSizer* nameSizer = new wxBoxSizer(wxHORIZONTAL);
    nameSizer->Add(new wxStaticText(this, wxID_ANY, "Preset Name:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    
    nameInput = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(250, -1));
    nameSizer->Add(nameInput, 1, wxEXPAND);
    
    mainSizer->Add(nameSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 15);
    
    mainSizer->AddSpacer(20);
    
    
    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->AddStretchSpacer();
    btnSizer->Add(new wxButton(this, wxID_OK, "Save"), 0, wxRIGHT, 5);
    btnSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0);
    
    mainSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 15);
    
    SetSizerAndFit(mainSizer);
    Center();
    
    nameInput->SetFocus();
}

void CreatePresetDialog::OnOK(wxCommandEvent& evt)
{
    wxString name = nameInput->GetValue().Trim();
    
    if (name.IsEmpty())
    {
        wxMessageBox("Please enter a preset name.", "Error", wxICON_ERROR);
        return;
    }
    
    
    if (name.Contains("/") || name.Contains("\\") || name.Contains(":") || 
        name.Contains("*") || name.Contains("?") || name.Contains("\"") ||
        name.Contains("<") || name.Contains(">") || name.Contains("|"))
    {
        wxMessageBox("Preset name contains invalid characters.", "Error", wxICON_ERROR);
        return;
    }
    
    if (SavePreset(name, tracksToSave))
    {
        result.confirmed = true;
        result.presetName = name;
        EndModal(wxID_OK);
    }
}

bool CreatePresetDialog::SavePreset(const wxString& name, const std::vector<Track>& tracks)
{
    wxString presetsDir = GetPresetsDirectory();
    wxString filepath = presetsDir + wxFileName::GetPathSeparator() + name + ".preset";
    
    
    if (wxFileExists(filepath))
    {
        int answer = wxMessageBox("A preset with this name already exists. Overwrite?", 
                                  "Confirm Overwrite", wxYES_NO | wxICON_QUESTION);
        if (answer != wxYES)
            return false;
    }
    
    
    std::ostringstream ss;
    ss << "# Hitsound DAW Preset\n";
    ss << "# Created by hsd\n\n";
    
    
    for (const auto& track : tracks)
    {
        ss << "[Track]\n";
        ss << "name=" << track.name << "\n";
        ss << "sampleSet=" << static_cast<int>(track.sampleSet) << "\n";
        ss << "sampleType=" << static_cast<int>(track.sampleType) << "\n";
        ss << "gain=" << track.gain << "\n";
        ss << "isGrouping=" << (track.isGrouping ? 1 : 0) << "\n";
        ss << "isExpanded=" << (track.isExpanded ? 1 : 0) << "\n";
        ss << "primaryChildIndex=" << track.primaryChildIndex << "\n";
        
        
        if (!track.layers.empty())
        {
            ss << "layers=";
            for (size_t i = 0; i < track.layers.size(); ++i)
            {
                if (i > 0) ss << ";";
                ss << static_cast<int>(track.layers[i].bank) << "," << static_cast<int>(track.layers[i].type);
            }
            ss << "\n";
        }
        
        
        for (const auto& child : track.children)
        {
            ss << "[Child]\n";
            ss << "name=" << child.name << "\n";
            ss << "sampleSet=" << static_cast<int>(child.sampleSet) << "\n";
            ss << "sampleType=" << static_cast<int>(child.sampleType) << "\n";
            ss << "gain=" << child.gain << "\n";
            
            if (!child.layers.empty())
            {
                ss << "layers=";
                for (size_t i = 0; i < child.layers.size(); ++i)
                {
                    if (i > 0) ss << ";";
                    ss << static_cast<int>(child.layers[i].bank) << "," << static_cast<int>(child.layers[i].type);
                }
                ss << "\n";
            }
            ss << "[/Child]\n";
        }
        
        ss << "[/Track]\n\n";
    }
    
    
    wxFile file(filepath, wxFile::write);
    if (!file.IsOpened())
    {
        wxMessageBox("Failed to create preset file.", "Error", wxICON_ERROR);
        return false;
    }
    
    std::string content = ss.str();
    file.Write(content.c_str(), content.size());
    file.Close();
    
    wxMessageBox(wxString::Format("Preset '%s' saved successfully!", name), "Success", wxICON_INFORMATION);
    return true;
}
