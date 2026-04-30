#include "tachyon/studio/studio_library.h"

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

    const std::filesystem::path manifest_path = m_root / "system" / "manifest.json";
    const nlohmann::json manifest = read_json_file(manifest_path, m_diagnostics);
    if (!m_diagnostics.ok()) {
        return false;
    }

    const bool scenes_ok = load_scenes(manifest);
    const bool transitions_ok = load_transitions(manifest);

    m_ok = scenes_ok && transitions_ok && m_diagnostics.ok();
    return m_ok;
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
        if (!entry.id.empty()) {
            m_scenes.push_back(std::move(entry));
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

std::optional<StudioTransitionEntry> StudioLibrary::find_transition(const std::string& id) const {
    const auto it = std::find_if(m_transitions.begin(), m_transitions.end(), [&](const StudioTransitionEntry& entry) {
        return entry.id == id;
    });
    if (it == m_transitions.end()) {
        return std::nullopt;
    }
    return *it;
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
