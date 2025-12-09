#include "Commands.h"
#include <algorithm>

// AddEventCommand

AddEventCommand::AddEventCommand(Track* track, Event evt, std::function<void()> refreshCallback)
    : track(track), evt(evt), refresh(refreshCallback) {}

void AddEventCommand::Do()
{
    track->events.push_back(evt);
    refresh();
}

void AddEventCommand::Undo()
{
    auto it = std::find_if(track->events.rbegin(), track->events.rend(), 
        [&](const Event& e) { return e.id == evt.id; });

    if (it != track->events.rend()) {
        track->events.erase(std::next(it).base());
    }
    refresh();
}

std::string AddEventCommand::GetDescription() const
{
    return "Add Note";
}

// AddMultipleEventsCommand

AddMultipleEventsCommand::AddMultipleEventsCommand(const std::vector<Item>& items, std::function<void()> refreshCallback)
    : items(items), refresh(refreshCallback) {}

void AddMultipleEventsCommand::Do()
{
    for (const auto& item : items) {
        item.track->events.push_back(item.evt);
    }
    refresh();
}

void AddMultipleEventsCommand::Undo()
{
    for (const auto& item : items) {
        auto it = std::find_if(item.track->events.rbegin(), item.track->events.rend(), 
            [&](const Event& e) { return e.id == item.evt.id; });

        if (it != item.track->events.rend()) {
            item.track->events.erase(std::next(it).base());
        }
    }
    refresh();
}

std::string AddMultipleEventsCommand::GetDescription() const
{
    return "Add Notes";
}

// RemoveEventsCommand

RemoveEventsCommand::RemoveEventsCommand(const std::vector<Item>& items, std::function<void()> refreshCallback)
    : items(items), refresh(refreshCallback) {}

void RemoveEventsCommand::Do()
{
    for (const auto& item : items) {
        auto it = std::find_if(item.track->events.begin(), item.track->events.end(), 
            [&](const Event& e) { return e.id == item.evt.id; });
        if (it != item.track->events.end()) {
            item.track->events.erase(it);
        }
    }
    refresh();
}

void RemoveEventsCommand::Undo()
{
    for (const auto& item : items) {
        item.track->events.push_back(item.evt);
    }
    refresh();
}

std::string RemoveEventsCommand::GetDescription() const
{
    return "Delete Notes";
}

// MoveEventsCommand

MoveEventsCommand::MoveEventsCommand(const std::vector<MoveInfo>& moves, std::function<void()> refreshCallback)
    : moves(moves), refresh(refreshCallback) {}

void MoveEventsCommand::Do()
{
    // First remove all original events
    for (const auto& m : moves) {
        auto it = std::find_if(m.originalTrack->events.begin(), m.originalTrack->events.end(), 
            [&](const Event& e) { return e.id == m.originalEvent.id; });
        if (it != m.originalTrack->events.end()) 
            m.originalTrack->events.erase(it);
    }

    // Then add all new events
    for (const auto& m : moves) {
        m.newTrack->events.push_back(m.newEvent);
    }

    refresh();
}

void MoveEventsCommand::Undo()
{
    // Remove new events
    for (const auto& m : moves) {
        auto it = std::find_if(m.newTrack->events.begin(), m.newTrack->events.end(), 
            [&](const Event& e) { return e.id == m.newEvent.id; });
        if (it != m.newTrack->events.end()) 
            m.newTrack->events.erase(it);
    }

    // Restore original events
    for (const auto& m : moves) {
        m.originalTrack->events.push_back(m.originalEvent);
    }
    refresh();
}

std::string MoveEventsCommand::GetDescription() const
{
    return "Move Notes";
}

// PasteEventsCommand

PasteEventsCommand::PasteEventsCommand(const std::vector<PasteItem>& items, 
    std::function<void(const std::vector<Track*>&)> selectionCallback, 
    std::function<void()> refreshCallback)
    : items(items), select(selectionCallback), refresh(refreshCallback) {}

void PasteEventsCommand::Do()
{
    std::vector<Track*> affected;
    for (const auto& item : items) {
        item.track->events.push_back(item.evt);
        affected.push_back(item.track);
    }

    if (select) 
        select(affected);
    refresh();
}

void PasteEventsCommand::Undo()
{
    for (const auto& item : items) {
        auto it = std::find_if(item.track->events.rbegin(), item.track->events.rend(), 
            [&](const Event& e) { return e.id == item.evt.id; });
        if (it != item.track->events.rend()) {
            item.track->events.erase(std::next(it).base());
        }
    }
    refresh();
}

std::string PasteEventsCommand::GetDescription() const
{
    return "Paste Notes";
}
