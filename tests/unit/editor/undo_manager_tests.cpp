#include "tachyon/editor/undo/undo_manager.h"
#include "tachyon/editor/undo/command.h"

#include <iostream>
#include <string>

namespace tachyon::editor {

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

// Mock command for testing: increments/decrements an integer.
class IncrementCommand : public Command {
public:
    explicit IncrementCommand(int& target, int delta, const std::string& desc)
        : target_(target), delta_(delta), desc_(desc) {}

    void execute() override { target_ += delta_; }
    void undo() override { target_ -= delta_; }
    std::string description() const override { return desc_; }
    const char* command_type() const override { return "increment"; }

    bool try_merge(const Command& /*other*/) override {
        return false; // Default: no merge. Separate mock tests merge explicitly.
    }

    int delta() const { return delta_; }

private:
    int& target_;
    int delta_;
    std::string desc_;
};

} // namespace

bool run_undo_manager_tests() {
    g_failures = 0;

    // Basic execute / undo / redo
    {
        UndoManager mgr;
        int value = 0;

        mgr.execute(std::make_unique<IncrementCommand>(value, 5, "Add 5"));
        check_true(value == 5, "execute should apply command");
        check_true(mgr.can_undo(), "can_undo after execute");
        check_true(mgr.is_dirty(), "dirty after execute");
        check_true(mgr.peek_undo().description() == "Add 5", "peek_undo description");

        mgr.undo();
        check_true(value == 0, "undo should restore state");
        check_true(mgr.can_redo(), "can_redo after undo");
        check_true(!mgr.can_undo(), "cannot undo after undoing all");

        mgr.redo();
        check_true(value == 5, "redo should re-apply command");
        check_true(mgr.can_undo(), "can_undo after redo");
    }

    // Multiple commands
    {
        UndoManager mgr;
        int value = 0;

        mgr.execute(std::make_unique<IncrementCommand>(value, 1, "+1"));
        mgr.execute(std::make_unique<IncrementCommand>(value, 2, "+2"));
        mgr.execute(std::make_unique<IncrementCommand>(value, 3, "+3"));
        check_true(value == 6, "three commands should sum to 6");
        check_true(mgr.undo_size() == 3, "undo stack should have 3 items");

        mgr.undo();
        check_true(value == 3, "undo once -> 3");
        mgr.undo();
        check_true(value == 1, "undo twice -> 1");
        mgr.redo();
        check_true(value == 3, "redo once -> 3");
    }

    // New branch clears redo stack
    {
        UndoManager mgr;
        int value = 0;

        mgr.execute(std::make_unique<IncrementCommand>(value, 10, "+10"));
        mgr.undo();
        check_true(mgr.can_redo(), "can_redo before new branch");

        mgr.execute(std::make_unique<IncrementCommand>(value, 20, "+20"));
        check_true(!mgr.can_redo(), "redo cleared after new branch");
        check_true(value == 20, "new branch value should be 20");
    }

    // Dirty tracking
    {
        UndoManager mgr;
        int value = 0;

        check_true(!mgr.is_dirty(), "initially clean");
        mgr.execute(std::make_unique<IncrementCommand>(value, 1, "+1"));
        check_true(mgr.is_dirty(), "dirty after execute");

        mgr.mark_clean();
        check_true(!mgr.is_dirty(), "clean after mark_clean");

        mgr.undo();
        check_true(mgr.is_dirty(), "dirty after undo (state changed from last save)");
    }

    // Max history trimming
    {
        UndoManager mgr(3);
        int value = 0;

        mgr.execute(std::make_unique<IncrementCommand>(value, 1, "+1"));
        mgr.execute(std::make_unique<IncrementCommand>(value, 2, "+2"));
        mgr.execute(std::make_unique<IncrementCommand>(value, 3, "+3"));
        mgr.execute(std::make_unique<IncrementCommand>(value, 4, "+4"));
        check_true(mgr.undo_size() == 3, "undo stack trimmed to max_history=3");

        // Oldest (+1) should be evicted. Undoing 3 times should restore to 0.
        mgr.undo(); // -4
        mgr.undo(); // -3
        mgr.undo(); // -2
        check_true(value == 1, "oldest command evicted, value should be 1 not 0");
    }

    // Null command rejection (Manifesto Rule 6: fail-fast)
    {
        UndoManager mgr;
        bool threw = false;
        try {
            mgr.execute(nullptr);
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        check_true(threw, "null command should throw invalid_argument");
    }

    // Undo when empty throws (fail-fast)
    {
        UndoManager mgr;
        bool threw = false;
        try {
            mgr.undo();
        } catch (const std::runtime_error&) {
            threw = true;
        }
        check_true(threw, "undo on empty stack should throw");
    }

    // Redo when empty throws (fail-fast)
    {
        UndoManager mgr;
        bool threw = false;
        try {
            mgr.redo();
        } catch (const std::runtime_error&) {
            threw = true;
        }
        check_true(threw, "redo on empty stack should throw");
    }

    // Clear
    {
        UndoManager mgr;
        int value = 0;
        mgr.execute(std::make_unique<IncrementCommand>(value, 5, "+5"));
        mgr.clear();
        check_true(!mgr.can_undo(), "cannot undo after clear");
        check_true(!mgr.can_redo(), "cannot redo after clear");
        check_true(!mgr.is_dirty(), "clean after clear");
    }

    // Command merge
    {
        struct MergeableIncrementCommand : public Command {
            int& target;
            int delta{0};
            std::string desc;
            MergeableIncrementCommand(int& t, int d, std::string s)
                : target(t), delta(d), desc(std::move(s)) {}
            void execute() override { target += delta; }
            void undo() override { target -= delta; }
            std::string description() const override { return desc; }
            const char* command_type() const override { return "mergeable_increment"; }
            bool try_merge(const Command& other) override {
                const auto* o = dynamic_cast<const MergeableIncrementCommand*>(&other);
                if (!o || &o->target != &target) return false;
                delta += o->delta; // Absorb other's delta into THIS.
                return true;
            }
        };

        UndoManager mgr;
        int value = 0;

        mgr.execute(std::make_unique<MergeableIncrementCommand>(value, 1, "+1"));
        mgr.execute(std::make_unique<MergeableIncrementCommand>(value, 2, "+2"));
        check_true(mgr.undo_size() == 1, "merged commands should occupy 1 slot");
        check_true(value == 3, "merged delta should be 1+2=3");

        mgr.undo();
        check_true(value == 0, "undo merged command restores state");
    }

    return g_failures == 0;
}

} // namespace tachyon::editor
