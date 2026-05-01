#include "tachyon/core/spec/scene_spec_serialize.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include "tachyon/core/spec/scene_spec_audio.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace tachyon {

static std::string to_string(LayerType type) {
    switch (type) {
        case LayerType::Solid: return "solid";
        case LayerType::Shape: return "shape";
        case LayerType::Image: return "image";
        case LayerType::Video: return "video";
        case LayerType::Text: return "text";
        case LayerType::Camera: return "camera";
        case LayerType::Precomp: return "precomp";
        case LayerType::Light: return "light";
        case LayerType::Mask: return "mask";
        case LayerType::NullLayer: return "null";
        case LayerType::Procedural: return "procedural";
        case LayerType::Unknown: return "unknown";
    }
    return "unknown";
}

static std::string to_string(TransitionKind kind) {
    switch (kind) {
        case TransitionKind::None: return "none";
        case TransitionKind::Fade: return "fade";
        case TransitionKind::Slide: return "slide";
        case TransitionKind::Zoom: return "zoom";
        case TransitionKind::Flip: return "flip";
        case TransitionKind::Wipe: return "wipe";
        case TransitionKind::Dissolve: return "dissolve";
        case TransitionKind::Custom: return "custom";
    }
    return "none";
}

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

json serialize_vector3_property(const AnimatedVector3Spec& prop) {
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

json serialize_color_property(const AnimatedColorSpec& prop) {
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

static json serialize_transform3d(const Transform3D& transform) {
    json j;
    if (!transform.anchor_point_property.empty()) j["anchor_point"] = serialize_vector3_property(transform.anchor_point_property);
    if (!transform.position_property.empty()) j["position"] = serialize_vector3_property(transform.position_property);
    if (!transform.orientation_property.empty()) j["orientation"] = serialize_vector3_property(transform.orientation_property);
    if (!transform.rotation_property.empty()) j["rotation"] = serialize_vector3_property(transform.rotation_property);
    if (!transform.scale_property.empty()) j["scale"] = serialize_vector3_property(transform.scale_property);
    return j;
}

static json serialize_transition(const LayerTransitionSpec& trans) {
    if (trans.type.empty() && trans.kind == TransitionKind::None) return json{};
    json j;
    // Use string type for JSON schema
    if (!trans.type.empty()) {
        j["type"] = trans.type;
    } else if (trans.kind != TransitionKind::None) {
        j["type"] = to_string(trans.kind);
    }
    // Backward compatible: write transition_id if different from type
    if (!trans.transition_id.empty() && trans.transition_id != trans.type) {
        j["transition_id"] = trans.transition_id;
    }
    if (!trans.direction.empty()) j["direction"] = trans.direction;
    j["duration"] = trans.duration;
    if (trans.delay != 0.0) j["delay"] = trans.delay;
    if (trans.easing != animation::EasingPreset::None) {
        j["easing"] = serialize_easing_preset(trans.easing);
    }
    return j;
}

json serialize_layer(const LayerSpec& layer) {
    json j;
    j["id"] = layer.id;
    j["name"] = layer.name;
    // Write type from kind if string type is empty, otherwise use string type
    if (!layer.type.empty()) {
        j["type"] = layer.type;
    } else if (layer.kind != LayerType::NullLayer && layer.kind != LayerType::Unknown) {
        j["type"] = to_string(layer.kind);
    }
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

        json t3d = serialize_transform3d(layer.transform3d);
        if (!t3d.empty()) j["transform3d"] = t3d;
    }
    
    {
        json tin = serialize_transition(layer.transition_in);
        if (!tin.empty()) j["transition_in"] = tin;
        
        json tout = serialize_transition(layer.transition_out);
        if (!tout.empty()) j["transition_out"] = tout;
    }

    // Auto-serialize scalar and color properties using X-Macros
    #define TACHYON_PROPERTY_SCALAR(json_key, cpp_field, fallback) \
        if (!layer.cpp_field.empty()) j[#json_key] = serialize_scalar_property(layer.cpp_field);
    #define TACHYON_PROPERTY_COLOR(json_key, cpp_field, fallback) \
        if (!layer.cpp_field.empty()) j[#json_key] = serialize_color_property(layer.cpp_field);
    #include "tachyon/core/spec/schema/objects/layer_properties.def"

    // Only serialize fallback stroke width if the animated property is empty
    if (layer.stroke_width_property.empty() && layer.stroke_width != 0.0) {
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
    if (!layer.alignment.empty()) j["alignment"] = layer.alignment;
    if (!layer.font_axes.empty()) {
        json axes_json = json::object();
        for (const auto& [axis_tag, axis_spec] : layer.font_axes) {
            axes_json[axis_tag] = serialize_scalar_property(axis_spec);
        }
        j["font_axes"] = axes_json;
    }
    if (layer.extrusion_depth != 0.0) j["extrusion_depth"] = layer.extrusion_depth;
    if (layer.bevel_size != 0.0) j["bevel_size"] = layer.bevel_size;
    if (layer.hole_bevel_ratio != 0.0) j["hole_bevel_ratio"] = layer.hole_bevel_ratio;
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
        // Serialize shape morphing fields
        if (!layer.shape_path->morph_target_points.empty()) {
            sp["morph_target_points"] = json::array();
            for (const auto& pt : layer.shape_path->morph_target_points) {
                json pj;
                pj["position"] = json::array({pt.position.x, pt.position.y});
                if (pt.tangent_in.x != 0.0f || pt.tangent_in.y != 0.0f) {
                    pj["tangent_in"] = json::array({pt.tangent_in.x, pt.tangent_in.y});
                }
                if (pt.tangent_out.x != 0.0f || pt.tangent_out.y != 0.0f) {
                    pj["tangent_out"] = json::array({pt.tangent_out.x, pt.tangent_out.y});
                }
                sp["morph_target_points"].push_back(pj);
            }
        }
        if (layer.shape_path->morph_progress != 0.0) {
            sp["morph_progress"] = layer.shape_path->morph_progress;
        }
        j["shape_path"] = sp;
    }
    if (!layer.subtitle_path.empty()) j["subtitle_path"] = layer.subtitle_path;

    if (!layer.effects.empty()) {
        j["effects"] = layer.effects;
    }
    if (!layer.text_animators.empty()) {
        j["text_animators"] = layer.text_animators;
    }
    if (!layer.text_highlights.empty()) {
        j["text_highlights"] = layer.text_highlights;
    }

    // Repeater properties are now serialized via X-Macros above

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

    if (layer.procedural.has_value() && !layer.procedural->empty()) {
        j["procedural"] = *layer.procedural;
    }

    if (layer.particle_spec.has_value() && !layer.particle_spec->empty()) {
        j["particle_spec"] = *layer.particle_spec;
    }

    return j;
}

} // namespace tachyon
