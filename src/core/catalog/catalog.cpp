#include "tachyon/core/catalog/catalog.h"
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

std::filesystem::path default_catalog_root() {
    // Moved from "studio/library" to "assets/catalog"
    return std::filesystem::current_path() / "assets" / "catalog";
}

std::string trim_copy(std::string value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return {};
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

struct TransitionCatalogEntry {
    std::string id;
    std::string name;
    std::string description;
    std::string shader_file;
    std::string thumb_file;
};

bool parse_transition_catalog_line(const std::string& line, TransitionCatalogEntry& entry) {
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

std::filesystem::path resolve_catalog_path(const std::filesystem::path& root, const std::string& value) {
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

TachyonCatalog::TachyonCatalog(std::filesystem::path root)
    : m_root(root.empty() ? default_catalog_root() : std::move(root)) {
    reload();
}

void TachyonCatalog::reset() {
    m_ok = false;
    m_diagnostics = {};
    m_scenes.clear();
    m_transitions.clear();
}

bool TachyonCatalog::reload() {
    reset();

    // 1. Register Built-in C++ Presets (Modern approach)
    register_cpp_presets();
    register_transition_assets();

    m_ok = m_diagnostics.ok();
    return m_ok;
}

void TachyonCatalog::register_cpp_presets() {
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

void TachyonCatalog::register_transition_assets() {
    const auto root = m_root / "transitions";
    const auto catalog_path = root / "catalog.txt";
    if (!std::filesystem::exists(catalog_path)) {
        return;
    }

    std::ifstream catalog(catalog_path);
    if (!catalog) {
        return;
    }

    const auto output_dir = root / "output";
    std::string line;
    while (std::getline(catalog, line)) {
        TransitionCatalogEntry row;
        if (!parse_transition_catalog_line(line, row)) {
            continue;
        }

        const auto shader_path = resolve_catalog_path(root, row.shader_file);
        const auto thumb_path = resolve_catalog_path(root, row.thumb_file);

        CatalogTransitionEntry transition;
        transition.id = row.id;
        transition.name = row.name.empty() ? transition_slug_from_id(row.id) : row.name;
        transition.pack_id = "public.transitions";
        transition.description = row.description;
        transition.manifest_path = catalog_path;
        transition.demo_path = output_dir / (transition_slug_from_id(row.id) + ".mp4");
        transition.output_dir = output_dir;
        transition.shader_path = shader_path;
        transition.thumb_path = thumb_path;
        transition.duration_seconds = 0.8;

        if (const auto* reg_spec = presets::TransitionPresetRegistry::instance().find(transition.id)) {
            transition.name = reg_spec->metadata.display_name;
            transition.description = reg_spec->metadata.description;
        }

        m_transitions.push_back(std::move(transition));
    }
}

std::optional<CatalogSceneEntry> TachyonCatalog::find_scene(const std::string& id) const {
    const auto it = std::find_if(m_scenes.begin(), m_scenes.end(), [&](const CatalogSceneEntry& entry) {
        return entry.id == id;
    });
    if (it == m_scenes.end()) {
        return std::nullopt;
    }
    return *it;
}

std::optional<CatalogTransitionEntry> TachyonCatalog::find_transition(const std::string& id) const {
    const auto it = std::find_if(m_transitions.begin(), m_transitions.end(), [&](const CatalogTransitionEntry& entry) {
        return entry.id == id;
    });
    if (it == m_transitions.end()) {
        return std::nullopt;
    }
    return *it;
}

SceneSpec TachyonCatalog::instantiate_scene(const std::string& id) const {
    auto entry = find_scene(id);
    if (!entry) return {};

    if (entry->is_cpp_preset) {
        if (auto scene = presets::ScenePresetRegistry::instance().create(id, {})) {
            return std::move(*scene);
        }
    }
    return {};
}

EffectSpec TachyonCatalog::build_transition_effect(
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

    // Fallback logic for missing shaders
    if (std::filesystem::exists(transition->shader_path)) {
        effect.strings["shader_path"] = transition->shader_path.generic_string();
    } else {
        // Log warning or diagnostic
        // Use default crossfade shader as fallback if available
        const auto fallback_shader = m_root / "transitions" / "shaders" / "crossfade.glsl";
        if (std::filesystem::exists(fallback_shader)) {
            effect.strings["shader_path"] = fallback_shader.generic_string();
        } else {
            effect.enabled = false; // Disable if even fallback is missing
        }
    }

    effect.strings["manifest_path"] = transition->manifest_path.generic_string();
    effect.strings["pack_id"] = transition->pack_id;
    return effect;
}

}  // namespace tachyon
