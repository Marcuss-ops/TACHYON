#pragma once

#include <string>
#include <memory>

namespace tachyon::editor {

/**
 * @brief Reversible editor command.
 *
 * Manifesto Rule 7: every command is explicit, named, and reversible.
 * Manifesto Rule 5: deterministic — execute + undo + execute must restore
 * the exact same state.
 */
class Command {
public:
    virtual ~Command() = default;

    /** Execute the command. Called once before pushing onto the undo stack. */
    virtual void execute() = 0;

    /** Undo the command. Must restore the exact state before execute(). */
    virtual void undo() = 0;

    /** Human-readable description for UI (e.g., "Move Layer 'Title' to x=120"). */
    virtual std::string description() const = 0;

    /** Optional: merge with a subsequent command of the same type.
     *  Returns true if merged (this command absorbs @p next). */
    virtual bool try_merge(const Command& /*next*/) { return false; }

    /** Unique type tag for fast filtering / grouping in UI. */
    virtual const char* command_type() const = 0;
};

using CommandPtr = std::unique_ptr<Command>;

} // namespace tachyon::editor
