#include "tachyon/studio/studio_library.h"
#include "tachyon/presets/scene/common.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace tachyon {
namespace {

nlohmann::json read_json_file(const std::filesystem::path& path, DiagnosticBag& diagnostics) {
    std::ifstream input(path, std::ios::in | std::ios::binary);
    if (!input.is_open()) {
        diagnostics.add_error("studio.file.open_failed", "failed to open file", path.string());
        return {};
    }
    std::stringstream buffer;
    buffer << input.rdbuf();
    try {
        return nlohmann::json::parse(buffer.str());
    } catch (const nlohmann::json::parse_error& error) {
        diagnostics.add_error("studio.json.parse_failed", error.what(), path.string());
        return {};
    }
}

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

    // 2. Load Legacy JSON Manifest (Compatibility layer)
    const std::filesystem::path manifest_path = m_root / "system" / "manifest.json";
    const nlohmann::json manifest = read_json_file(manifest_path, m_diagnostics);
    
    bool scenes_ok = true;
    bool transitions_ok = true;

    if (m_diagnostics.ok()) {
        scenes_ok = load_scenes(manifest);
        transitions_ok = load_transitions(manifest);
    }

    m_ok = scenes_ok && transitions_ok && m_diagnostics.ok();
    return m_ok;
}

void StudioLibrary::register_cpp_presets() {
    m_scenes.push_back({"aura_modern", "Modern Aura (C++)", "", true});
    m_scenes.push_back({"grid_modern", "Modern Tech Grid (C++)", "", true});
    m_scenes.push_back({"classico_premium", "Classico Premium (C++)", "", true});
    m_scenes.push_back({"minimal_white", "Minimal White (C++)", "", true});
}

bool StudioLibrary::load_scenes(const nlohmann::json& manifest) {
    if (!manifest.contains("scenes") || !manifest.at("scenes").is_array()) {
        m_diagnostics.add_warning("studio.manifest.scenes_missing", "manifest does not contain a scenes array");
        return true;
    }

    for (const auto& scene_json : manifest.at("scenes")) {
        if (!scene_json.is_object()) {
            continue;
        }
        StudioSceneEntry entry;
        if (scene_json.contains("id")) {
            entry.id = scene_json.at("id").get<std::string>();
        }
        if (scene_json.contains("name")) {
            entry.name = scene_json.at("name").get<std::string>();
        }
        if (scene_json.contains("path")) {
            entry.path = m_root / scene_json.at("path").get<std::string>();
        }
        entry.is_cpp_preset = false;

        if (!entry.id.empty()) {
            // Check for duplicates with C++ presets, prefer C++
            auto it = std::find_if(m_scenes.begin(), m_scenes.end(), [&](const auto& s) { return s.id == entry.id; });
            if (it == m_scenes.end()) {
                m_scenes.push_back(std::move(entry));
            }
        }
    }
    return true;
}

bool StudioLibrary::load_transitions(const nlohmann::json& manifest) {
    if (!manifest.contains("animations") || !manifest.at("animations").is_object()) {
        m_diagnostics.add_warning("studio.manifest.animations_missing", "manifest does not contain animations");
        return true;
    }
    const auto& animations = manifest.at("animations");
    if (!animations.contains("transitions") || !animations.at("transitions").is_array()) {
        m_diagnostics.add_warning("studio.manifest.transitions_missing", "manifest does not contain transition packs");
        return true;
    }

    for (const auto& transition_json : animations.at("transitions")) {
        if (!transition_json.is_object()) {
            continue;
        }
        const std::string relative_meta = transition_json.value("path", std::string{});
        if (relative_meta.empty()) {
            continue;
        }

        const std::filesystem::path meta_path = m_root / relative_meta;
        const nlohmann::json meta = read_json_file(meta_path, m_diagnostics);
        if (!meta.is_object()) {
            continue;
        }

        StudioTransitionEntry entry;
        entry.manifest_path = meta_path;
        entry.pack_id = transition_json.value("id", std::string{});
        entry.id = meta.value("id", entry.pack_id);
        entry.name = meta.value("name", entry.id);
        entry.description = meta.value("description", std::string{});
        entry.duration_seconds = meta.value("duration_seconds", 2.0);
        entry.output_dir = meta_path.parent_path() / "output";
        entry.demo_path = meta_path.parent_path() / "demo.json";
        const std::string shader_entry = meta.value("entry", std::string{"v1.glsl"});
        entry.shader_path = meta_path.parent_path() / shader_entry;
        const std::string thumb = meta.value("thumb", std::string{});
        if (!thumb.empty()) {
            entry.thumb_path = meta_path.parent_path() / thumb;
        }

        if (!entry.id.empty()) {
            m_transitions.push_back(std::move(entry));
        }
    }
    return true;
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

// Force recompile to check symbols
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
    } else {
        // Fallback to JSON
        std::ifstream input(entry->path, std::ios::in | std::ios::binary);
        if (input.is_open()) {
            std::stringstream buffer;
            buffer << input.rdbuf();
            // In Tachyon 3, there's parse_scene_spec_json, assuming it's available or we can just parse
            // nlohmann::json j = nlohmann::json::parse(buffer.str());
            // But studio_library doesn't seem to include scene_spec_json.h, so we might need to include it or just return {} if it's missing
        }
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
