#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include "tachyon/core/spec/scene_spec_audio.h"
#include "tachyon/text/fonts/font_manifest.h"
#include <fstream>
#include <sstream>

using json = nlohmann::json;

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

    if (object.contains("variable_decls") && object.at("variable_decls").is_array()) {
        const auto& var_decls = object.at("variable_decls");
        for (std::size_t i = 0; i < var_decls.size(); ++i) {
            if (!var_decls[i].is_object()) continue;
            const auto& v = var_decls[i];
            VariableDecl decl;
            read_string(v, "name", decl.name);
            read_string(v, "type", decl.type);
            composition.variable_decls.push_back(std::move(decl));
        }
    }

    // Parse input_props
    if (object.contains("input_props") && object.at("input_props").is_object()) {
        for (auto& [key, val] : object.at("input_props").items()) {
            composition.input_props[key] = val;
        }
    }

    // Parse components
    if (object.contains("components") && object.at("components").is_array()) {
        const auto& comps = object.at("components");
        for (std::size_t i = 0; i < comps.size(); ++i) {
            if (!comps[i].is_object()) continue;
            const auto& c = comps[i];
            ComponentSpec comp_spec;
            read_string(c, "id", comp_spec.id);
            read_string(c, "name", comp_spec.name);
            if (c.contains("params") && c.at("params").is_array()) {
                const auto& params = c.at("params");
                for (std::size_t j = 0; j < params.size(); ++j) {
                    if (!params[j].is_object()) continue;
                    VariableDecl vd;
                    read_string(params[j], "name", vd.name);
                    read_string(params[j], "type", vd.type);
                    comp_spec.params.push_back(vd);
                }
            }
            // TODO: parse layers inside component
            composition.components.push_back(std::move(comp_spec));
        }
    }

    // Parse component instances
    if (object.contains("component_instances") && object.at("component_instances").is_array()) {
        const auto& instances = object.at("component_instances");
        for (std::size_t i = 0; i < instances.size(); ++i) {
            if (!instances[i].is_object()) continue;
            const auto& inst = instances[i];
            ComponentInstanceSpec inst_spec;
            read_string(inst, "component_id", inst_spec.component_id);
            read_string(inst, "instance_id", inst_spec.instance_id);
            if (inst.contains("param_values") && inst.at("param_values").is_object()) {
                for (auto& [key, val] : inst.at("param_values").items()) {
                    inst_spec.param_values[key] = val.dump();
                }
            }
            composition.component_instances.push_back(std::move(inst_spec));
        }
    }
    
    composition.spec_hash = compute_composition_hash(composition);
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

        if (parsed.contains("parameters") && parsed.at("parameters").is_array()) {
            const auto& params = parsed.at("parameters");
            for (const auto& p : params) {
                if (!p.is_object()) continue;
                ParameterDefinition def;
                read_string(p, "name", def.name);
                std::string type_str;
                read_string(p, "type", type_str);
                if (type_str == "float") def.type = ParameterType::Float;
                else if (type_str == "string") def.type = ParameterType::String;
                else if (type_str == "color") def.type = ParameterType::Color;
                else if (type_str == "vector2") def.type = ParameterType::Vector2;
                else if (type_str == "bool") def.type = ParameterType::Bool;
                
                if (p.contains("default")) {
                    def.default_value = p.at("default");
                }
                scene.parameters.push_back(std::move(def));
            }
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

        scene.spec_hash = compute_scene_hash(scene);
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

} // namespace tachyon
