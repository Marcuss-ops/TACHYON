#pragma once

#include "tachyon/editor/undo/command.h"

#include <vector>
#include <cstddef>
#include <stdexcept>

namespace tachyon::editor {

/**
 * @brief Deterministic undo/redo manager for the editor.
 *
 * Manifesto Rules:
 * - Rule 3: pre-allocate, avoid churn. Uses vector reserve.
 * - Rule 5: deterministic — same sequence of execute/undo always produces
 *           the same state.
 * - Rule 6: fail-fast — undo/redo when impossible throws.
 * - Rule 7: explicit names (can_undo, peek_undo, is_dirty).
 */
class UndoManager {
public:
    static constexpr std::size_t kDefaultMaxHistory = 256;

    explicit UndoManager(std::size_t max_history = kDefaultMaxHistory);

    /** Execute a command and push it onto the undo stack.
     *  Clears the redo stack (branching history is not supported). */
    void execute(CommandPtr cmd);

    /** @return true if there is at least one command to undo. */
    [[nodiscard]] bool can_undo() const noexcept { return !undo_stack_.empty(); }

    /** @return true if there is at least one command to redo. */
    [[nodiscard]] bool can_redo() const noexcept { return !redo_stack_.empty(); }

    /** Undo the most recent command. Throws if can_undo() is false. */
    void undo();

    /** Redo the most recently undone command. Throws if can_redo() is false. */
    void redo();

    /** Clear both undo and redo stacks. */
    void clear();

    /** @return Number of commands available to undo. */
    [[nodiscard]] std::size_t undo_size() const noexcept { return undo_stack_.size(); }

    /** @return Number of commands available to redo. */
    [[nodiscard]] std::size_t redo_size() const noexcept { return redo_stack_.size(); }

    /** Peek at the most recent undo command without modifying state.
     *  Throws if can_undo() is false. */
    [[nodiscard]] const Command& peek_undo() const;

    /** Peek at the next redo command without modifying state.
     *  Throws if can_redo() is false. */
    [[nodiscard]] const Command& peek_redo() const;

    /** Change the maximum history depth. Older commands are trimmed immediately. */
    void set_max_history(std::size_t max);

    /** @return true if the document has unsaved changes. */
    [[nodiscard]] bool is_dirty() const noexcept { return dirty_; }

    /** Reset the dirty flag (call after a successful save). */
    void mark_clean() noexcept { dirty_ = false; }

    /** Force the dirty flag (call after external changes). */
    void mark_dirty() noexcept { dirty_ = true; }

private:
    std::vector<CommandPtr> undo_stack_;
    std::vector<CommandPtr> redo_stack_;
    std::size_t max_history_;
    bool dirty_{false};

    void trim_undo_stack();
};

} // namespace tachyon::editor
