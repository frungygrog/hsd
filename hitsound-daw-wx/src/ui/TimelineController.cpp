#include "TimelineController.h"
#include "../model/Commands.h"
#include "../model/ProjectValidator.h"
#include <algorithm>
#include <functional>

TimelineController::TimelineController()
{
}

void TimelineController::SetProject(Project* p)
{
    project = p;
    selection.clear();
    baseSelection.clear();
    lastFocusedTrackId = 0;
}

// -----------------------------------------------------------------------------
// Selection
// -----------------------------------------------------------------------------

void TimelineController::SelectEvent(uint64_t trackId, uint64_t eventId, bool addToSelection)
{
    if (!addToSelection)
        selection.clear();
    selection.insert({trackId, eventId});
}

void TimelineController::DeselectEvent(uint64_t trackId, uint64_t eventId)
{
    selection.erase({trackId, eventId});
}

void TimelineController::ClearSelection()
{
    selection.clear();
}

void TimelineController::SelectAll()
{
    selection.clear();
    std::vector<Track*> visible = GetVisibleTracks();
    
    for (Track* t : visible)
    {
        if (!t->isExpanded && !t->children.empty())
        {
            for (Track& child : t->children)
            {
                for (const auto& evt : child.events)
                    selection.insert({child.id, evt.id});
            }
        }
        else
        {
            for (const auto& evt : t->events)
                selection.insert({t->id, evt.id});
        }
    }
    
    if (OnDataChanged) OnDataChanged();
}

void TimelineController::SetSelection(const std::set<std::pair<uint64_t, uint64_t>>& newSelection)
{
    selection = newSelection;
}

bool TimelineController::IsSelected(uint64_t trackId, uint64_t eventId) const
{
    return selection.count({trackId, eventId}) > 0;
}

void TimelineController::SaveBaseSelection()
{
    baseSelection = selection;
}

void TimelineController::RestoreBaseSelection()
{
    selection = baseSelection;
}

void TimelineController::MergeWithBaseSelection(const std::set<std::pair<uint64_t, uint64_t>>& newItems, bool toggle)
{
    selection = baseSelection;
    for (const auto& item : newItems)
    {
        if (toggle && baseSelection.count(item))
            selection.erase(item);
        else
            selection.insert(item);
    }
}

// -----------------------------------------------------------------------------
// Clipboard
// -----------------------------------------------------------------------------

int TimelineController::FindRowIndex(Track* t, const std::vector<Track*>& visible)
{
    for (int i = 0; i < (int)visible.size(); ++i)
    {
        if (visible[i] == t) return i;
        if (!visible[i]->isExpanded)
        {
            for (const auto& c : visible[i]->children)
            {
                if (&c == t) return i;
            }
        }
    }
    return 0;
}

void TimelineController::CopySelection(const std::vector<Track*>& visibleTracks)
{
    if (selection.empty()) return;
    
    clipboard.clear();
    
    double minTime = std::numeric_limits<double>::max();
    int minRow = std::numeric_limits<int>::max();
    
    // First pass: find min time and row
    for (const auto& sel : selection)
    {
        Track* track = FindTrackById(sel.first);
        if (!track) continue;
        
        for (const auto& evt : track->events)
        {
            if (evt.id == sel.second)
            {
                int row = FindRowIndex(track, visibleTracks);
                if (evt.time < minTime) minTime = evt.time;
                if (row < minRow) minRow = row;
                break;
            }
        }
    }
    
    // Second pass: build clipboard items
    for (const auto& sel : selection)
    {
        Track* track = FindTrackById(sel.first);
        if (!track) continue;
        
        for (const auto& evt : track->events)
        {
            if (evt.id == sel.second)
            {
                int row = FindRowIndex(track, visibleTracks);
                
                ClipboardItem item;
                item.evt = evt;
                item.relativeRow = row - minRow;
                item.relativeTime = evt.time - minTime;
                clipboard.push_back(item);
                break;
            }
        }
    }
}

void TimelineController::CutSelection(const std::vector<Track*>& visibleTracks)
{
    CopySelection(visibleTracks);
    DeleteSelection();
}

void TimelineController::PasteAtPlayhead(double playheadTime, const std::vector<Track*>& visibleTracks)
{
    if (clipboard.empty()) return;
    
    int anchorRow = 0;
    Track* lastFocused = FindTrackById(lastFocusedTrackId);
    if (lastFocused)
    {
        for (int i = 0; i < (int)visibleTracks.size(); ++i)
        {
            if (visibleTracks[i] == lastFocused ||
                (!visibleTracks[i]->isExpanded && std::find_if(visibleTracks[i]->children.begin(), visibleTracks[i]->children.end(),
                    [lastFocused](const Track& c){ return &c == lastFocused; }) != visibleTracks[i]->children.end()))
            {
                anchorRow = i;
                break;
            }
        }
    }
    
    std::vector<PasteEventsCommand::PasteItem> itemsToPaste;
    
    for (const auto& ci : clipboard)
    {
        int targetRow = anchorRow + ci.relativeRow;
        if (targetRow < 0) targetRow = 0;
        if (targetRow >= (int)visibleTracks.size()) targetRow = (int)visibleTracks.size() - 1;
        
        Track* visualTarget = visibleTracks[targetRow];
        Track* actualTarget = GetEffectiveTargetTrack(visualTarget);
        if (!actualTarget) continue;
        
        Event newEvt = ci.evt;
        newEvt.time = playheadTime + ci.relativeTime;
        
        itemsToPaste.push_back({actualTarget, newEvt});
    }
    
    if (!itemsToPaste.empty())
    {
        auto refreshFn = [this](){ ValidateHitsounds(); if (OnDataChanged) OnDataChanged(); };
        undoManager.PushCommand(std::make_unique<PasteEventsCommand>(itemsToPaste, 
            [](const std::vector<Track*>&){}, refreshFn));
    }
}

void TimelineController::DeleteSelection()
{
    if (selection.empty()) return;
    
    std::vector<RemoveEventsCommand::Item> items;
    for (const auto& sel : selection)
    {
        Track* track = FindTrackById(sel.first);
        if (!track) continue;
        
        for (const auto& evt : track->events)
        {
            if (evt.id == sel.second)
            {
                items.push_back({track, evt});
                break;
            }
        }
    }
    
    auto refreshFn = [this](){ selection.clear(); ValidateHitsounds(); if (OnDataChanged) OnDataChanged(); };
    undoManager.PushCommand(std::make_unique<RemoveEventsCommand>(items, refreshFn));
}

// -----------------------------------------------------------------------------
// Event Placement
// -----------------------------------------------------------------------------

void TimelineController::PlaceEvent(Track* target, double time, std::optional<SampleSet> defaultHitnormalBank)
{
    if (!target || !project) return;
    
    Event newEvt;
    newEvt.time = time;
    newEvt.volume = target->gain;
    
    bool isAddition = (target->sampleType != SampleType::HitNormal);
    
    if (isAddition && defaultHitnormalBank.has_value())
    {
        // Auto-hitnormal logic
        bool hitnormalExists = false;
        for (auto& t : project->tracks)
        {
            if (t.sampleType == SampleType::HitNormal)
            {
                for (auto& child : t.children)
                {
                    for (const auto& e : child.events)
                    {
                        if (std::abs(e.time - time) < 0.0005)
                        {
                            hitnormalExists = true;
                            break;
                        }
                    }
                    if (hitnormalExists) break;
                }
            }
            if (hitnormalExists) break;
        }
        
        if (!hitnormalExists)
        {
            Track* hnTrack = FindOrCreateHitnormalTrack(*defaultHitnormalBank, target->gain);
            if (hnTrack)
            {
                Event hnEvt;
                hnEvt.time = time;
                hnEvt.volume = target->gain;
                
                std::vector<AddMultipleEventsCommand::Item> items = {
                    {hnTrack, hnEvt},
                    {target, newEvt}
                };
                
                auto refreshFn = [this](){ ValidateHitsounds(); if (OnDataChanged) OnDataChanged(); if (OnTracksModified) OnTracksModified(); };
                undoManager.PushCommand(std::make_unique<AddMultipleEventsCommand>(items, refreshFn));
                return;
            }
        }
    }
    
    // Normal single event placement
    auto refreshFn = [this](){ ValidateHitsounds(); if (OnDataChanged) OnDataChanged(); };
    undoManager.PushCommand(std::make_unique<AddEventCommand>(target, newEvt, refreshFn));
}

// -----------------------------------------------------------------------------
// Undo/Redo
// -----------------------------------------------------------------------------

void TimelineController::Undo()
{
    undoManager.Undo();
    if (OnDataChanged) OnDataChanged();
}

void TimelineController::Redo()
{
    undoManager.Redo();
    if (OnDataChanged) OnDataChanged();
}

bool TimelineController::CanUndo() const
{
    return undoManager.CanUndo();
}

bool TimelineController::CanRedo() const
{
    return undoManager.CanRedo();
}

std::string TimelineController::GetUndoDescription() const
{
    return undoManager.GetUndoDescription();
}

std::string TimelineController::GetRedoDescription() const
{
    return undoManager.GetRedoDescription();
}

// -----------------------------------------------------------------------------
// Dirty State
// -----------------------------------------------------------------------------

void TimelineController::MarkClean()
{
    undoManager.MarkClean();
}

bool TimelineController::IsDirty() const
{
    return undoManager.IsDirty();
}

// -----------------------------------------------------------------------------
// Validation
// -----------------------------------------------------------------------------

void TimelineController::ValidateHitsounds()
{
    if (project)
    {
        ProjectValidator::Validate(*project);
    }
}

// -----------------------------------------------------------------------------
// Track Helpers
// -----------------------------------------------------------------------------

Track* TimelineController::FindTrackById(uint64_t id)
{
    if (!project || id == 0) return nullptr;
    
    std::function<Track*(std::vector<Track>&)> search = [&](std::vector<Track>& tracks) -> Track* {
        for (auto& t : tracks)
        {
            if (t.id == id) return &t;
            Track* found = search(t.children);
            if (found) return found;
        }
        return nullptr;
    };
    
    return search(project->tracks);
}

Track* TimelineController::FindOrCreateHitnormalTrack(SampleSet bank, double volume)
{
    if (!project) return nullptr;
    
    // Look for existing track with matching bank and volume
    for (auto& t : project->tracks)
    {
        if (t.sampleType == SampleType::HitNormal && t.sampleSet == bank)
        {
            // Check children for matching volume
            for (auto& child : t.children)
            {
                if (std::abs(child.gain - volume) < 0.01)
                    return &child;
            }
            
            // No matching volume, use primary child if we match bank
            if (!t.children.empty() && t.primaryChildIndex < (int)t.children.size())
                return &t.children[t.primaryChildIndex];
        }
    }
    
    // Create new track
    std::string bankStr = (bank == SampleSet::Normal) ? "normal" : (bank == SampleSet::Soft ? "soft" : "drum");
    int volPct = (int)(volume * 100);
    
    Track parent;
    parent.name = bankStr + "-hitnormal";
    parent.sampleSet = bank;
    parent.sampleType = SampleType::HitNormal;
    parent.primaryChildIndex = 0;
    
    Track child;
    child.name = bankStr + "-hitnormal (" + std::to_string(volPct) + "%)";
    child.sampleSet = bank;
    child.sampleType = SampleType::HitNormal;
    child.gain = volume;
    child.isChildTrack = true;
    
    parent.children.push_back(child);
    project->tracks.push_back(parent);
    
    if (OnTracksModified) OnTracksModified();
    
    return &project->tracks.back().children[0];
}

Track* TimelineController::GetEffectiveTargetTrack(Track* t)
{
    if (!t) return nullptr;
    
    if (!t->children.empty() && !t->isExpanded)
    {
        int idx = t->primaryChildIndex;
        if (idx >= 0 && idx < (int)t->children.size())
            return &t->children[idx];
    }
    
    return t;
}

std::vector<Track*> TimelineController::GetVisibleTracks()
{
    std::vector<Track*> visible;
    if (!project) return visible;
    
    std::function<void(Track&)> traverse = [&](Track& t) {
        visible.push_back(&t);
        if (t.isExpanded)
        {
            for (auto& c : t.children) traverse(c);
        }
    };
    
    for (auto& t : project->tracks) traverse(t);
    return visible;
}
