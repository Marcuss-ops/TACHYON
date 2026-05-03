#include "tachyon/studio/studio_library.h"
#include "tachyon/presets/scene/common.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace tachyon {
namespace {

std::filesystem::path default_library_root() {
    return std::filesystem::current_path() / "studio" / "library";
}

}  // namespace

StudioLibrary::StudioLibrary(std::filesystem::path root)
    : m_root(root.empty() ? default_library_root() : std::move(root)) {
    reload();
}

void StudioLibrary::reset() {
    m_ok = false;
    m_diagnostics = {};
    m_scenes.clear();
    m_transitions.clear();
}

bool StudioLibrary::reload() {
    reset();

    // 1. Register Built-in C++ Presets (Modern approach)
    register_cpp_presets();

    m_ok = m_diagnostics.ok();
    return m_ok;
}

void StudioLibrary::register_cpp_presets() {
    m_scenes.push_back({"aura_modern", "Modern Aura (C++)", "", true});
    m_scenes.push_back({"grid_modern", "Modern Tech Grid (C++)", "", true});
    m_scenes.push_back({"classico_premium", "Classico Premium (C++)", "", true});
    m_scenes.push_back({"minimal_white", "Minimal White (C++)", "", true});
}

std::optional<StudioSceneEntry> StudioLibrary::find_scene(const std::string& id) const {
    const auto it = std::find_if(m_scenes.begin(), m_scenes.end(), [&](const StudioSceneEntry& entry) {
        return entry.id == id;
    });
    if (it == m_scenes.end()) {
        return std::nullopt;
    }
    return *it;
}

std::optional<StudioTransitionEntry> StudioLibrary::find_transition(const std::string& id) const {
    const auto it = std::find_if(m_transitions.begin(), m_transitions.end(), [&](const StudioTransitionEntry& entry) {
        return entry.id == id;
    });
    if (it == m_transitions.end()) {
        return std::nullopt;
    }
    return *it;
}

SceneSpec StudioLibrary::instantiate_scene(const std::string& id) const {
    auto entry = find_scene(id);
    if (!entry) return {};

    if (entry->is_cpp_preset) {
        if (id == "aura_modern") return presets::scene::build_enhanced_text_scene();
        if (id == "grid_modern") return presets::scene::build_modern_grid_scene();
        if (id == "classico_premium") return presets::scene::build_classico_premium_scene();
        if (id == "minimal_white") return presets::scene::build_minimal_text_scene();
    }
    return {};
}

EffectSpec StudioLibrary::build_transition_effect(
    const std::string& transition_id,
    const std::string& from_layer_id,
    const std::string& to_layer_id,
    double progress) const {
    EffectSpec effect;
    const auto transition = find_transition(transition_id);
    if (!transition.has_value()) {
        effect.enabled = false;
        return effect;
    }

    effect.enabled = true;
    effect.type = "glsl_transition";
    effect.scalars["t"] = progress;
    effect.scalars["duration_seconds"] = transition->duration_seconds;
    effect.strings["transition_id"] = transition->id;
    effect.strings["from_layer_id"] = from_layer_id;
    effect.strings["transition_to_layer_id"] = to_layer_id;
    effect.strings["shader_path"] = transition->shader_path.generic_string();
    effect.strings["manifest_path"] = transition->manifest_path.generic_string();
    effect.strings["pack_id"] = transition->pack_id;
    return effect;
}

}  // namespace tachyon
