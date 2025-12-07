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
    void EnsureCleanState() {
        // Optional: Call this when project saves to know where "clean" state was
    }

    void PushCommand(std::unique_ptr<Command> cmd)
    {
        // If we are not at the end, clear redo stack
        if (currentIndex < (int)history.size())
        {
            history.erase(history.begin() + currentIndex, history.end());
        }

        // Try merge
        if (!history.empty() && history.back()->MergeWith(cmd.get()))
        {
            // Merged, Do() is assumed properly handled or valid
            // Usually merged commands are like "Move + 5" + "Move + 2" = "Move + 7"
            // But often we just update parameters. 
            // For now, let's execute the new command part if needed, or assume caller did it.
            // Actually, simpler: Wrapper calls Do(), then pushes. 
            // If merged, we expect the history.back() to have updated its state.
            return;
        }

        cmd->Do();
        history.push_back(std::move(cmd));
        currentIndex++;
        
        // Limit history size if needed
        if (history.size() > maxHistory)
        {
            history.pop_front();
            currentIndex--;
        }
    }

    void Undo()
    {
        if (CanUndo())
        {
            currentIndex--;
            history[currentIndex]->Undo();
        }
    }

    void Redo()
    {
        if (CanRedo())
        {
            history[currentIndex]->Do();
            currentIndex++;
        }
    }

    bool CanUndo() const { return currentIndex > 0; }
    bool CanRedo() const { return currentIndex < (int)history.size(); }
    
    std::string GetUndoDescription() const {
        if (CanUndo()) return history[currentIndex - 1]->GetDescription();
        return "";
    }
    
    std::string GetRedoDescription() const {
        if (CanRedo()) return history[currentIndex]->GetDescription();
        return "";
    }
    
    void Clear() {
        history.clear();
        currentIndex = 0;
    }

private:
    std::deque<std::unique_ptr<Command>> history;
    int currentIndex = 0; // Points to the slot for the NEXT new command (or the one to Redo)
    const size_t maxHistory = 100;
};
