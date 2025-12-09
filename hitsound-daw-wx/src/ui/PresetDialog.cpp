#include "PresetDialog.h"
#include "CreatePresetDialog.h"  
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/file.h>
#include <wx/textfile.h>
#include <sstream>

wxBEGIN_EVENT_TABLE(PresetDialog, wxDialog)
    EVT_BUTTON(wxID_OK, PresetDialog::OnOK)
wxEND_EVENT_TABLE()

PresetDialog::PresetDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Load Preset", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
    result.confirmed = false;
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    
    wxStaticText* desc = new wxStaticText(this, wxID_ANY, 
        "Select a preset to add tracks and groupings to your project:");
    mainSizer->Add(desc, 0, wxALL, 10);
    
    
    RefreshPresetList();
    
    
    wxBoxSizer* choiceSizer = new wxBoxSizer(wxHORIZONTAL);
    choiceSizer->Add(new wxStaticText(this, wxID_ANY, "Preset:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    
    presetChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(200, -1), presetNames);
    if (!presetNames.IsEmpty())
        presetChoice->SetSelection(0);
    choiceSizer->Add(presetChoice, 1, wxEXPAND);
    
    mainSizer->Add(choiceSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    
    mainSizer->AddSpacer(20);
    
    
    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->AddStretchSpacer();
    btnSizer->Add(new wxButton(this, wxID_OK, "Load"), 0, wxRIGHT, 5);
    btnSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0);
    
    mainSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 10);
    
    SetSizerAndFit(mainSizer);
    Center();
}

void PresetDialog::RefreshPresetList()
{
    presetNames.Clear();
    presetPaths.Clear();
    
    
    presetNames.Add("Generic (Built-in)");
    presetPaths.Add("");  
    
    
    wxString presetsDir = CreatePresetDialog::GetPresetsDirectory();
    if (wxDirExists(presetsDir))
    {
        wxDir dir(presetsDir);
        if (dir.IsOpened())
        {
            wxString filename;
            bool cont = dir.GetFirst(&filename, "*.preset", wxDIR_FILES);
            while (cont)
            {
                wxString nameWithoutExt = wxFileName(filename).GetName();
                presetNames.Add(nameWithoutExt);
                presetPaths.Add(presetsDir + wxFileName::GetPathSeparator() + filename);
                cont = dir.GetNext(&filename);
            }
        }
    }
}

void PresetDialog::OnOK(wxCommandEvent& evt)
{
    int sel = presetChoice->GetSelection();
    if (sel < 0 || sel >= (int)presetNames.GetCount())
        return;
    
    result.confirmed = true;
    result.presetName = presetNames[sel];
    result.isBuiltIn = presetPaths[sel].IsEmpty();
    
    evt.Skip();
}

std::vector<Track> PresetDialog::LoadPresetFromFile(const wxString& filepath)
{
    std::vector<Track> tracks;
    
    wxTextFile file;
    if (!file.Open(filepath))
        return tracks;
    
    Track* currentTrack = nullptr;
    Track* currentChild = nullptr;
    bool inChild = false;
    
    for (wxString line = file.GetFirstLine(); !file.Eof(); line = file.GetNextLine())
    {
        line = line.Trim();
        
        if (line.IsEmpty() || line.StartsWith("#"))
            continue;
        
        if (line == "[Track]")
        {
            tracks.push_back(Track());
            currentTrack = &tracks.back();
            currentChild = nullptr;
            inChild = false;
            continue;
        }
        
        if (line == "[/Track]")
        {
            currentTrack = nullptr;
            continue;
        }
        
        if (line == "[Child]")
        {
            if (currentTrack)
            {
                currentTrack->children.push_back(Track());
                currentChild = &currentTrack->children.back();
                currentChild->isChildTrack = true;
                inChild = true;
            }
            continue;
        }
        
        if (line == "[/Child]")
        {
            currentChild = nullptr;
            inChild = false;
            continue;
        }
        
        
        int eqPos = line.Find('=');
        if (eqPos == wxNOT_FOUND)
            continue;
        
        wxString key = line.Left(eqPos);
        wxString value = line.Mid(eqPos + 1);
        
        Track* target = inChild ? currentChild : currentTrack;
        if (!target)
            continue;
        
        if (key == "name")
            target->name = value.ToStdString();
        else if (key == "sampleSet")
            target->sampleSet = static_cast<SampleSet>(wxAtoi(value));
        else if (key == "sampleType")
            target->sampleType = static_cast<SampleType>(wxAtoi(value));
        else if (key == "gain")
            target->gain = wxAtof(value);
        else if (key == "isGrouping")
            target->isGrouping = (wxAtoi(value) != 0);
        else if (key == "isExpanded")
            target->isExpanded = (wxAtoi(value) != 0);
        else if (key == "primaryChildIndex")
            target->primaryChildIndex = wxAtoi(value);
        else if (key == "layers")
        {
            
            wxArrayString layerPairs;
            wxString layersStr = value;
            while (!layersStr.IsEmpty())
            {
                int semiPos = layersStr.Find(';');
                wxString pair = (semiPos != wxNOT_FOUND) ? layersStr.Left(semiPos) : layersStr;
                
                int commaPos = pair.Find(',');
                if (commaPos != wxNOT_FOUND)
                {
                    SampleLayer layer;
                    layer.bank = static_cast<SampleSet>(wxAtoi(pair.Left(commaPos)));
                    layer.type = static_cast<SampleType>(wxAtoi(pair.Mid(commaPos + 1)));
                    target->layers.push_back(layer);
                }
                
                if (semiPos != wxNOT_FOUND)
                    layersStr = layersStr.Mid(semiPos + 1);
                else
                    break;
            }
        }
    }
    
    file.Close();
    return tracks;
}
