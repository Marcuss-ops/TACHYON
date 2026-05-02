#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include "tachyon/core/spec/scene_spec_audio.h"
#include "tachyon/text/fonts/management/font_manifest.h"
#include "tachyon/core/spec/json_scene_utils.h"
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
    if (!composition.fps.has_value() && composition.frame_rate.denominator != 0) {
        composition.fps = static_cast<std::int64_t>(composition.frame_rate.value());
    }
    if (object.contains("background")) {
        composition.background = object.at("background").get<BackgroundSpec>();
    }
    if (object.contains("environment") && object.at("environment").is_string()) composition.environment_path = object.at("environment").get<std::string>();
    if (object.contains("layers") && object.at("layers").is_array()) {
        const auto& layers = object.at("layers");
        for (std::size_t i = 0; i < layers.size(); ++i) {
            if (layers[i].is_object()) {
                LayerSpec layer;
                parse_layer(layers[i], layer, make_path(path, "layers[" + std::to_string(i) + "]"), diagnostics);
                composition.layers.push_back(std::move(layer));
            }
        }
    }
    parse_composition_audio_tracks(composition, object, path, diagnostics);
    
    if (object.contains("camera_cuts") && object.at("camera_cuts").is_array()) {
        const auto& cuts = object.at("camera_cuts");
        for (std::size_t i = 0; i < cuts.size(); ++i) {
            if (!cuts[i].is_object()) continue;
            const auto& c = cuts[i];
            spec::CameraCut cut;
            read_string(c, "camera_id", cut.camera_id);
            read_number(c, "start_seconds", cut.start_seconds);
            read_number(c, "end_seconds", cut.end_seconds);
            composition.camera_cuts.push_back(std::move(cut));
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

ParseResult<SceneSpec> parse_scene_spec_json(const std::string& text, const std::filesystem::path& base_dir) {
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

        // Parse font_manifest if present
        if (parsed.contains("font_manifest") && parsed.at("font_manifest").is_string()) {
            std::string fm_path = parsed.at("font_manifest").get<std::string>();
            scene.font_manifest_path = fm_path;
            std::filesystem::path fm_fs_path(fm_path);
            if (fm_fs_path.is_relative() && !base_dir.empty()) {
                fm_fs_path = base_dir / fm_fs_path;
            }
            auto manifest_opt = tachyon::text::FontManifestParser::parse_file(fm_fs_path);
            if (manifest_opt.has_value()) {
                scene.font_manifest = std::move(*manifest_opt);
            } else {
                result.diagnostics.add_warning("scene.font_manifest.load_failed",
                    "failed to load font manifest from: " + fm_fs_path.string());
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
    std::filesystem::path base_dir = path.parent_path();
    return parse_scene_spec_json(buffer.str(), base_dir);
}

// ---------------------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------------------

// Note: layer serialization is now handled in serialization/layer_serialize.cpp

json serialize_composition(const CompositionSpec& comp) {
    json j;
    j["id"] = comp.id;
    j["name"] = comp.name;
    j["width"] = comp.width;
    j["height"] = comp.height;
    j["duration"] = comp.duration;
    j["frame_rate"] = { {"numerator", comp.frame_rate.numerator}, {"denominator", comp.frame_rate.denominator} };
    if (comp.fps.has_value()) j["fps"] = *comp.fps;
    if (comp.background.has_value()) j["background"] = *comp.background;
    if (comp.environment_path.has_value()) j["environment"] = *comp.environment_path;

    j["layers"] = json::array();
    for (const auto& layer : comp.layers) {
        j["layers"].push_back(serialize_layer(layer));
    }

    if (!comp.audio_tracks.empty()) {
        j["audio_tracks"] = json::array();
        for (const auto& track : comp.audio_tracks) {
            json t;
            t["id"] = track.id;
            t["source_path"] = track.source_path;
            t["volume"] = track.volume;
            t["pan"] = track.pan;
            t["start_offset_seconds"] = track.start_offset_seconds;
            j["audio_tracks"].push_back(t);
        }
    }

    if (!comp.camera_cuts.empty()) {
        j["camera_cuts"] = json::array();
        for (const auto& cut : comp.camera_cuts) {
            json c;
            c["camera_id"] = cut.camera_id;
            c["start_seconds"] = cut.start_seconds;
            c["end_seconds"] = cut.end_seconds;
            j["camera_cuts"].push_back(c);
        }
    }

    return j;
}

json serialize_scene_spec(const SceneSpec& scene) {
    json j;
    j["version"] = scene.version;
    j["spec_version"] = scene.spec_version;
    j["project"] = spec::serialize_project(scene.project);

    j["compositions"] = json::array();
    for (const auto& comp : scene.compositions) {
        j["compositions"].push_back(serialize_composition(comp));
    }

    if (!scene.assets.empty()) {
        j["assets"] = json::array();
        for (const auto& asset : scene.assets) {
            json a;
            a["id"] = asset.id;
            a["type"] = asset.type;
            a["source"] = asset.source;
            if (!asset.path.empty()) a["path"] = asset.path;
            if (asset.alpha_mode.has_value() && !asset.alpha_mode->empty()) a["alpha_mode"] = *asset.alpha_mode;
            j["assets"].push_back(a);
        }
    }

    return j;
}

} // namespace tachyon
