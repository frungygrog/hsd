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

    // Returns true if this command absorbed the other command (for merging drag operations)
    virtual bool MergeWith(const Command* other) { return false; }
};

class UndoManager
{
public:
    void MarkClean();
    bool IsDirty() const;

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
    int currentIndex = 0;
    int savedStateIndex = 0;
    const size_t maxHistory = 100;
};
