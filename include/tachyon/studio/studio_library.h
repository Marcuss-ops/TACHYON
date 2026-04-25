#pragma once

#include "tachyon/core/spec/schema/common/common_spec.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace tachyon {

struct StudioSceneEntry {
    std::string id;
    std::string name;
    std::filesystem::path path;
};

struct StudioTransitionEntry {
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

class StudioLibrary {
public:
    explicit StudioLibrary(std::filesystem::path root = {});

    bool reload();

    [[nodiscard]] bool ok() const noexcept { return m_ok; }
    [[nodiscard]] const DiagnosticBag& diagnostics() const noexcept { return m_diagnostics; }
    [[nodiscard]] const std::filesystem::path& root() const noexcept { return m_root; }

    [[nodiscard]] const std::vector<StudioSceneEntry>& scenes() const noexcept { return m_scenes; }
    [[nodiscard]] const std::vector<StudioTransitionEntry>& transitions() const noexcept { return m_transitions; }

    [[nodiscard]] std::optional<StudioSceneEntry> find_scene(const std::string& id) const;
    [[nodiscard]] std::optional<StudioTransitionEntry> find_transition(const std::string& id) const;

    [[nodiscard]] EffectSpec build_transition_effect(
        const std::string& transition_id,
        const std::string& from_layer_id,
        const std::string& to_layer_id,
        double progress = 0.0) const;

    [[nodiscard]] std::string random_transition_id(uint32_t seed = 0) const;

    [[nodiscard]] EffectSpec build_random_transition(
        const std::string& from_layer_id,
        const std::string& to_layer_id,
        uint32_t seed = 0) const;

private:
    void reset();
    bool load_scenes(const nlohmann::json& manifest);
    bool load_transitions(const nlohmann::json& manifest);

    std::filesystem::path m_root;
    bool m_ok{false};
    DiagnosticBag m_diagnostics;
    std::vector<StudioSceneEntry> m_scenes;
    std::vector<StudioTransitionEntry> m_transitions;
};

}  // namespace tachyon
