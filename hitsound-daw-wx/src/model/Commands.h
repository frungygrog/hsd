#pragma once
#include "Command.h"
#include "Track.h"
#include "Project.h"
#include <vector>
#include <algorithm>

// -----------------------------------------------------------------------------
// Add Event (Draw Tool)
// -----------------------------------------------------------------------------
class AddEventCommand : public Command
{
public:
    AddEventCommand(Track* track, Event evt, std::function<void()> refreshCallback)
        : track(track), evt(evt), refresh(refreshCallback) {}

    void Do() override {
        track->events.push_back(evt);
        refresh();
    }

    void Undo() override {
        // Assume it's at the end or find it? 
        // Safer to find by time/properties or keep index if we are sure no other insert happens.
        // Since we are single-threaded and undo stack is strict, finding the exact event instance is safest.
        auto it = std::find_if(track->events.rbegin(), track->events.rend(), 
            [&](const Event& e) { return e.time == evt.time && e.volume == evt.volume; });
        
        if (it != track->events.rend()) {
            track->events.erase(std::next(it).base());
        }
        refresh();
    }

    std::string GetDescription() const override { return "Add Note"; }

private:
    Track* track;
    Event evt;
    std::function<void()> refresh;
};

// -----------------------------------------------------------------------------
// Remove Events (Delete)
// -----------------------------------------------------------------------------
class RemoveEventsCommand : public Command
{
public:
    struct Item {
        Track* track;
        Event evt;
    };

    RemoveEventsCommand(const std::vector<Item>& items, std::function<void()> refreshCallback)
        : items(items), refresh(refreshCallback) {}

    void Do() override {
        for (const auto& item : items) {
             auto it = std::find_if(item.track->events.begin(), item.track->events.end(), 
                [&](const Event& e) { return e.time == item.evt.time && e.volume == item.evt.volume; });
             if (it != item.track->events.end()) {
                 item.track->events.erase(it);
             }
        }
        refresh();
    }

    void Undo() override {
        for (const auto& item : items) {
            item.track->events.push_back(item.evt);
        }
        refresh();
    }

    std::string GetDescription() const override { return "Delete Notes"; }

private:
    std::vector<Item> items;
    std::function<void()> refresh;
};

// -----------------------------------------------------------------------------
// Move Events (Drag)
// -----------------------------------------------------------------------------
class MoveEventsCommand : public Command
{
public:
    struct MoveInfo {
        Track* originalTrack;
        Event originalEvent;
        
        Track* newTrack;
        Event newEvent; // Has new time
    };

    MoveEventsCommand(const std::vector<MoveInfo>& moves, std::function<void()> refreshCallback)
        : moves(moves), refresh(refreshCallback) {}

    void Do() override {
        // Technically this command is created AFTER the move is visually "Done" by the user dragging.
        // So Do() is just applying the final state. 
        // BUT, the TimelineView::OnLeftUp clears selection and effectively "commits".
        // So we will perform the actual move here.
        
        // Remove Originals
        for (const auto& m : moves) {
             auto it = std::find_if(m.originalTrack->events.begin(), m.originalTrack->events.end(), 
                [&](const Event& e) { return e.time == m.originalEvent.time; });
             if (it != m.originalTrack->events.end()) m.originalTrack->events.erase(it);
        }
        
        // Add News
        for (const auto& m : moves) {
            m.newTrack->events.push_back(m.newEvent);
        }
        
        refresh();
    }
    
    void Undo() override {
        // Remove News
        for (const auto& m : moves) {
             auto it = std::find_if(m.newTrack->events.begin(), m.newTrack->events.end(), 
                [&](const Event& e) { return e.time == m.newEvent.time; });
             if (it != m.newTrack->events.end()) m.newTrack->events.erase(it);
        }
        
        // Restore Originals
        for (const auto& m : moves) {
            m.originalTrack->events.push_back(m.originalEvent);
        }
        refresh();
    }

    std::string GetDescription() const override { return "Move Notes"; }

private:
    std::vector<MoveInfo> moves;
    std::function<void()> refresh;
};

// -----------------------------------------------------------------------------
// Paste Events
// -----------------------------------------------------------------------------
class PasteEventsCommand : public Command
{
public:
    struct PasteItem {
        Track* track;
        Event evt;
    };
    
    PasteEventsCommand(const std::vector<PasteItem>& items, std::function<void(const std::vector<Track*>&)> selectionCallback, std::function<void()> refreshCallback)
        : items(items), select(selectionCallback), refresh(refreshCallback) {}
        
    void Do() override {
        std::vector<Track*> affected;
        for (const auto& item : items) {
            item.track->events.push_back(item.evt);
            affected.push_back(item.track);
        }
        // Ideally we select the pasted notes
        if (select) select(affected);
        refresh();
    }
    
    void Undo() override {
        for (const auto& item : items) {
            auto it = std::find_if(item.track->events.rbegin(), item.track->events.rend(), 
                [&](const Event& e) { return e.time == item.evt.time; });
            if (it != item.track->events.rend()) {
                 item.track->events.erase(std::next(it).base());
            }
        }
        refresh();
    }

    std::string GetDescription() const override { return "Paste Notes"; }

private:
    std::vector<PasteItem> items;
    std::function<void(const std::vector<Track*>&)> select; // To re-select on Redo/Do
    std::function<void()> refresh;
};
