#pragma once

#include "../model/Project.h"
#include "../model/Command.h"
#include "../model/Track.h"
#include <set>
#include <vector>
#include <functional>
#include <optional>

/**
 * TimelineController handles all business logic for timeline operations.
 * This separates concerns from TimelineView (which handles rendering and input).
 * 
 * Responsibilities:
 * - Selection management
 * - Clipboard operations (copy/cut/paste/delete)
 * - Event placement with auto-hitnormal logic
 * - Undo/Redo management
 * - Dirty state tracking
 */
class TimelineController
{
public:
    TimelineController();
    
    void SetProject(Project* p);
    Project* GetProject() const { return project; }
    
    // -------------------------------------------------------------------------
    // Selection
    // -------------------------------------------------------------------------
    
    void SelectEvent(uint64_t trackId, uint64_t eventId, bool addToSelection = false);
    void DeselectEvent(uint64_t trackId, uint64_t eventId);
    void ClearSelection();
    void SelectAll();
    void SetSelection(const std::set<std::pair<uint64_t, uint64_t>>& newSelection);
    
    bool IsSelected(uint64_t trackId, uint64_t eventId) const;
    const std::set<std::pair<uint64_t, uint64_t>>& GetSelection() const { return selection; }
    bool HasSelection() const { return !selection.empty(); }
    
    // Base selection for marquee operations
    void SaveBaseSelection();
    void RestoreBaseSelection();
    void MergeWithBaseSelection(const std::set<std::pair<uint64_t, uint64_t>>& newItems, bool toggle);
    
    // Last focused track for paste anchor
    void SetLastFocusedTrack(uint64_t trackId) { lastFocusedTrackId = trackId; }
    uint64_t GetLastFocusedTrackId() const { return lastFocusedTrackId; }
    
    // -------------------------------------------------------------------------
    // Clipboard
    // -------------------------------------------------------------------------
    
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
    
    // -------------------------------------------------------------------------
    // Event Placement
    // -------------------------------------------------------------------------
    
    // Place an event with auto-hitnormal logic
    void PlaceEvent(Track* target, double time, std::optional<SampleSet> defaultHitnormalBank);
    
    // -------------------------------------------------------------------------
    // Undo/Redo
    // -------------------------------------------------------------------------
    
    void Undo();
    void Redo();
    bool CanUndo() const;
    bool CanRedo() const;
    std::string GetUndoDescription() const;
    std::string GetRedoDescription() const;
    
    UndoManager& GetUndoManager() { return undoManager; }
    
    // -------------------------------------------------------------------------
    // Dirty State
    // -------------------------------------------------------------------------
    
    void MarkClean();
    bool IsDirty() const;
    
    // -------------------------------------------------------------------------
    // Validation
    // -------------------------------------------------------------------------
    
    void ValidateHitsounds();
    
    // -------------------------------------------------------------------------
    // Track Helpers
    // -------------------------------------------------------------------------
    
    Track* FindTrackById(uint64_t id);
    Track* FindOrCreateHitnormalTrack(SampleSet bank, double volume);
    Track* GetEffectiveTargetTrack(Track* t);
    std::vector<Track*> GetVisibleTracks();
    
    // -------------------------------------------------------------------------
    // Callbacks
    // -------------------------------------------------------------------------
    
    // Called when data changes and view should refresh
    std::function<void()> OnDataChanged;
    
    // Called when tracks are added/removed (for audio sync)
    std::function<void()> OnTracksModified;
    
private:
    Project* project = nullptr;
    UndoManager undoManager;
    
    // Selection using {trackId, eventId} pairs
    std::set<std::pair<uint64_t, uint64_t>> selection;
    std::set<std::pair<uint64_t, uint64_t>> baseSelection;
    
    uint64_t lastFocusedTrackId = 0;
    
    // Clipboard
    std::vector<ClipboardItem> clipboard;
    
    // Helper to find row index for clipboard operations
    int FindRowIndex(Track* t, const std::vector<Track*>& visible);
};
