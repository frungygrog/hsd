#pragma once

#include "../model/Project.h"
#include "../model/Command.h"
#include "../model/Track.h"
#include <set>
#include <vector>
#include <functional>
#include <optional>

class TimelineController
{
public:
    TimelineController();
    
    void SetProject(Project* p);
    Project* GetProject() const { return project; }

    
    void SelectEvent(uint64_t trackId, uint64_t eventId, bool addToSelection = false);
    void DeselectEvent(uint64_t trackId, uint64_t eventId);
    void ClearSelection();
    void SelectAll();
    void SetSelection(const std::set<std::pair<uint64_t, uint64_t>>& newSelection);
    
    bool IsSelected(uint64_t trackId, uint64_t eventId) const;
    const std::set<std::pair<uint64_t, uint64_t>>& GetSelection() const { return selection; }
    bool HasSelection() const { return !selection.empty(); }
    
    
    void SaveBaseSelection();
    void RestoreBaseSelection();
    void MergeWithBaseSelection(const std::set<std::pair<uint64_t, uint64_t>>& newItems, bool toggle);
    
    
    void SetLastFocusedTrack(uint64_t trackId) { lastFocusedTrackId = trackId; }
    uint64_t GetLastFocusedTrackId() const { return lastFocusedTrackId; }
    
    
    
    
    
    struct ClipboardItem {
        Event evt;
        int relativeRow;
        double relativeTime;
    };
    
    void CopySelection(const std::vector<Track*>& visibleTracks);
    void CutSelection(const std::vector<Track*>& visibleTracks);
    void PasteAtPlayhead(double playheadTime, const std::vector<Track*>& visibleTracks);
    void DeleteSelection();
    
    bool HasClipboard() const { return !clipboard.empty(); }
    
    
    
    
    
    
    void PlaceEvent(Track* target, double time, std::optional<SampleSet> defaultHitnormalBank);
    
    
    
    
    
    void Undo();
    void Redo();
    bool CanUndo() const;
    bool CanRedo() const;
    std::string GetUndoDescription() const;
    std::string GetRedoDescription() const;
    
    UndoManager& GetUndoManager() { return undoManager; }
    
    
    
    
    
    void MarkClean();
    bool IsDirty() const;
    
    
    
    
    
    void ValidateHitsounds();
    
    
    
    
    
    Track* FindTrackById(uint64_t id);
    Track* FindOrCreateHitnormalTrack(SampleSet bank, double volume);
    Track* GetEffectiveTargetTrack(Track* t);
    std::vector<Track*> GetVisibleTracks();
    
    
    
    
    
    
    std::function<void()> OnDataChanged;
    
    
    std::function<void()> OnTracksModified;
    
private:
    Project* project = nullptr;
    UndoManager undoManager;
    
    
    std::set<std::pair<uint64_t, uint64_t>> selection;
    std::set<std::pair<uint64_t, uint64_t>> baseSelection;
    
    uint64_t lastFocusedTrackId = 0;
    
    
    std::vector<ClipboardItem> clipboard;
    
    
    int FindRowIndex(Track* t, const std::vector<Track*>& visible);
};
