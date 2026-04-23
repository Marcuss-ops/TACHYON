#include "tachyon/editor/undo/undo_manager.h"

namespace tachyon::editor {

UndoManager::UndoManager(std::size_t max_history)
    : max_history_(max_history) {
    undo_stack_.reserve(max_history_);
    redo_stack_.reserve(max_history_);
}

void UndoManager::execute(CommandPtr cmd) {
    if (!cmd) {
        throw std::invalid_argument("UndoManager::execute received null command");
    }

    cmd->execute();

    // Manifesto Rule 5: deterministic merge — same sequence = same state.
    if (!undo_stack_.empty() && undo_stack_.back()->try_merge(*cmd)) {
        return; // Merged: new command absorbed into previous.
    }

    undo_stack_.push_back(std::move(cmd));
    trim_undo_stack();

    // New branch: clear redo history.
    redo_stack_.clear();
    dirty_ = true;
}

void UndoManager::undo() {
    if (undo_stack_.empty()) {
        throw std::runtime_error("UndoManager::undo: no commands to undo");
    }

    CommandPtr cmd = std::move(undo_stack_.back());
    undo_stack_.pop_back();
    cmd->undo();
    redo_stack_.push_back(std::move(cmd));
    dirty_ = true;
}

void UndoManager::redo() {
    if (redo_stack_.empty()) {
        throw std::runtime_error("UndoManager::redo: no commands to redo");
    }

    CommandPtr cmd = std::move(redo_stack_.back());
    redo_stack_.pop_back();
    cmd->execute();
    undo_stack_.push_back(std::move(cmd));
    dirty_ = true;
}

void UndoManager::clear() {
    undo_stack_.clear();
    redo_stack_.clear();
    dirty_ = false;
}

const Command& UndoManager::peek_undo() const {
    if (undo_stack_.empty()) {
        throw std::runtime_error("UndoManager::peek_undo: undo stack is empty");
    }
    return *undo_stack_.back();
}

const Command& UndoManager::peek_redo() const {
    if (redo_stack_.empty()) {
        throw std::runtime_error("UndoManager::peek_redo: redo stack is empty");
    }
    return *redo_stack_.back();
}

void UndoManager::set_max_history(std::size_t max) {
    max_history_ = max;
    trim_undo_stack();
}

void UndoManager::trim_undo_stack() {
    // Trim oldest commands when exceeding max_history.
    // Vector erase at begin is O(n) but max_history is small (default 256).
    while (undo_stack_.size() > max_history_) {
        undo_stack_.erase(undo_stack_.begin());
    }
}

} // namespace tachyon::editor
