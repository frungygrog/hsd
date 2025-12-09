#pragma once
#include <string>
#include <vector>
#include <memory>
#include <deque>

class Command
{
public:
    virtual ~Command() = default;

    virtual void Do() = 0;
    virtual void Undo() = 0;
    virtual std::string GetDescription() const = 0;
    
    // For merging similar commands (e.g. dragging updates)
    virtual bool MergeWith(const Command* other) { return false; }
};

class UndoManager
{
public:
    void MarkClean();      // Call after save to mark current state as clean
    bool IsDirty() const;  // Returns true if there are unsaved changes
    
    void PushCommand(std::unique_ptr<Command> cmd);
    void Undo();
    void Redo();
    bool CanUndo() const;
    bool CanRedo() const;
    std::string GetUndoDescription() const;
    std::string GetRedoDescription() const;
    void Clear();

private:
    std::deque<std::unique_ptr<Command>> history;
    int currentIndex = 0;      // Points to the slot for the NEXT new command (or the one to Redo)
    int savedStateIndex = 0;   // Index at which the project was last saved
    const size_t maxHistory = 100;
};
