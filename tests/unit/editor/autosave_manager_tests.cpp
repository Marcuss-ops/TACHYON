#include "tachyon/editor/autosave/autosave_manager.h"
#include "tachyon/editor/undo/undo_manager.h"
#include "tachyon/editor/undo/command.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"

#include <iostream>
#include <filesystem>
#include <string>
#include <chrono>
#include <thread>

namespace {
struct NoOpCommand : public tachyon::editor::Command {
    void execute() override {}
    void undo() override {}
    std::string description() const override { return "noop"; }
    const char* command_type() const override { return "noop"; }
};
} // namespace

namespace tachyon::editor {

namespace fs = std::filesystem;

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

// Simple round-trip serialize/deserialize for testing.
std::string test_serialize(const SceneSpec& scene) {
    return scene.project.id + "\n" + scene.project.name;
}

SceneSpec test_deserialize(const std::string& data) {
    SceneSpec scene;
    const auto nl = data.find('\n');
    scene.project.id = data.substr(0, nl);
    scene.project.name = (nl != std::string::npos) ? data.substr(nl + 1) : "";
    return scene;
}

} // namespace

bool run_autosave_manager_tests() {
    g_failures = 0;

    const std::string test_dir = "/tmp/tachyon_autosave_test";
    if (fs::exists(test_dir)) {
        fs::remove_all(test_dir);
    }

    AutosaveManager::Config cfg;
    cfg.autosave_directory = test_dir;
    cfg.max_versions = 3;
    cfg.enabled = true;
    AutosaveManager mgr(cfg);

    // force_autosave writes a file
    {
        SceneSpec scene;
        scene.project.id = "proj_001";
        scene.project.name = "TestProject";

        const std::string path = mgr.force_autosave(scene, test_serialize);
        check_true(!path.empty(), "force_autosave should return a path");
        check_true(fs::exists(path), "autosave file should exist on disk");

        const std::string latest = mgr.find_latest_autosave();
        check_true(!latest.empty(), "find_latest_autosave should find the file");
        check_true(latest == path, "latest should match the written path");
    }

    // load_latest round-trips data
    {
        const auto loaded = mgr.load_latest(test_deserialize);
        check_true(loaded.has_value(), "load_latest should return a scene");
        check_true(loaded->project.id == "proj_001", "round-trip project id");
        check_true(loaded->project.name == "TestProject", "round-trip project name");
    }

    // max_versions trimming
    {
        SceneSpec scene;
        scene.project.id = "v1"; mgr.force_autosave(scene, test_serialize);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        scene.project.id = "v2"; mgr.force_autosave(scene, test_serialize);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        scene.project.id = "v3"; mgr.force_autosave(scene, test_serialize);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        scene.project.id = "v4"; mgr.force_autosave(scene, test_serialize);

        int file_count = 0;
        for (const auto& entry : fs::directory_iterator(test_dir)) {
            if (entry.path().extension() == ".autosave") ++file_count;
        }
        check_true(file_count == 3, "max_versions=3 should keep only 3 files");

        const auto latest = mgr.load_latest(test_deserialize);
        check_true(latest->project.id == "v4", "latest should be the most recent (v4)");
    }

    // purge_all
    {
        mgr.purge_all();
        int file_count = 0;
        for (const auto& entry : fs::directory_iterator(test_dir)) {
            if (entry.path().extension() == ".autosave") ++file_count;
        }
        check_true(file_count == 0, "purge_all should remove all autosave files");
    }

    // maybe_autosave respects dirty flag
    {
        UndoManager undo;
        SceneSpec scene;
        scene.project.id = "dirty_test";

        // Clean undo manager: should not autosave
        bool saved = mgr.maybe_autosave(undo, scene, test_serialize);
        check_true(!saved, "maybe_autosave should skip when clean");

        // Dirty undo manager: should autosave on first dirty edge
        undo.execute(std::make_unique<NoOpCommand>());
        saved = mgr.maybe_autosave(undo, scene, test_serialize);
        check_true(saved, "maybe_autosave should trigger on dirty edge");

        // Still dirty but no edge: should not autosave again
        saved = mgr.maybe_autosave(undo, scene, test_serialize);
        check_true(!saved, "maybe_autosave should throttle repeated dirty checks");
    }

    // Disabled manager rejects all operations
    {
        AutosaveManager::Config disabled_cfg;
        disabled_cfg.autosave_directory = test_dir + "_disabled";
        disabled_cfg.enabled = false;
        AutosaveManager disabled_mgr(disabled_cfg);

        SceneSpec scene;
        scene.project.id = "disabled";
        const std::string path = disabled_mgr.force_autosave(scene, test_serialize);
        check_true(path.empty(), "force_autosave should return empty when disabled");
    }

    // Cleanup
    if (fs::exists(test_dir)) {
        fs::remove_all(test_dir);
    }

    return g_failures == 0;
}

} // namespace tachyon::editor
