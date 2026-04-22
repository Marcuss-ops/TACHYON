#include "tachyon/core/spec/scene_spec_core.h"
#include <fstream>
#include <sstream>

namespace tachyon {

CompositionSpec parse_composition(const json& object, const std::string& path, DiagnosticBag& diagnostics) {
    CompositionSpec composition;
    read_string(object, "id", composition.id);
    read_string(object, "name", composition.name);
    read_number(object, "width", composition.width);
    read_number(object, "height", composition.height);
    read_number(object, "duration", composition.duration);
    if (object.contains("fps") && object.at("fps").is_number_integer()) {
        composition.fps = object.at("fps").get<std::int64_t>();
        composition.frame_rate.numerator = *composition.fps;
        composition.frame_rate.denominator = 1;
    }
    if (object.contains("frame_rate")) {
        const auto& fr = object.at("frame_rate");
        if (fr.is_number_integer()) { composition.frame_rate.numerator = fr.get<std::int64_t>(); composition.frame_rate.denominator = 1; }
        else if (fr.is_object()) { read_number(fr, "numerator", composition.frame_rate.numerator); read_number(fr, "denominator", composition.frame_rate.denominator); }
    }
    if (object.contains("background") && object.at("background").is_string()) composition.background = object.at("background").get<std::string>();
    if (object.contains("environment") && object.at("environment").is_string()) composition.environment_path = object.at("environment").get<std::string>();
    if (object.contains("layers") && object.at("layers").is_array()) {
        const auto& layers = object.at("layers");
        for (std::size_t i = 0; i < layers.size(); ++i) {
            if (layers[i].is_object()) composition.layers.push_back(parse_layer(layers[i], make_path(path, "layers[" + std::to_string(i) + "]"), diagnostics));
        }
    }
    return composition;
}

AssetSpec parse_asset(const json& object, const std::string& path, DiagnosticBag& diagnostics) {
    (void)path; (void)diagnostics;
    AssetSpec asset;
    read_string(object, "id", asset.id);
    read_string(object, "type", asset.type);
    read_string(object, "source", asset.source);
    read_string(object, "path", asset.path);
    if (object.contains("alpha_mode") && object.at("alpha_mode").is_string()) asset.alpha_mode = object.at("alpha_mode").get<std::string>();
    if (asset.source.empty() && !asset.path.empty()) asset.source = asset.path;
    return asset;
}

ParseResult<SceneSpec> parse_scene_spec_json(const std::string& text) {
    ParseResult<SceneSpec> result;
    try {
        const json parsed = json::parse(text);
        if (!parsed.is_object()) {
            result.diagnostics.add_error("scene.json.root_invalid", "scene root must be an object");
            return result;
        }

        SceneSpec scene;
        if (parsed.contains("version") && parsed.at("version").is_string()) scene.version = parsed.at("version").get<std::string>();
        if (parsed.contains("spec_version") && parsed.at("spec_version").is_string()) scene.spec_version = parsed.at("spec_version").get<std::string>();
        if (parsed.contains("project") && parsed.at("project").is_object()) {
            const auto& project = parsed.at("project");
            read_string(project, "id", scene.project.id);
            read_string(project, "name", scene.project.name);
            read_string(project, "authoring_tool", scene.project.authoring_tool);
            read_optional_int(project, "root_seed", scene.project.root_seed);
        }

        if (parsed.contains("compositions") && parsed.at("compositions").is_array()) {
            const auto& compositions = parsed.at("compositions");
            for (std::size_t i = 0; i < compositions.size(); ++i) {
                if (compositions[i].is_object()) {
                    scene.compositions.push_back(parse_composition(compositions[i], make_path("scene", "compositions[" + std::to_string(i) + "]"), result.diagnostics));
                }
            }
        }

        if (parsed.contains("assets") && parsed.at("assets").is_array()) {
            const auto& assets = parsed.at("assets");
            for (std::size_t i = 0; i < assets.size(); ++i) {
                if (assets[i].is_object()) {
                    scene.assets.push_back(parse_asset(assets[i], make_path("scene", "assets[" + std::to_string(i) + "]"), result.diagnostics));
                }
            }
        }

        result.value = std::move(scene);
        return result;
    } catch (const std::exception& ex) {
        result.diagnostics.add_error("scene.json.parse_failed", std::string("failed to parse scene json: ") + ex.what());
        return result;
    }
}

ParseResult<SceneSpec> parse_scene_spec_file(const std::filesystem::path& path) {
    ParseResult<SceneSpec> result;
    std::ifstream file(path);
    if (!file) {
        result.diagnostics.add_error("scene.file.open_failed", "failed to open scene file: " + path.string());
        return result;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return parse_scene_spec_json(buffer.str());
}

} // namespace tachyon
