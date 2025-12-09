#include "Command.h"

// -----------------------------------------------------------------------------
// UndoManager Implementation
// -----------------------------------------------------------------------------

void UndoManager::MarkClean()
{
    // Remember the current position as the "clean" saved state
    savedStateIndex = currentIndex;
}

bool UndoManager::IsDirty() const
{
    // Dirty if current position differs from where we last saved
    return currentIndex != savedStateIndex;
}
void UndoManager::PushCommand(std::unique_ptr<Command> cmd)
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

void UndoManager::Undo()
{
    if (CanUndo())
    {
        currentIndex--;
        history[currentIndex]->Undo();
    }
}

void UndoManager::Redo()
{
    if (CanRedo())
    {
        history[currentIndex]->Do();
        currentIndex++;
    }
}

bool UndoManager::CanUndo() const
{
    return currentIndex > 0;
}

bool UndoManager::CanRedo() const
{
    return currentIndex < (int)history.size();
}

std::string UndoManager::GetUndoDescription() const
{
    if (CanUndo())
        return history[currentIndex - 1]->GetDescription();
    return "";
}

std::string UndoManager::GetRedoDescription() const
{
    if (CanRedo())
        return history[currentIndex]->GetDescription();
    return "";
}

void UndoManager::Clear()
{
    history.clear();
    currentIndex = 0;
    savedStateIndex = 0;
}
