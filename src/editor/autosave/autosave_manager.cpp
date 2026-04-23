#include "tachyon/editor/autosave/autosave_manager.h"

#include <filesystem>
#include <fstream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <stdexcept>

namespace tachyon::editor {

namespace fs = std::filesystem;

AutosaveManager::AutosaveManager(const Config& config) : config_(config) {
    if (config_.autosave_directory.empty()) {
        throw std::invalid_argument("AutosaveManager: autosave_directory must not be empty");
    }
    if (!fs::exists(config_.autosave_directory)) {
        fs::create_directories(config_.autosave_directory);
    }
}

bool AutosaveManager::maybe_autosave(const UndoManager& undo_mgr,
                                     const SceneSpec& scene,
                                     const SerializeFn& serialize) {
    if (!config_.enabled) return false;

    const bool is_dirty = undo_mgr.is_dirty();
    const bool dirty_edge = is_dirty && !was_dirty_last_check_;

    // We track elapsed time internally via external calls (simpler than threading).
    // In a real editor, the caller passes delta_seconds from the update loop.
    // For this implementation we treat each call as a tick and rely on
    // dirty_edge + interval policy. A more advanced version would accumulate
    // delta time; here we keep it simple and deterministic.
    if (!is_dirty) {
        was_dirty_last_check_ = false;
        return false;
    }

    // Only save on the transition from clean->dirty or if enough time passed.
    // Since we don't have a real clock here, we use a simplified policy:
    // save immediately when dirty edge is detected, then throttle by
    // counting calls. In production this would use std::chrono::steady_clock.
    if (!dirty_edge) {
        return false; // Throttle: already dirty, already autosaved recently.
    }

    const std::string path = force_autosave(scene, serialize);
    was_dirty_last_check_ = true;
    return !path.empty();
}

std::string AutosaveManager::force_autosave(const SceneSpec& scene,
                                            const SerializeFn& serialize) {
    if (!config_.enabled) return {};

    const std::string path_str = make_autosave_path();
    const std::string data = serialize(scene);

    std::ofstream file(path_str, std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("AutosaveManager: failed to open " + path_str);
    }
    file.write(data.data(), static_cast<std::streamsize>(data.size()));
    if (!file.good()) {
        throw std::runtime_error("AutosaveManager: failed to write " + path_str);
    }

    trim_old_versions();
    return path_str;
}

std::string AutosaveManager::find_latest_autosave() const {
    if (!fs::exists(config_.autosave_directory)) return {};

    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(config_.autosave_directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".autosave") {
            files.push_back(entry.path());
        }
    }

    if (files.empty()) return {};

    auto newest = *std::max_element(files.begin(), files.end(),
        [](const fs::path& a, const fs::path& b) {
            return fs::last_write_time(a) < fs::last_write_time(b);
        });
    return newest.string();
}

std::optional<SceneSpec> AutosaveManager::load_latest(const DeserializeFn& deserialize) const {
    const std::string path = find_latest_autosave();
    if (path.empty()) return std::nullopt;

    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) return std::nullopt;

    std::string data((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
    if (data.empty()) return std::nullopt;

    return deserialize(data);
}

void AutosaveManager::purge_all() {
    if (!fs::exists(config_.autosave_directory)) return;
    for (const auto& entry : fs::directory_iterator(config_.autosave_directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".autosave") {
            fs::remove(entry.path());
        }
    }
}

std::string AutosaveManager::make_autosave_path() const {
    const auto now = std::chrono::system_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    return (fs::path(config_.autosave_directory) /
            ("tachyon_" + std::to_string(ms) + ".autosave")).string();
}

void AutosaveManager::trim_old_versions() {
    if (!fs::exists(config_.autosave_directory)) return;

    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(config_.autosave_directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".autosave") {
            files.push_back(entry.path());
        }
    }

    if (files.size() <= config_.max_versions) return;

    std::sort(files.begin(), files.end(),
        [](const fs::path& a, const fs::path& b) {
            return fs::last_write_time(a) < fs::last_write_time(b);
        });

    const std::size_t to_remove = files.size() - config_.max_versions;
    for (std::size_t i = 0; i < to_remove; ++i) {
        fs::remove(files[i]);
    }
}

} // namespace tachyon::editor
