#include "tachyon/core/spec/scene_spec_core.h"
#include <fstream>
#include <set>
#include <sstream>

namespace tachyon {

bool is_version_like(const std::string& version) {
    if (version.empty()) return false;
    bool saw_digit = false;
    bool saw_dot = false;
    bool last_was_dot = false;
    for (const char ch : version) {
        if (ch >= '0' && ch <= '9') { saw_digit = true; last_was_dot = false; continue; }
        if (ch == '.') { if (!saw_digit || last_was_dot) return false; saw_dot = true; last_was_dot = true; continue; }
        return false;
    }
    return saw_digit && !last_was_dot && saw_dot;
}

std::string make_path(const std::string& parent, const std::string& child) {
    if (parent.empty()) return child;
    if (child.empty()) return parent;
    return parent + "." + child;
}

bool is_asset_alpha_mode_valid(const std::string& mode) {
    return mode == "premultiplied" || mode == "straight" || mode == "opaque";
}

bool is_layer_blend_mode_valid(const std::string& mode) {
    return mode == "normal" || mode == "additive" || mode == "multiply" || mode == "screen" || mode == "overlay" || mode == "soft_light" || mode == "softLight";
}

ParseResult<SceneSpec> parse_scene_spec_json(const std::string& text) {
    ParseResult<SceneSpec> result;
    json root;
    try {
        root = json::parse(text);
    } catch (const json::parse_error& error) {
        result.diagnostics.add_error("scene.json.parse_failed", error.what());
        return result;
    }

    if (!root.is_object()) {
        result.diagnostics.add_error("scene.json.root_invalid", "scene spec root must be an object");
        return result;
    }

    SceneSpec scene;
    read_string(root, "version", scene.version);
    read_string(root, "spec_version", scene.spec_version);
    if (scene.spec_version.empty() && !scene.version.empty()) scene.spec_version = scene.version;

    if (root.contains("project") && root.at("project").is_object()) {
        const auto& project = root.at("project");
        read_string(project, "id", scene.project.id);
        read_string(project, "name", scene.project.name);
        read_string(project, "authoring_tool", scene.project.authoring_tool);
        read_optional_int(project, "root_seed", scene.project.root_seed);
    } else {
        result.diagnostics.add_error("scene.project.missing", "project object is required", "project");
    }

    if (root.contains("assets") && root.at("assets").is_array()) {
        const auto& assets = root.at("assets");
        for (std::size_t i = 0; i < assets.size(); ++i) {
            if (assets[i].is_object()) scene.assets.push_back(parse_asset(assets[i], "assets[" + std::to_string(i) + "]", result.diagnostics));
        }
    }

    if (root.contains("compositions") && root.at("compositions").is_array()) {
        const auto& compositions = root.at("compositions");
        for (std::size_t i = 0; i < compositions.size(); ++i) {
            if (compositions[i].is_object()) scene.compositions.push_back(parse_composition(compositions[i], "compositions[" + std::to_string(i) + "]", result.diagnostics));
        }
    }

    if (root.contains("render_defaults") && root.at("render_defaults").is_object()) {
        const auto& rd = root.at("render_defaults");
        if (rd.contains("output") && rd.at("output").is_string()) scene.render_defaults.output = rd.at("output").get<std::string>();
    }

    if (result.diagnostics.ok()) result.value = std::move(scene);
    return result;
}

ParseResult<SceneSpec> parse_scene_spec_file(const std::filesystem::path& path) {
    ParseResult<SceneSpec> result;
    std::ifstream input(path, std::ios::in | std::ios::binary);
    if (!input.is_open()) {
        result.diagnostics.add_error("scene.file.open_failed", "failed to open scene spec file: " + path.string(), path.string());
        return result;
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return parse_scene_spec_json(buffer.str());
}

ValidationResult validate_scene_spec(const SceneSpec& scene) {
    ValidationResult result;
    if (scene.spec_version.empty()) result.diagnostics.add_error("scene.version_missing", "version is required", "version");
    else if (!is_version_like(scene.spec_version)) result.diagnostics.add_error("scene.spec_version.invalid", "spec_version invalid", "spec_version");
    if (scene.project.id.empty()) result.diagnostics.add_error("scene.project.id_missing", "project.id is required", "project.id");
    if (scene.compositions.empty()) result.diagnostics.add_error("scene.compositions.missing", "at least one composition is required", "compositions");

    std::set<std::string> comp_ids;
    for (std::size_t i = 0; i < scene.compositions.size(); ++i) {
        const auto& comp = scene.compositions[i];
        std::string path = "compositions[" + std::to_string(i) + "]";
        if (comp.id.empty()) result.diagnostics.add_error("scene.composition.id_missing", "id required", path + ".id");
        else if (!comp_ids.insert(comp.id).second) result.diagnostics.add_error("scene.composition.id_duplicate", "id must be unique", path + ".id");
        if (comp.width <= 0 || comp.height <= 0) result.diagnostics.add_error("scene.composition.size_invalid", "size must be positive", path + ".width");

        std::set<std::string> layer_ids;
        for (std::size_t j = 0; j < comp.layers.size(); ++j) {
            const auto& layer = comp.layers[j];
            std::string lpath = path + ".layers[" + std::to_string(j) + "]";
            if (layer.id.empty()) result.diagnostics.add_error("scene.layer.id_missing", "id required", lpath + ".id");
            else if (!layer_ids.insert(layer.id).second) result.diagnostics.add_error("scene.layer.id_duplicate", "id duplicate", lpath + ".id");
            if (!is_layer_blend_mode_valid(layer.blend_mode)) result.diagnostics.add_error("scene.layer.blend_mode_invalid", "blend_mode invalid", lpath + ".blend_mode");
        }
    }
    return result;
}

} // namespace tachyon
