#include "test_utils.h"
#include <vector>

namespace tachyon::editor {
bool run_undo_manager_tests();
bool run_autosave_manager_tests();
} // namespace tachyon::editor

int main(int argc, char** argv) {
    using namespace tachyon::test;
    std::vector<TestCase> tests = {
        {"undo_manager", tachyon::editor::run_undo_manager_tests},
        {"autosave_manager", tachyon::editor::run_autosave_manager_tests},
    };

    return run_test_suite(argc, argv, tests);
}
