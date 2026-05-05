#pragma once

#include "tachyon/core/spec/schema/common/common_spec.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace tachyon {

struct CatalogSceneEntry {
    std::string id;
    std::string name;
    std::filesystem::path path;
    bool is_cpp_preset{false};
};

struct CatalogTransitionEntry {
    std::string id;
    std::string name;
    std::string pack_id;
    std::string description;
    std::filesystem::path manifest_path;
    std::filesystem::path demo_path;
    std::filesystem::path output_dir;
    std::filesystem::path shader_path;
    std::filesystem::path thumb_path;
    double duration_seconds{2.0};
};

/**
 * @brief Manages the library of assets, scenes, and transitions available to the engine.
 * Formerly known as StudioLibrary.
 */
class TachyonCatalog {
public:
    explicit TachyonCatalog(std::filesystem::path root = {});

    bool reload();

    [[nodiscard]] bool ok() const noexcept { return m_ok; }
    [[nodiscard]] const DiagnosticBag& diagnostics() const noexcept { return m_diagnostics; }
    [[nodiscard]] const std::filesystem::path& root() const noexcept { return m_root; }

    [[nodiscard]] const std::vector<CatalogSceneEntry>& scenes() const noexcept { return m_scenes; }
    [[nodiscard]] const std::vector<CatalogTransitionEntry>& transitions() const noexcept { return m_transitions; }

    [[nodiscard]] std::optional<CatalogSceneEntry> find_scene(const std::string& id) const;

    [[nodiscard]] std::optional<CatalogTransitionEntry> find_transition(const std::string& id) const;

    [[nodiscard]] SceneSpec instantiate_scene(const std::string& id) const;

    [[nodiscard]] EffectSpec build_transition_effect(
        const std::string& transition_id,
        const std::string& from_layer_id,
        const std::string& to_layer_id,
        double progress = 0.0) const;

private:
    void reset();
    void register_cpp_presets();
    void register_transition_assets();

    std::filesystem::path m_root;
    bool m_ok{false};
    DiagnosticBag m_diagnostics;
    std::vector<CatalogSceneEntry> m_scenes;
    std::vector<CatalogTransitionEntry> m_transitions;
};

}  // namespace tachyon
