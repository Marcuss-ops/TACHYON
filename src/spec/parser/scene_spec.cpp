#include "tachyon/spec/scene_spec.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <set>
#include <sstream>

namespace tachyon {
namespace {

using json = nlohmann::json;

bool is_version_like(const std::string& version) {
    if (version.empty()) {
        return false;
    }

    bool saw_digit = false;
    bool saw_dot = false;
    bool last_was_dot = false;
    for (const char ch : version) {
        if (ch >= '0' && ch <= '9') {
            saw_digit = true;
            last_was_dot = false;
            continue;
        }
        if (ch == '.') {
            if (!saw_digit || last_was_dot) {
                return false;
            }
            saw_dot = true;
            last_was_dot = true;
            continue;
        }
        return false;
    }

    return saw_digit && !last_was_dot && saw_dot;
}

template <typename T>
bool read_number(const json& object, const char* key, T& out) {
    if (!object.contains(key) || object.at(key).is_null()) {
        return false;
    }
    const auto& value = object.at(key);
    if (!value.is_number()) {
        return false;
    }
    out = value.get<T>();
    return true;
}

bool read_string(const json& object, const char* key, std::string& out) {
    if (!object.contains(key) || object.at(key).is_null()) {
        return false;
    }
    const auto& value = object.at(key);
    if (!value.is_string()) {
        return false;
    }
    out = value.get<std::string>();
    return true;
}

bool read_bool(const json& object, const char* key, bool& out) {
    if (!object.contains(key) || object.at(key).is_null()) {
        return false;
    }
    const auto& value = object.at(key);
    if (!value.is_boolean()) {
        return false;
    }
    out = value.get<bool>();
    return true;
}

bool read_optional_int(const json& object, const char* key, std::optional<std::int64_t>& out) {
    if (!object.contains(key) || object.at(key).is_null()) {
        return false;
    }
    const auto& value = object.at(key);
    if (!value.is_number_integer()) {
        return false;
    }
    out = value.get<std::int64_t>();
    return true;
}

std::string make_path(const std::string& parent, const std::string& child) {
    if (parent.empty()) {
        return child;
    }
    if (child.empty()) {
        return parent;
    }
    return parent + "." + child;
}

bool read_vector2_like(const json& value, math::Vector2& out) {
    if (value.is_array()) {
        if (value.size() < 2 || !value[0].is_number() || !value[1].is_number()) {
            return false;
        }
        out = {
            static_cast<float>(value[0].get<double>()),
            static_cast<float>(value[1].get<double>())
        };
        return true;
    }

    if (value.is_number()) {
        const float scalar = static_cast<float>(value.get<double>());
        out = {scalar, scalar};
        return true;
    }

    return false;
}

bool read_scalar_like(const json& value, double& out) {
    if (value.is_number()) {
        out = value.get<double>();
        return true;
    }

    if (value.is_array() && !value.empty() && value[0].is_number()) {
        out = value[0].get<double>();
        return true;
    }

    return false;
}

bool read_vector2_object(const json& value, math::Vector2& out) {
    if (!value.is_object()) {
        return false;
    }

    if (!value.contains("x") || !value.contains("y") || !value.at("x").is_number() || !value.at("y").is_number()) {
        return false;
    }

    out = {
        static_cast<float>(value.at("x").get<double>()),
        static_cast<float>(value.at("y").get<double>())
    };
    return true;
}

bool parse_vector2_value(const json& value, math::Vector2& out) {
    return read_vector2_like(value, out) || read_vector2_object(value, out);
}

animation::EasingPreset parse_easing_preset(const json& value) {
    if (!value.is_string()) {
        return animation::EasingPreset::None;
    }

    const std::string easing = value.get<std::string>();
    if (easing == "ease_in" || easing == "easeIn") {
        return animation::EasingPreset::EaseIn;
    }
    if (easing == "ease_out" || easing == "easeOut") {
        return animation::EasingPreset::EaseOut;
    }
    if (easing == "ease_in_out" || easing == "easeInOut") {
        return animation::EasingPreset::EaseInOut;
    }
    if (easing == "custom" || easing == "bezier") {
        return animation::EasingPreset::Custom;
    }
    return animation::EasingPreset::None;
}

animation::CubicBezierEasing parse_bezier(const json& value) {
    animation::CubicBezierEasing bezier = animation::CubicBezierEasing::linear();
    if (!value.is_object()) {
        return bezier;
    }

    if (value.contains("cx1") && value.at("cx1").is_number()) {
        bezier.cx1 = value.at("cx1").get<double>();
    }
    if (value.contains("cy1") && value.at("cy1").is_number()) {
        bezier.cy1 = value.at("cy1").get<double>();
    }
    if (value.contains("cx2") && value.at("cx2").is_number()) {
        bezier.cx2 = value.at("cx2").get<double>();
    }
    if (value.contains("cy2") && value.at("cy2").is_number()) {
        bezier.cy2 = value.at("cy2").get<double>();
    }
    return bezier;
}

template <typename KeyframeT>
bool parse_keyframe_common(const json& object, KeyframeT& keyframe, const std::string& path, DiagnosticBag& diagnostics) {
    if (!object.contains("time") || !object.at("time").is_number()) {
        diagnostics.add_error("scene.layer.keyframe.time_invalid", "keyframe.time must be numeric", path + ".time");
        return false;
    }

    keyframe.time = object.at("time").get<double>();
    if (object.contains("easing")) {
        keyframe.easing = parse_easing_preset(object.at("easing"));
        if (keyframe.easing == animation::EasingPreset::Custom && object.contains("bezier")) {
            keyframe.bezier = parse_bezier(object.at("bezier"));
        }
    }
    return true;
}

bool parse_scalar_keyframe(const json& object, ScalarKeyframeSpec& keyframe, const std::string& path, DiagnosticBag& diagnostics) {
    if (!parse_keyframe_common(object, keyframe, path, diagnostics)) {
        return false;
    }
    if (!object.contains("value") || !object.at("value").is_number()) {
        diagnostics.add_error("scene.layer.keyframe.value_invalid", "scalar keyframe value must be numeric", path + ".value");
        return false;
    }
    keyframe.value = object.at("value").get<double>();
    return true;
}

bool parse_vector2_keyframe(const json& object, Vector2KeyframeSpec& keyframe, const std::string& path, DiagnosticBag& diagnostics) {
    if (!parse_keyframe_common(object, keyframe, path, diagnostics)) {
        return false;
    }
    if (!object.contains("value") || !parse_vector2_value(object.at("value"), keyframe.value)) {
        diagnostics.add_error("scene.layer.keyframe.value_invalid", "vector2 keyframe value must be array or object", path + ".value");
        return false;
    }
    return true;
}

void parse_optional_scalar_property(const json& object, const char* key, AnimatedScalarSpec& property, const std::string& path, DiagnosticBag& diagnostics) {
    if (!object.contains(key) || object.at(key).is_null()) {
        return;
    }

    const auto& value = object.at(key);
    if (value.is_number()) {
        property.value = value.get<double>();
        return;
    }

    if (value.is_object()) {
        if (value.contains("value") && value.at("value").is_number()) {
            property.value = value.at("value").get<double>();
        }

        if (value.contains("keyframes") && value.at("keyframes").is_array()) {
            const auto& keyframes = value.at("keyframes");
            for (std::size_t index = 0; index < keyframes.size(); ++index) {
                if (!keyframes[index].is_object()) {
                    diagnostics.add_error("scene.layer.keyframe.invalid", "scalar keyframe entries must be objects", make_path(path, key) + ".keyframes[" + std::to_string(index) + "]");
                    continue;
                }
                ScalarKeyframeSpec keyframe;
                if (parse_scalar_keyframe(keyframes[index], keyframe, make_path(path, key) + ".keyframes[" + std::to_string(index) + "]", diagnostics)) {
                    property.keyframes.push_back(keyframe);
                }
            }
            return;
        }

        if (property.value.has_value()) {
            return;
        }

        diagnostics.add_error("scene.layer.property_invalid", std::string(key) + " must contain a numeric value or keyframes when provided as an object", make_path(path, key));
        return;
    }

    diagnostics.add_error("scene.layer.property_invalid", std::string(key) + " must be a number or object", make_path(path, key));
}

void parse_optional_vector_property(const json& object, const char* key, AnimatedVector2Spec& property, const std::string& path, DiagnosticBag& diagnostics) {
    if (!object.contains(key) || object.at(key).is_null()) {
        return;
    }

    const auto& value = object.at(key);
    math::Vector2 vector_value;
    if (parse_vector2_value(value, vector_value)) {
        property.value = vector_value;
        return;
    }

    if (value.is_object()) {
        if (value.contains("value") && parse_vector2_value(value.at("value"), vector_value)) {
            property.value = vector_value;
            return;
        }

        if (value.contains("keyframes") && value.at("keyframes").is_array()) {
            const auto& keyframes = value.at("keyframes");
            for (std::size_t index = 0; index < keyframes.size(); ++index) {
                if (!keyframes[index].is_object()) {
                    diagnostics.add_error("scene.layer.keyframe.invalid", "vector keyframe entries must be objects", make_path(path, key) + ".keyframes[" + std::to_string(index) + "]");
                    continue;
                }
                Vector2KeyframeSpec keyframe;
                if (parse_vector2_keyframe(keyframes[index], keyframe, make_path(path, key) + ".keyframes[" + std::to_string(index) + "]", diagnostics)) {
                    property.keyframes.push_back(keyframe);
                }
            }
            return;
        }

        if (property.value.has_value()) {
            return;
        }

        diagnostics.add_error("scene.layer.property_invalid", std::string(key) + " must contain a vector value or keyframes when provided as an object", make_path(path, key));
        return;
    }

    diagnostics.add_error("scene.layer.property_invalid", std::string(key) + " must be a vector or object", make_path(path, key));
}

void parse_transform(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics) {
    if (object.contains("position") && !object.at("position").is_null()) {
        math::Vector2 position;
        if (!read_vector2_like(object.at("position"), position)) {
            diagnostics.add_error("scene.transform.position_invalid", "position must be a number or array with at least two numbers", make_path(path, "transform.position"));
        } else {
            layer.transform.position_x = static_cast<double>(position.x);
            layer.transform.position_y = static_cast<double>(position.y);
        }
    }

    if (object.contains("rotation") && !object.at("rotation").is_null()) {
        double rotation = 0.0;
        if (read_scalar_like(object.at("rotation"), rotation)) {
            layer.transform.rotation = rotation;
        } else {
            diagnostics.add_error("scene.transform.rotation_invalid", "rotation must be a number or array with at least one number", make_path(path, "transform.rotation"));
        }
    }

    if (object.contains("scale") && !object.at("scale").is_null()) {
        if (object.at("scale").is_array()) {
            const auto& scale = object.at("scale");
            if (scale.size() < 2 || !scale[0].is_number() || !scale[1].is_number()) {
                diagnostics.add_error("scene.transform.scale_invalid", "scale must be a number or array with at least two numbers", make_path(path, "transform.scale"));
            } else {
                layer.transform.scale_x = scale[0].get<double>();
                layer.transform.scale_y = scale[1].get<double>();
            }
        } else if (object.at("scale").is_number()) {
            const double scale = object.at("scale").get<double>();
            layer.transform.scale_x = scale;
            layer.transform.scale_y = scale;
        } else {
            diagnostics.add_error("scene.transform.scale_invalid", "scale must be a number or array with at least two numbers", make_path(path, "transform.scale"));
        }
    }

    parse_optional_vector_property(object, "position_property", layer.transform.position_property, path, diagnostics);
    parse_optional_scalar_property(object, "rotation_property", layer.transform.rotation_property, path, diagnostics);
    parse_optional_vector_property(object, "scale_property", layer.transform.scale_property, path, diagnostics);
}

void parse_shape_path(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics) {
    if (!object.contains("shape_path") || object.at("shape_path").is_null()) {
        return;
    }

    const auto& shape_path = object.at("shape_path");
    if (!shape_path.is_object()) {
        diagnostics.add_error("scene.layer.shape_path_invalid", "shape_path must be an object", make_path(path, "shape_path"));
        return;
    }

    if (!shape_path.contains("points") || !shape_path.at("points").is_array()) {
        diagnostics.add_error("scene.layer.shape_path_invalid", "shape_path.points must be an array", make_path(path, "shape_path.points"));
        return;
    }

    ShapePathSpec parsed;
    if (shape_path.contains("closed") && shape_path.at("closed").is_boolean()) {
        parsed.closed = shape_path.at("closed").get<bool>();
    }

    const auto& points = shape_path.at("points");
    for (std::size_t index = 0; index < points.size(); ++index) {
        ShapePathPointSpec point;
        if (!parse_vector2_value(points[index], point.position)) {
            diagnostics.add_error("scene.layer.shape_path_point_invalid", "shape_path points must be vector2-like", make_path(path, "shape_path.points[" + std::to_string(index) + "]"));
            continue;
        }
        parsed.points.push_back(point);
    }

    if (!parsed.points.empty()) {
        layer.shape_path = std::move(parsed);
    }
}

LayerSpec parse_layer(const json& object, const std::string& path, DiagnosticBag& diagnostics) {
    LayerSpec layer;
    read_string(object, "id", layer.id);
    read_string(object, "type", layer.type);
    read_string(object, "name", layer.name);
    read_bool(object, "enabled", layer.enabled);
    read_number(object, "start_time", layer.start_time);
    read_number(object, "in_point", layer.in_point);
    read_number(object, "out_point", layer.out_point);
    read_number(object, "opacity", layer.opacity);

    if (object.contains("parent") && !object.at("parent").is_null()) {
        if (object.at("parent").is_string()) {
            layer.parent = object.at("parent").get<std::string>();
        } else {
            diagnostics.add_error("scene.layer.parent_invalid", "parent must be a string when provided", make_path(path, "parent"));
        }
    }

    if (object.contains("transform") && object.at("transform").is_object()) {
        parse_transform(object.at("transform"), layer, path, diagnostics);
    }

    parse_optional_scalar_property(object, "opacity_property", layer.opacity_property, path, diagnostics);
    parse_shape_path(object, layer, path, diagnostics);

    parse_optional_scalar_property(object, "time_remap", layer.time_remap_property, path, diagnostics);

    return layer;
}

bool is_asset_alpha_mode_valid(const std::string& mode) {
    return mode == "premultiplied" || mode == "straight" || mode == "opaque";
}

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
        const auto& frame_rate = object.at("frame_rate");
        if (frame_rate.is_number_integer()) {
            composition.frame_rate.numerator = frame_rate.get<std::int64_t>();
            composition.frame_rate.denominator = 1;
        } else if (frame_rate.is_object()) {
            read_number(frame_rate, "numerator", composition.frame_rate.numerator);
            read_number(frame_rate, "denominator", composition.frame_rate.denominator);
        } else {
            diagnostics.add_error("scene.composition.frame_rate_invalid", "frame_rate must be an integer or object", make_path(path, "frame_rate"));
        }
    }

    if (object.contains("background") && object.at("background").is_string()) {
        composition.background = object.at("background").get<std::string>();
    }

    if (object.contains("layers")) {
        const auto& layers = object.at("layers");
        if (!layers.is_array()) {
            diagnostics.add_error("scene.composition.layers_invalid", "layers must be an array", make_path(path, "layers"));
        } else {
            for (std::size_t index = 0; index < layers.size(); ++index) {
                if (!layers[index].is_object()) {
                    diagnostics.add_error("scene.layer.invalid", "layer entries must be objects", make_path(path, "layers[" + std::to_string(index) + "]"));
                    continue;
                }
                composition.layers.push_back(parse_layer(layers[index], make_path(path, "layers[" + std::to_string(index) + "]"), diagnostics));
            }
        }
    }

    return composition;
}

AssetSpec parse_asset(const json& object, const std::string& path, DiagnosticBag& diagnostics) {
    AssetSpec asset;
    read_string(object, "id", asset.id);
    read_string(object, "type", asset.type);
    read_string(object, "source", asset.source);
    read_string(object, "path", asset.path);

    if (object.contains("alpha_mode") && object.at("alpha_mode").is_string()) {
        asset.alpha_mode = object.at("alpha_mode").get<std::string>();
    }

    if (asset.id.empty()) {
        diagnostics.add_error("scene.asset.id_missing", "asset id is required", make_path(path, "id"));
    }
    if (asset.source.empty() && !asset.path.empty()) {
        asset.source = asset.path;
    }
    if (asset.alpha_mode.has_value() && !is_asset_alpha_mode_valid(*asset.alpha_mode)) {
        diagnostics.add_error("scene.asset.alpha_mode_invalid", "alpha_mode must be premultiplied, straight, or opaque", make_path(path, "alpha_mode"));
    }
    return asset;
}

} // namespace

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
    if (scene.spec_version.empty() && !scene.version.empty()) {
        scene.spec_version = scene.version;
    }

    if (root.contains("project") && root.at("project").is_object()) {
        const auto& project = root.at("project");
        read_string(project, "id", scene.project.id);
        read_string(project, "name", scene.project.name);
        read_string(project, "authoring_tool", scene.project.authoring_tool);
        read_optional_int(project, "root_seed", scene.project.root_seed);
    } else {
        result.diagnostics.add_error("scene.project.missing", "project object is required", "project");
    }

    if (root.contains("assets")) {
        const auto& assets = root.at("assets");
        if (!assets.is_array()) {
            result.diagnostics.add_error("scene.assets.invalid", "assets must be an array", "assets");
        } else {
            for (std::size_t index = 0; index < assets.size(); ++index) {
                if (!assets[index].is_object()) {
                    result.diagnostics.add_error("scene.asset.invalid", "asset entries must be objects", "assets[" + std::to_string(index) + "]");
                    continue;
                }
                scene.assets.push_back(parse_asset(assets[index], "assets[" + std::to_string(index) + "]", result.diagnostics));
            }
        }
    }

    if (root.contains("compositions")) {
        const auto& compositions = root.at("compositions");
        if (!compositions.is_array()) {
            result.diagnostics.add_error("scene.compositions.invalid", "compositions must be an array", "compositions");
        } else {
            for (std::size_t index = 0; index < compositions.size(); ++index) {
                if (!compositions[index].is_object()) {
                    result.diagnostics.add_error("scene.composition.invalid", "composition entries must be objects", "compositions[" + std::to_string(index) + "]");
                    continue;
                }
                scene.compositions.push_back(parse_composition(compositions[index], "compositions[" + std::to_string(index) + "]", result.diagnostics));
            }
        }
    }

    if (root.contains("render_defaults") && root.at("render_defaults").is_object()) {
        const auto& render_defaults = root.at("render_defaults");
        if (render_defaults.contains("output") && render_defaults.at("output").is_string()) {
            scene.render_defaults.output = render_defaults.at("output").get<std::string>();
        }
    }

    if (result.diagnostics.ok()) {
        result.value = std::move(scene);
    }
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

    if (scene.spec_version.empty()) {
        result.diagnostics.add_error("scene.version_missing", "version/spec_version is required", "version");
    } else if (!is_version_like(scene.spec_version)) {
        result.diagnostics.add_error("scene.spec_version.invalid", "spec_version must be parseable and version-like", "spec_version");
    }

    if (scene.project.id.empty()) {
        result.diagnostics.add_error("scene.project.id_missing", "project.id is required", "project.id");
    }

    if (scene.project.name.empty()) {
        result.diagnostics.add_error("scene.project.name_missing", "project.name is required", "project.name");
    }

    if (scene.project.authoring_tool.empty()) {
        result.diagnostics.add_warning("scene.project.authoring_tool_missing", "project.authoring_tool should be set for locked contract scenes", "project.authoring_tool");
    }

    if (scene.project.root_seed.has_value() && *scene.project.root_seed < 0) {
        result.diagnostics.add_error("scene.project.root_seed_invalid", "project.root_seed must be non-negative", "project.root_seed");
    }

    if (scene.compositions.empty()) {
        result.diagnostics.add_error("scene.compositions.missing", "at least one composition is required", "compositions");
    }

    std::set<std::string> composition_ids;
    for (std::size_t composition_index = 0; composition_index < scene.compositions.size(); ++composition_index) {
        const auto& composition = scene.compositions[composition_index];
        const std::string composition_path = "compositions[" + std::to_string(composition_index) + "]";

        if (composition.id.empty()) {
            result.diagnostics.add_error("scene.composition.id_missing", "composition.id is required", composition_path + ".id");
        } else if (!composition_ids.insert(composition.id).second) {
            result.diagnostics.add_error("scene.composition.id_duplicate", "composition.id must be unique", composition_path + ".id");
        }

        if (composition.width <= 0 || composition.height <= 0) {
            result.diagnostics.add_error("scene.composition.size_invalid", "composition width and height must be positive", composition_path + ".width");
        }

        if (composition.duration <= 0.0) {
            result.diagnostics.add_error("scene.composition.duration_invalid", "composition duration must be positive", composition_path + ".duration");
        }

        if (composition.fps.has_value() && *composition.fps <= 0) {
            result.diagnostics.add_error("scene.composition.fps_invalid", "fps must be positive", composition_path + ".fps");
        }

        if (composition.frame_rate.numerator <= 0 || composition.frame_rate.denominator <= 0) {
            result.diagnostics.add_error("scene.composition.frame_rate_invalid", "frame_rate must be positive", composition_path + ".frame_rate");
        }

        std::set<std::string> layer_ids;
        for (std::size_t layer_index = 0; layer_index < composition.layers.size(); ++layer_index) {
            const auto& layer = composition.layers[layer_index];
            const std::string layer_path = composition_path + ".layers[" + std::to_string(layer_index) + "]";

            if (layer.id.empty()) {
                result.diagnostics.add_error("scene.layer.id_missing", "layer.id is required", layer_path + ".id");
            } else if (!layer_ids.insert(layer.id).second) {
                result.diagnostics.add_error("scene.layer.id_duplicate", "layer.id must be unique within a composition", layer_path + ".id");
            }

            if (layer.type.empty()) {
                result.diagnostics.add_error("scene.layer.type_missing", "layer.type is required", layer_path + ".type");
            } else {
                static const std::set<std::string> allowed_types = {
                    "solid",
                    "image",
                    "text",
                    "shape",
                    "mask",
                    "precomp",
                    "null",
                    "camera"
                };
                if (!allowed_types.contains(layer.type)) {
                    result.diagnostics.add_error("scene.layer.type_unsupported", "unsupported layer type: " + layer.type, layer_path + ".type");
                }
            }

            if (layer.in_point > layer.out_point) {
                result.diagnostics.add_error("scene.layer.time_invalid", "layer.in_point must be less than or equal to layer.out_point", layer_path + ".in_point");
            }

            if (layer.opacity < 0.0 || layer.opacity > 1.0) {
                result.diagnostics.add_error("scene.layer.opacity_invalid", "layer.opacity must be between 0 and 1", layer_path + ".opacity");
            }

            if (layer.parent.has_value() && !layer.parent->empty() && !layer_ids.contains(*layer.parent)) {
                result.diagnostics.add_error("scene.layer.parent_missing", "layer.parent must reference an earlier layer id in the same composition", layer_path + ".parent");
            }

            if (layer.type == "mask" && layer.opacity <= 0.0) {
                result.diagnostics.add_warning("scene.layer.mask_transparent", "mask layers with zero opacity do not affect clipping", layer_path + ".opacity");
            }

            if (layer.type == "shape" && (!layer.shape_path.has_value() || layer.shape_path->points.empty())) {
                result.diagnostics.add_warning("scene.layer.shape_path_missing", "shape layers should define shape_path for real geometry", layer_path + ".shape_path");
            }
        }
    }

    return result;
}

} // namespace tachyon
