#pragma once

#include "tachyon/editor/undo/undo_manager.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"

#include <string>
#include <functional>
#include <cstddef>
#include <optional>

namespace tachyon::editor {

/**
 * @brief Autosave manager for crash recovery and background saves.
 *
 * Manifesto Rules:
 * - Rule 3: no blocking I/O on render thread. Serialization is caller's
 *           responsibility; this manager only orchestrates.
 * - Rule 6: fail-fast — invalid paths or serialization errors are reported,
 *           never swallowed.
 * - Rule 7: explicit names and policies (max_versions, interval_seconds).
 */
class AutosaveManager {
public:
    struct Config {
        std::string autosave_directory;   ///< Directory for .autosave files.
        std::size_t max_versions{5};      ///< Keep only the N most recent saves.
        double interval_seconds{30.0};    ///< Minimum seconds between autosaves.
        bool enabled{true};
    };

    using SerializeFn = std::function<std::string(const SceneSpec&)>;
    using DeserializeFn = std::function<SceneSpec(const std::string&)>;

    explicit AutosaveManager(const Config& config);

    /** Call periodically (e.g., every second) from the editor update loop.
     *  If the undo manager is dirty and enough time has passed, triggers
     *  an autosave by calling @p serialize and writing to disk.
     *
     *  @return true if an autosave was written this call. */
    bool maybe_autosave(const UndoManager& undo_mgr,
                        const SceneSpec& scene,
                        const SerializeFn& serialize);

    /** Force an immediate autosave, bypassing the interval timer.
     *  @return path of the written file, or empty on failure. */
    std::string force_autosave(const SceneSpec& scene,
                               const SerializeFn& serialize);

    /** Look for the most recent autosave file.
     *  @return path to the newest .autosave file, or empty if none. */
    [[nodiscard]] std::string find_latest_autosave() const;

    /** Load the most recent autosave.
     *  @return The deserialized SceneSpec, or empty optional on failure. */
    std::optional<SceneSpec> load_latest(const DeserializeFn& deserialize) const;

    /** Delete all autosave files in the configured directory. */
    void purge_all();

    /** @return true if autosave is enabled. */
    [[nodiscard]] bool is_enabled() const noexcept { return config_.enabled; }

    void set_enabled(bool enabled) noexcept { config_.enabled = enabled; }

private:
    Config config_;
    double seconds_since_last_save_{0.0};
    bool was_dirty_last_check_{false};

    std::string make_autosave_path() const;
    void trim_old_versions();
};

} // namespace tachyon::editor
