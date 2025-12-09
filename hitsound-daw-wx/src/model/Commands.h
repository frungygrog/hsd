#pragma once
#include "Command.h"
#include "Track.h"
#include "Project.h"
#include <vector>
#include <functional>

class AddEventCommand : public Command
{
public:
    AddEventCommand(Track* track, Event evt, std::function<void()> refreshCallback);

    void Do() override;
    void Undo() override;
    std::string GetDescription() const override;

private:
    Track* track;
    Event evt;
    std::function<void()> refresh;
};

// Used for placing auto-hitnormal events together with additions
class AddMultipleEventsCommand : public Command
{
public:
    struct Item {
        Track* track;
        Event evt;
    };

    AddMultipleEventsCommand(const std::vector<Item>& items, std::function<void()> refreshCallback);

    void Do() override;
    void Undo() override;
    std::string GetDescription() const override;

private:
    std::vector<Item> items;
    std::function<void()> refresh;
};

class RemoveEventsCommand : public Command
{
public:
    struct Item {
        Track* track;
        Event evt;
    };

    RemoveEventsCommand(const std::vector<Item>& items, std::function<void()> refreshCallback);

    void Do() override;
    void Undo() override;
    std::string GetDescription() const override;

private:
    std::vector<Item> items;
    std::function<void()> refresh;
};

class MoveEventsCommand : public Command
{
public:
    struct MoveInfo {
        Track* originalTrack;
        Event originalEvent;
        Track* newTrack;
        Event newEvent;
    };

    MoveEventsCommand(const std::vector<MoveInfo>& moves, std::function<void()> refreshCallback);

    void Do() override;
    void Undo() override;
    std::string GetDescription() const override;

private:
    std::vector<MoveInfo> moves;
    std::function<void()> refresh;
};

class PasteEventsCommand : public Command
{
public:
    struct PasteItem {
        Track* track;
        Event evt;
    };

    PasteEventsCommand(const std::vector<PasteItem>& items, 
        std::function<void(const std::vector<Track*>&)> selectionCallback, 
        std::function<void()> refreshCallback);

    void Do() override;
    void Undo() override;
    std::string GetDescription() const override;

private:
    std::vector<PasteItem> items;
    std::function<void(const std::vector<Track*>&)> select;
    std::function<void()> refresh;
};
