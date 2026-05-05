#include "tachyon/studio/studio_library.h"
#include "tachyon/presets/scene/common.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include "tachyon/presets/transition/transition_preset_registry.h"

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
    register_transition_assets();

    m_ok = m_diagnostics.ok();
    return m_ok;
}

void StudioLibrary::register_cpp_presets() {
    m_scenes.push_back({"tachyon.scene.enhanced_text", "Modern Aura (C++)", "", true});
    m_scenes.push_back({"tachyon.scene.modern_grid", "Modern Tech Grid (C++)", "", true});
    m_scenes.push_back({"tachyon.scene.classico_premium", "Classico Premium (C++)", "", true});
    m_scenes.push_back({"tachyon.scene.minimal_text", "Minimal White (C++)", "", true});
}

void StudioLibrary::register_transition_assets() {
    const auto root = m_root / "animations" / "transitions";
    if (!std::filesystem::exists(root)) return;

    for (const auto& entry : std::filesystem::directory_iterator(root)) {
        if (!entry.is_directory()) continue;

        const auto id = entry.path().filename().string();
        const auto dir = entry.path();
        
        // Basic validation: shader must exist.
        if (!std::filesystem::exists(dir / "v1.glsl")) continue;

        StudioTransitionEntry transition;
        transition.id = "tachyon.transition." + id;
        transition.name = id; // Default name
        transition.pack_id = "public.transitions";
        transition.description = "";
        transition.manifest_path = dir / "v1.glsl";
        transition.demo_path = dir / "output" / (id + ".mp4");
        transition.output_dir = dir / "output";
        transition.shader_path = dir / "v1.glsl";
        transition.thumb_path = dir / "thumb.png";
        transition.duration_seconds = 0.8;

        // Enrich with registry metadata if available (canonical source of truth)
        if (const auto* reg_spec = presets::TransitionPresetRegistry::instance().find(transition.id)) {
            transition.name = reg_spec->metadata.display_name;
            transition.description = reg_spec->metadata.description;
        }

        m_transitions.push_back(std::move(transition));
    }
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
        if (id == "tachyon.scene.enhanced_text") return presets::scene::build_enhanced_text_scene();
        if (id == "tachyon.scene.modern_grid") return presets::scene::build_modern_grid_scene();
        if (id == "tachyon.scene.classico_premium") return presets::scene::build_classico_premium_scene();
        if (id == "tachyon.scene.minimal_text") return presets::scene::build_minimal_text_scene();
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
    effect.type = "tachyon.effect.transition.glsl";
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
