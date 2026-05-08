#include "tachyon/core/library/library.h"
#include "tachyon/presets/scene/common.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include "tachyon/presets/transition/transition_preset_registry.h"
#include "tachyon/presets/scene/scene_preset_registry.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string_view>

namespace tachyon {
namespace {

std::filesystem::path default_library_root() {
    return std::filesystem::current_path() / "assets" / "library";
}

std::string trim_copy(std::string value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return {};
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

struct TransitionLibraryEntry {
    std::string id;
    std::string name;
    std::string description;
    std::string shader_file;
    std::string thumb_file;
};

bool parse_transition_library_line(const std::string& line, TransitionLibraryEntry& entry) {
    const std::string trimmed = trim_copy(line);
    if (trimmed.empty() || trimmed.front() == '#') {
        return false;
    }

    std::stringstream ss(trimmed);
    std::string fields[5];
    for (std::string& field : fields) {
        if (!std::getline(ss, field, '|')) {
            return false;
        }
        field = trim_copy(field);
    }

    entry.id = std::move(fields[0]);
    entry.name = std::move(fields[1]);
    entry.description = std::move(fields[2]);
    entry.shader_file = std::move(fields[3]);
    entry.thumb_file = std::move(fields[4]);
    return !entry.id.empty() && !entry.shader_file.empty() && !entry.thumb_file.empty();
}

std::filesystem::path resolve_library_path(const std::filesystem::path& root, const std::string& value) {
    const std::filesystem::path path = value;
    return path.is_absolute() ? path : root / path;
}

std::string transition_slug_from_id(const std::string& id) {
    constexpr std::string_view prefix = "tachyon.transition.";
    if (id.rfind(prefix, 0) == 0) {
        return id.substr(prefix.size());
    }
    return id;
}

}  // namespace

TachyonLibrary::TachyonLibrary(std::filesystem::path root)
    : m_root(root.empty() ? default_library_root() : std::move(root)) {
    reload();
}

void TachyonLibrary::reset() {
    m_ok = false;
    m_diagnostics = {};
    m_scenes.clear();
    m_transitions.clear();
}

bool TachyonLibrary::reload() {
    reset();

    register_cpp_presets();
    register_transition_assets();

    m_ok = m_diagnostics.ok();
    return m_ok;
}

void TachyonLibrary::register_cpp_presets() {
    const auto& registry = presets::ScenePresetRegistry::instance();
    for (const auto& id : registry.list_ids()) {
        if (const auto* spec = registry.find(id)) {
            m_scenes.push_back({
                spec->id,
                spec->metadata.display_name,
                spec->metadata.description,
                true // is_cpp_preset
            });
        }
    }
}

void TachyonLibrary::register_transition_assets() {
    const auto root = m_root / "transitions";
    const auto library_path = root / "library.txt";
    if (!std::filesystem::exists(library_path)) {
        return;
    }

    std::ifstream library_file(library_path);
    if (!library_file) {
        m_diagnostics.add_error("io", "Failed to open library listing at " + library_path.string());
        return;
    }

    const auto output_dir = root / "output";
    std::string line;
    while (std::getline(library_file, line)) {
        TransitionLibraryEntry row;
        if (!parse_transition_library_line(line, row)) {
            continue;
        }

        const auto shader_path = resolve_library_path(root, row.shader_file);
        const auto thumb_path = resolve_library_path(root, row.thumb_file);

        LibraryTransitionEntry transition;
        transition.id = row.id;
        transition.pack_id = "public.transitions";
        transition.manifest_path = library_path;
        transition.demo_path = output_dir / (transition_slug_from_id(row.id) + ".mp4");
        transition.output_dir = output_dir;
        transition.shader_path = shader_path;
        transition.thumb_path = thumb_path;

        transition.name = row.name.empty() ? transition_slug_from_id(row.id) : row.name;
        transition.description = row.description;
        transition.duration_seconds = 0.8;

        presets::TransitionPresetRegistry preset_reg;
        if (const auto* reg_spec = preset_reg.find(transition.id)) {
            transition.name = reg_spec->metadata.display_name;
            transition.description = reg_spec->metadata.description;
            auto default_spec = reg_spec->factory({});
            transition.duration_seconds = default_spec.duration;
        }

        m_transitions.push_back(std::move(transition));
    }

    presets::TransitionPresetRegistry preset_reg;
    for (const auto& id : preset_reg.list_ids()) {
        if (std::find_if(m_transitions.begin(), m_transitions.end(), [&](const auto& e) { return e.id == id; }) != m_transitions.end()) {
            continue;
        }

        if (const auto* spec = preset_reg.find(id)) {
            auto default_spec = spec->factory({});

            LibraryTransitionEntry transition;
            transition.id = spec->id;
            transition.name = spec->metadata.display_name;
            transition.description = spec->metadata.description;
            transition.pack_id = "builtin.transitions";
            transition.manifest_path = "TransitionManifest";
            transition.duration_seconds = default_spec.duration;
            
            transition.shader_path = ""; 
            transition.thumb_path = spec->metadata.tags.empty() ? "" : spec->metadata.tags[0];

            m_transitions.push_back(std::move(transition));
        }
    }
}

std::optional<LibrarySceneEntry> TachyonLibrary::find_scene(const std::string& id) const {
    const auto it = std::find_if(m_scenes.begin(), m_scenes.end(), [&](const LibrarySceneEntry& entry) {
        return entry.id == id;
    });
    if (it == m_scenes.end()) {
        return std::nullopt;
    }
    return *it;
}

std::optional<LibraryTransitionEntry> TachyonLibrary::find_transition(const std::string& id) const {
    const auto it = std::find_if(m_transitions.begin(), m_transitions.end(), [&](const LibraryTransitionEntry& entry) {
        return entry.id == id;
    });
    if (it == m_transitions.end()) {
        return std::nullopt;
    }
    return *it;
}

SceneSpec TachyonLibrary::instantiate_scene(const std::string& id) const {
    auto entry = find_scene(id);
    if (!entry) return {};

    if (auto scene = presets::ScenePresetRegistry::instance().create(id, {})) {
        return std::move(*scene);
    }
    return {};
}

EffectSpec TachyonLibrary::build_transition_effect(
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

    if (std::filesystem::exists(transition->shader_path)) {
        effect.strings["shader_path"] = transition->shader_path.generic_string();
    } else {
        const auto fallback_shader = m_root / "transitions" / "shaders" / "crossfade.glsl";
        if (std::filesystem::exists(fallback_shader)) {
            effect.strings["shader_path"] = fallback_shader.generic_string();
        } else {
            effect.enabled = false;
        }
    }

    effect.strings["manifest_path"] = transition->manifest_path.generic_string();
    effect.strings["pack_id"] = transition->pack_id;
    return effect;
}

}  // namespace tachyon
