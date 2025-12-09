#include "Command.h"

void UndoManager::MarkClean()
{
    savedStateIndex = currentIndex;
}

bool UndoManager::IsDirty() const
{
    return currentIndex != savedStateIndex;
}

void UndoManager::PushCommand(std::unique_ptr<Command> cmd)
{
    // Truncate redo history when pushing new command
    if (currentIndex < (int)history.size())
    {
        history.erase(history.begin() + currentIndex, history.end());
    }

    // Try to merge with previous command (for continuous drag operations)
    if (!history.empty() && history.back()->MergeWith(cmd.get()))
    {
        return;
    }

    cmd->Do();
    history.push_back(std::move(cmd));
    currentIndex++;

    // Limit history size
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
