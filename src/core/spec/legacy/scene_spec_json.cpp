#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include "tachyon/core/spec/scene_spec_audio.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include "tachyon/text/fonts/management/font_manifest.h"
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
        const auto& bg = object.at("background");
        if (bg.is_string() || bg.is_object()) {
            composition.background = bg.get<BackgroundSpec>();
        }
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
        if (parsed.contains("schema_version") && parsed.at("schema_version").is_string()) {
            scene.schema_version = SchemaVersion::from_string(parsed.at("schema_version").get<std::string>());
        }
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

static std::string serialize_easing_preset(animation::EasingPreset preset) {
    switch (preset) {
        case animation::EasingPreset::EaseIn: return "ease_in";
        case animation::EasingPreset::EaseOut: return "ease_out";
        case animation::EasingPreset::EaseInOut: return "ease_in_out";
        case animation::EasingPreset::Custom: return "custom";
        default: return "";
    }
}

static json serialize_bezier(const animation::CubicBezierEasing& bezier) {
    return {
        {"cx1", bezier.cx1}, {"cy1", bezier.cy1},
        {"cx2", bezier.cx2}, {"cy2", bezier.cy2}
    };
}

static json serialize_scalar_property(const AnimatedScalarSpec& prop) {
    if (prop.empty()) return json{};
    if (prop.value.has_value() && prop.keyframes.empty() && !prop.audio_band.has_value() && !prop.expression.has_value()) {
        return *prop.value;
    }
    json j;
    if (prop.value.has_value()) j["value"] = *prop.value;
    if (prop.audio_band.has_value()) {
        switch (*prop.audio_band) {
            case AudioBandType::Bass: j["audio_band"] = "bass"; break;
            case AudioBandType::Mid: j["audio_band"] = "mid"; break;
            case AudioBandType::High: j["audio_band"] = "high"; break;
            case AudioBandType::Presence: j["audio_band"] = "presence"; break;
            case AudioBandType::Rms: j["audio_band"] = "rms"; break;
        }
        j["min"] = prop.audio_min;
        j["max"] = prop.audio_max;
    }
    if (!prop.keyframes.empty()) {
        j["keyframes"] = json::array();
        for (const auto& kf : prop.keyframes) {
            json kj;
            kj["time"] = kf.time;
            kj["value"] = kf.value;
            if (kf.easing != animation::EasingPreset::None) {
                kj["easing"] = serialize_easing_preset(kf.easing);
                if (kf.easing == animation::EasingPreset::Custom) {
                    kj["bezier"] = serialize_bezier(kf.bezier);
                }
                kj["speed_in"] = kf.speed_in;
                kj["influence_in"] = kf.influence_in;
                kj["speed_out"] = kf.speed_out;
                kj["influence_out"] = kf.influence_out;
            }
            j["keyframes"].push_back(kj);
        }
    }
    if (prop.expression.has_value()) j["expression"] = *prop.expression;
    return j;
}

static json serialize_vector2_property(const AnimatedVector2Spec& prop) {
    if (prop.empty()) return json{};
    if (prop.value.has_value() && prop.keyframes.empty() && !prop.expression.has_value()) {
        return json::array({prop.value->x, prop.value->y});
    }
    json j;
    if (prop.value.has_value()) j["value"] = json::array({prop.value->x, prop.value->y});
    if (!prop.keyframes.empty()) {
        j["keyframes"] = json::array();
        for (const auto& kf : prop.keyframes) {
            json kj;
            kj["time"] = kf.time;
            kj["value"] = json::array({kf.value.x, kf.value.y});
            if (kf.tangent_in.x != 0.0f || kf.tangent_in.y != 0.0f) {
                kj["spatial_in"] = json::array({kf.tangent_in.x, kf.tangent_in.y});
            }
            if (kf.tangent_out.x != 0.0f || kf.tangent_out.y != 0.0f) {
                kj["spatial_out"] = json::array({kf.tangent_out.x, kf.tangent_out.y});
            }
            if (kf.easing != animation::EasingPreset::None) {
                kj["easing"] = serialize_easing_preset(kf.easing);
                if (kf.easing == animation::EasingPreset::Custom) {
                    kj["bezier"] = serialize_bezier(kf.bezier);
                }
                kj["speed_in"] = kf.speed_in;
                kj["influence_in"] = kf.influence_in;
                kj["speed_out"] = kf.speed_out;
                kj["influence_out"] = kf.influence_out;
            }
            j["keyframes"].push_back(kj);
        }
    }
    if (prop.expression.has_value()) j["expression"] = *prop.expression;
    return j;
}

static json serialize_vector3_property(const AnimatedVector3Spec& prop) {
    if (prop.empty()) return json{};
    if (prop.value.has_value() && prop.keyframes.empty() && !prop.expression.has_value()) {
        return json::array({prop.value->x, prop.value->y, prop.value->z});
    }
    json j;
    if (prop.value.has_value()) j["value"] = json::array({prop.value->x, prop.value->y, prop.value->z});
    if (!prop.keyframes.empty()) {
        j["keyframes"] = json::array();
        for (const auto& kf : prop.keyframes) {
            json kj;
            kj["time"] = kf.time;
            kj["value"] = json::array({kf.value.x, kf.value.y, kf.value.z});
            if (kf.tangent_in.x != 0.0f || kf.tangent_in.y != 0.0f || kf.tangent_in.z != 0.0f) {
                kj["spatial_in"] = json::array({kf.tangent_in.x, kf.tangent_in.y, kf.tangent_in.z});
            }
            if (kf.tangent_out.x != 0.0f || kf.tangent_out.y != 0.0f || kf.tangent_out.z != 0.0f) {
                kj["spatial_out"] = json::array({kf.tangent_out.x, kf.tangent_out.y, kf.tangent_out.z});
            }
            if (kf.easing != animation::EasingPreset::None) {
                kj["easing"] = serialize_easing_preset(kf.easing);
                if (kf.easing == animation::EasingPreset::Custom) {
                    kj["bezier"] = serialize_bezier(kf.bezier);
                }
                kj["speed_in"] = kf.speed_in;
                kj["influence_in"] = kf.influence_in;
                kj["speed_out"] = kf.speed_out;
                kj["influence_out"] = kf.influence_out;
            }
            j["keyframes"].push_back(kj);
        }
    }
    if (prop.expression.has_value()) j["expression"] = *prop.expression;
    return j;
}

static json serialize_color_property(const AnimatedColorSpec& prop) {
    if (prop.empty()) return json{};
    if (prop.value.has_value() && prop.keyframes.empty()) {
        return json::array({prop.value->r, prop.value->g, prop.value->b, prop.value->a});
    }
    json j;
    if (prop.value.has_value()) {
        j["value"] = json::array({prop.value->r, prop.value->g, prop.value->b, prop.value->a});
    }
    if (!prop.keyframes.empty()) {
        j["keyframes"] = json::array();
        for (const auto& kf : prop.keyframes) {
            json kj;
            kj["time"] = kf.time;
            kj["value"] = json::array({kf.value.r, kf.value.g, kf.value.b, kf.value.a});
            if (kf.easing != animation::EasingPreset::None) {
                kj["easing"] = serialize_easing_preset(kf.easing);
                if (kf.easing == animation::EasingPreset::Custom) {
                    kj["bezier"] = serialize_bezier(kf.bezier);
                }
                kj["speed_in"] = kf.speed_in;
                kj["influence_in"] = kf.influence_in;
                kj["speed_out"] = kf.speed_out;
                kj["influence_out"] = kf.influence_out;
            }
            j["keyframes"].push_back(kj);
        }
    }
    return j;
}

static json serialize_transform(const Transform2D& transform) {
    json j;
    if (transform.position_x.has_value()) j["position_x"] = *transform.position_x;
    if (transform.position_y.has_value()) j["position_y"] = *transform.position_y;
    if (transform.rotation.has_value()) j["rotation"] = *transform.rotation;
    if (transform.scale_x.has_value()) j["scale_x"] = *transform.scale_x;
    if (transform.scale_y.has_value()) j["scale_y"] = *transform.scale_y;
    if (!transform.anchor_point.empty()) j["anchor_point"] = serialize_vector2_property(transform.anchor_point);
    if (!transform.position_property.empty()) j["position"] = serialize_vector2_property(transform.position_property);
    if (!transform.rotation_property.empty()) j["rotation"] = serialize_scalar_property(transform.rotation_property);
    if (!transform.scale_property.empty()) j["scale"] = serialize_vector2_property(transform.scale_property);
    return j;
}

json serialize_layer(const LayerSpec& layer) {
    json j;
    j["id"] = layer.id;
    j["name"] = layer.name;
    j["type"] = layer.type;
    if (!layer.asset_id.empty()) j["asset_id"] = layer.asset_id;
    j["blend_mode"] = layer.blend_mode;
    j["enabled"] = layer.enabled;
    j["visible"] = layer.visible;
    j["is_3d"] = layer.is_3d;
    j["is_adjustment_layer"] = layer.is_adjustment_layer;
    j["motion_blur"] = layer.motion_blur;
    j["start_time"] = layer.start_time;
    j["in_point"] = layer.in_point;
    j["out_point"] = layer.out_point;
    j["opacity"] = layer.opacity;
    j["width"] = layer.width;
    j["height"] = layer.height;

    {
        json t = serialize_transform(layer.transform);
        if (!t.empty()) j["transform"] = t;
    }
    if (!layer.opacity_property.empty()) j["opacity"] = serialize_scalar_property(layer.opacity_property);
    if (!layer.mask_feather.empty()) j["mask_feather"] = serialize_scalar_property(layer.mask_feather);
    if (!layer.stroke_width_property.empty()) {
        j["stroke_width"] = serialize_scalar_property(layer.stroke_width_property);
    } else {
        j["stroke_width"] = layer.stroke_width;
    }

    if (layer.parent.has_value()) j["parent"] = *layer.parent;
    if (layer.track_matte_layer_id.has_value()) j["track_matte_layer_id"] = *layer.track_matte_layer_id;
    if (layer.track_matte_type != TrackMatteType::None) {
        switch (layer.track_matte_type) {
            case TrackMatteType::Alpha: j["track_matte_type"] = "alpha"; break;
            case TrackMatteType::AlphaInverted: j["track_matte_type"] = "alpha_inverted"; break;
            case TrackMatteType::Luma: j["track_matte_type"] = "luma"; break;
            case TrackMatteType::LumaInverted: j["track_matte_type"] = "luma_inverted"; break;
            default: break;
        }
    }
    if (layer.precomp_id.has_value()) j["precomp_id"] = *layer.precomp_id;

    if (!layer.text_content.empty()) j["text_content"] = layer.text_content;
    if (!layer.font_id.empty()) j["font_id"] = layer.font_id;
    if (!layer.font_size.empty()) j["font_size"] = serialize_scalar_property(layer.font_size);
    if (!layer.alignment.empty()) j["alignment"] = layer.alignment;
    if (!layer.fill_color.empty()) j["fill_color"] = serialize_color_property(layer.fill_color);
    if (!layer.stroke_color.empty()) j["stroke_color"] = serialize_color_property(layer.stroke_color);
    if (layer.shape_path.has_value() && !layer.shape_path->empty()) {
        json sp;
        sp["closed"] = layer.shape_path->closed;
        sp["points"] = json::array();
        for (const auto& pt : layer.shape_path->points) {
            json pj;
            pj["position"] = json::array({pt.position.x, pt.position.y});
            if (pt.tangent_in.x != 0.0f || pt.tangent_in.y != 0.0f) {
                pj["tangent_in"] = json::array({pt.tangent_in.x, pt.tangent_in.y});
            }
            if (pt.tangent_out.x != 0.0f || pt.tangent_out.y != 0.0f) {
                pj["tangent_out"] = json::array({pt.tangent_out.x, pt.tangent_out.y});
            }
            sp["points"].push_back(pj);
        }
        if (!layer.shape_path->subpaths.empty()) {
            sp["subpaths"] = json::array();
            for (const auto& sub : layer.shape_path->subpaths) {
                json sj;
                sj["closed"] = sub.closed;
                sj["vertices"] = json::array();
                for (const auto& pt : sub.vertices) {
                    json pj;
                    pj["position"] = json::array({pt.position.x, pt.position.y});
                    if (pt.tangent_in.x != 0.0f || pt.tangent_in.y != 0.0f) {
                        pj["tangent_in"] = json::array({pt.tangent_in.x, pt.tangent_in.y});
                    }
                    if (pt.tangent_out.x != 0.0f || pt.tangent_out.y != 0.0f) {
                        pj["tangent_out"] = json::array({pt.tangent_out.x, pt.tangent_out.y});
                    }
                    sj["vertices"].push_back(pj);
                }
                sp["subpaths"].push_back(sj);
            }
        }
        j["shape_path"] = sp;
    }
    if (!layer.subtitle_path.empty()) j["subtitle_path"] = layer.subtitle_path;

    if (!layer.effects.empty()) {
        j["effects"] = layer.effects;
    }
    if (!layer.text_animators.empty()) {
        j["text_animators"] = json::array();
        for (const auto& a : layer.text_animators) {
            j["text_animators"].push_back(a);
        }
    }
    if (!layer.text_highlights.empty()) {
        j["text_highlights"] = json::array();
        for (const auto& h : layer.text_highlights) {
            j["text_highlights"].push_back(h);
        }
    }

    if (!layer.repeater_count.empty()) j["repeater_count"] = serialize_scalar_property(layer.repeater_count);
    if (!layer.repeater_stagger_delay.empty()) j["repeater_stagger_delay"] = serialize_scalar_property(layer.repeater_stagger_delay);
    if (!layer.repeater_offset_position_x.empty()) j["repeater_offset_position_x"] = serialize_scalar_property(layer.repeater_offset_position_x);
    if (!layer.repeater_offset_position_y.empty()) j["repeater_offset_position_y"] = serialize_scalar_property(layer.repeater_offset_position_y);
    if (!layer.repeater_offset_rotation.empty()) j["repeater_offset_rotation"] = serialize_scalar_property(layer.repeater_offset_rotation);
    if (!layer.repeater_offset_scale_x.empty()) j["repeater_offset_scale_x"] = serialize_scalar_property(layer.repeater_offset_scale_x);
    if (!layer.repeater_offset_scale_y.empty()) j["repeater_offset_scale_y"] = serialize_scalar_property(layer.repeater_offset_scale_y);
    if (!layer.repeater_start_opacity.empty()) j["repeater_start_opacity"] = serialize_scalar_property(layer.repeater_start_opacity);
    if (!layer.repeater_end_opacity.empty()) j["repeater_end_opacity"] = serialize_scalar_property(layer.repeater_end_opacity);

    if (!layer.track_bindings.empty()) {
        j["track_bindings"] = json::array();
        for (const auto& binding : layer.track_bindings) {
            json b;
            b["property_path"] = binding.property_path;
            b["source_id"] = binding.source_id;
            b["source_track_name"] = binding.source_track_name;
            b["influence"] = binding.influence;
            b["enabled"] = binding.enabled;
            j["track_bindings"].push_back(b);
        }
    }

    if (layer.time_remap.enabled || !layer.time_remap.keyframes.empty()) {
        json tr;
        tr["enabled"] = layer.time_remap.enabled;
        switch (layer.time_remap.mode) {
            case spec::TimeRemapMode::Hold: tr["mode"] = "hold"; break;
            case spec::TimeRemapMode::Blend: tr["mode"] = "blend"; break;
            case spec::TimeRemapMode::OpticalFlow: tr["mode"] = "optical_flow"; break;
        }
        if (!layer.time_remap.keyframes.empty()) {
            tr["keyframes"] = json::array();
            for (const auto& kf : layer.time_remap.keyframes) {
                tr["keyframes"].push_back(json::array({kf.first, kf.second}));
            }
        }
        j["time_remap"] = tr;
    }

    if (layer.frame_blend != spec::FrameBlendMode::Linear) {
        switch (layer.frame_blend) {
            case spec::FrameBlendMode::None: j["frame_blend"] = "none"; break;
            case spec::FrameBlendMode::PixelMotion: j["frame_blend"] = "pixel_motion"; break;
            case spec::FrameBlendMode::OpticalFlow: j["frame_blend"] = "optical_flow"; break;
            default: break;
        }
    }

    if (layer.camera_type != "one_node") j["camera_type"] = layer.camera_type;
    if (!layer.camera_zoom.empty()) j["camera_zoom"] = serialize_scalar_property(layer.camera_zoom);
    if (!layer.camera_poi.empty()) j["camera_poi"] = serialize_vector3_property(layer.camera_poi);

    if (layer.camera_shake_seed != 0 || !layer.camera_shake_amplitude_pos.empty() || !layer.camera_shake_amplitude_rot.empty() || !layer.camera_shake_frequency.empty() || !layer.camera_shake_roughness.empty()) {
        json cs;
        cs["seed"] = layer.camera_shake_seed;
        if (!layer.camera_shake_amplitude_pos.empty()) cs["amplitude_pos"] = serialize_scalar_property(layer.camera_shake_amplitude_pos);
        if (!layer.camera_shake_amplitude_rot.empty()) cs["amplitude_rot"] = serialize_scalar_property(layer.camera_shake_amplitude_rot);
        if (!layer.camera_shake_frequency.empty()) cs["frequency"] = serialize_scalar_property(layer.camera_shake_frequency);
        if (!layer.camera_shake_roughness.empty()) cs["roughness"] = serialize_scalar_property(layer.camera_shake_roughness);
        j["camera_shake"] = cs;
    }

    return j;
}

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
    j["schema_version"] = scene.schema_version.to_string();
    j["project"] = {
        {"id", scene.project.id},
        {"name", scene.project.name},
        {"authoring_tool", scene.project.authoring_tool}
    };
    if (scene.project.root_seed.has_value()) {
        j["project"]["root_seed"] = *scene.project.root_seed;
    }

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
