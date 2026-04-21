#include "tachyon/core/scene/evaluator_layer.h"
#include "tachyon/renderer2d/animation/easing.h"
#include "tachyon/core/math/quaternion.h"
#include "tachyon/core/math/transform2.h"
#include "tachyon/timeline/time.h"
#include "tachyon/renderer2d/expressions/expression_evaluator.h"
#include "tachyon/renderer2d/math/math_utils.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/media/mesh_asset.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>

namespace tachyon {
namespace scene {


LayerType map_layer_type(const std::string& type) {
    if (type == "solid") return LayerType::Solid;
    if (type == "shape") return LayerType::Shape;
    if (type == "mask") return LayerType::Mask;
    if (type == "image") return LayerType::Image;
    if (type == "video") return LayerType::Video;
    if (type == "text") return LayerType::Text;
    if (type == "camera") return LayerType::Camera;
    if (type == "precomp") return LayerType::Precomp;
    if (type == "light") return LayerType::Light;
    if (type == "null") return LayerType::NullLayer;
    return LayerType::Unknown;
}



void evaluate_mesh_animations(EvaluatedLayerState& evaluated, double time) {
    if (!evaluated.mesh_asset) return;
    auto& asset = *evaluated.mesh_asset;
    
    // We'll use a temporary TRS cache for joints
    struct TRS {
        math::Vector3 translation{0,0,0};
        math::Quaternion rotation = math::Quaternion::identity(); // quaternion
        math::Vector3 scale{1,1,1};
        bool has_t{false}, has_r{false}, has_s{false};
    };
    std::vector<TRS> joint_trs;
    if (!asset.skins.empty()) {
        joint_trs.resize(asset.skins[0].joints.size());
    }

    // 1. Evaluate Animations to update joint local transforms and morph weights
    for (const auto& anim : asset.animations) {
        for (const auto& chan : anim.channels) {
            if (chan.times.empty()) continue;
            
            // Find keyframe
            size_t idx = 0;
            if (time <= chan.times.front()) idx = 0;
            else if (time >= chan.times.back()) idx = chan.times.size() - 2;
            else {
                for (size_t i = 0; i < chan.times.size() - 1; ++i) {
                    if (time < chan.times[i+1]) { idx = i; break; }
                }
            }
            
            float t = (static_cast<float>(time) - chan.times[idx]) / (chan.times[idx+1] - chan.times[idx]);
            t = std::clamp(t, 0.0f, 1.0f);
            
            if (chan.path == media::MeshAsset::AnimationChannel::Path::Weights) {
                size_t num_morphs = chan.values.size() / chan.times.size();
                evaluated.morph_weights.resize(num_morphs, 0.0f);
                for (size_t m = 0; m < num_morphs; ++m) {
                    float v0 = chan.values[idx * num_morphs + m];
                    float v1 = chan.values[(idx+1) * num_morphs + m];
                    evaluated.morph_weights[m] = v0 * (1.0f - t) + v1 * t;
                }
            } else if (!asset.skins.empty()) {
                // Find which joint this node corresponds to
                const auto& skin = asset.skins[0];
                int joint_idx = -1;
                for (int j = 0; j < static_cast<int>(skin.joints.size()); ++j) {
                    if (skin.joints[j].index == chan.node_idx) { joint_idx = j; break; }
                }
                
                if (joint_idx >= 0) {
                    TRS& trs = joint_trs[joint_idx];
                    if (chan.path == media::MeshAsset::AnimationChannel::Path::Translation) {
                        math::Vector3 v0{chan.values[idx*3+0], chan.values[idx*3+1], chan.values[idx*3+2]};
                        math::Vector3 v1{chan.values[(idx+1)*3+0], chan.values[(idx+1)*3+1], chan.values[(idx+1)*3+2]};
                        trs.translation = v0 * (1.0f - t) + v1 * t;
                        trs.has_t = true;
                    } else if (chan.path == media::MeshAsset::AnimationChannel::Path::Scale) {
                        math::Vector3 v0{chan.values[idx*3+0], chan.values[idx*3+1], chan.values[idx*3+2]};
                        math::Vector3 v1{chan.values[(idx+1)*3+0], chan.values[(idx+1)*3+1], chan.values[(idx+1)*3+2]};
                        trs.scale = v0 * (1.0f - t) + v1 * t;
                        trs.has_s = true;
                    } else if (chan.path == media::MeshAsset::AnimationChannel::Path::Rotation) {
                        // Rotation is a quaternion (X, Y, Z, W)
                        // For "fix veloce" we'll do Nlerp, but ideally Slerp
                        math::Quaternion q0{chan.values[idx*4+0], chan.values[idx*4+1], chan.values[idx*4+2], chan.values[idx*4+3]};
                        math::Quaternion q1{chan.values[(idx+1)*4+0], chan.values[(idx+1)*4+1], chan.values[(idx+1)*4+2], chan.values[(idx+1)*4+3]};
                        if (math::Quaternion::dot(q0, q1) < 0) q1 = q1 * -1.0f;
                        trs.rotation = (q0 * (1.0f - t) + q1 * t).normalized();
                        trs.has_r = true;
                    }
                }
            }
        }
    }
    
    // 2. Compute Joint Matrices
    if (!asset.skins.empty()) {
        const auto& skin = asset.skins[0];
        evaluated.joint_matrices.resize(skin.joints.size());
        std::vector<math::Matrix4x4> global_transforms(skin.joints.size());
        
        for (size_t i = 0; i < skin.joints.size(); ++i) {
            const auto& joint = skin.joints[i];
            const TRS& trs = joint_trs[i];
            
            // Build local transform from animated TRS or original local_transform
            math::Matrix4x4 local;
            if (trs.has_t || trs.has_r || trs.has_s) {
                local = math::compose_trs(trs.translation, trs.rotation, trs.scale);
            } else {
                local = joint.local_transform;
            }
            
            if (joint.parent >= 0) {
                // Determine joint index of parent
                int parent_joint_idx = -1;
                for (int pj = 0; pj < static_cast<int>(skin.joints.size()); ++pj) {
                    if (skin.joints[pj].index == joint.parent) { parent_joint_idx = pj; break; }
                }
                
                if (parent_joint_idx >= 0) {
                    global_transforms[i] = global_transforms[parent_joint_idx] * local;
                } else {
                    global_transforms[i] = local;
                }
            } else {
                global_transforms[i] = local;
            }
            
            evaluated.joint_matrices[i] = global_transforms[i] * joint.inverse_bind_matrix;
        }
    }
}

EvaluatedLayerState make_layer_state(
    EvaluationContext& context,
    const LayerSpec& layer,
    std::size_t layer_index,
    double time_offset) {
    
    // Unpack context for convenience
    const CompositionSpec& composition = context.composition;
    const SceneSpec* scene = context.scene;
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer = context.audio_analyzer;
    const EvaluationVariables& vars = context.vars;
    const double composition_time_seconds = context.composition_time_seconds;
    
    EvaluatedLayerState evaluated;
    evaluated.layer_index = layer_index;
    evaluated.id = layer.id;
    evaluated.type = map_layer_type(layer.type);
    evaluated.name = layer.name;
    evaluated.blend_mode = layer.blend_mode;
    evaluated.enabled = layer.enabled;
    evaluated.visible = layer.visible;
    evaluated.is_3d = layer.is_3d;
    evaluated.is_adjustment_layer = layer.is_adjustment_layer;
    
    // Implementation of Stagger (time offset)
    evaluated.local_time_seconds = timeline::local_time_from_composition(composition_time_seconds, layer.start_time) + time_offset;
    
    const std::uint64_t layer_seed = make_property_expression_seed(scene, composition, layer, "layer");
    const double remapped_time = sample_scalar(
        layer.time_remap_property,
        evaluated.local_time_seconds,
        evaluated.local_time_seconds,
        audio_analyzer,
        hash_combine(layer_seed, stable_string_hash("time_remap")),
        vars.numeric,
        vars.tables,
        static_cast<std::uint32_t>(layer_index));
    evaluated.child_time_seconds = remapped_time;
    evaluated.active = layer.enabled && composition_time_seconds >= layer.in_point && composition_time_seconds <= layer.out_point;

    auto make_loop_sampler = [&](const AnimatedScalarSpec& prop, double fallback, std::uint64_t seed) {
        return [&, prop, fallback, seed](int prop_idx, double t) -> double {
            if (prop_idx == -1) {
                return sample_scalar(prop, fallback, t, audio_analyzer, seed, vars.numeric, vars.tables, static_cast<std::uint32_t>(layer_index), nullptr, true);
            }
            return 0.0;
        };
    };

    const std::uint64_t opacity_seed = hash_combine(layer_seed, stable_string_hash("opacity"));
    evaluated.opacity = sample_scalar(
        layer.opacity_property,
        layer.opacity,
        remapped_time,
        audio_analyzer,
        opacity_seed,
        vars.numeric,
        vars.tables,
        static_cast<std::uint32_t>(layer_index),
        make_loop_sampler(layer.opacity_property, layer.opacity, opacity_seed));

    math::Vector2 pos = sample_vector2(
        layer.transform.position_property,
        fallback_position(layer),
        remapped_time,
        audio_analyzer,
        hash_combine(layer_seed, stable_string_hash("position")),
        vars.numeric,
        vars.tables);
        
    // Motion Path Support
    if (layer.motion_path_id.has_value() && !layer.motion_path_id->empty()) {
        const auto it = context.layer_indices.find(*layer.motion_path_id);
        if (it != context.layer_indices.end()) {
            const auto& path_layer = context.composition.layers[it->second];
            if (path_layer.shape_path.has_value() && !path_layer.shape_path->points.empty()) {
                const double progress = std::clamp(sample_scalar(
                    layer.motion_path_progress, 0.0, remapped_time, audio_analyzer,
                    hash_combine(layer_seed, stable_string_hash("path_progress")),
                    vars.numeric, vars.tables
                ) / 100.0, 0.0, 1.0);
                
                const auto& pts = path_layer.shape_path->points;
                if (pts.size() >= 2) {
                    const double segment_count = static_cast<double>(pts.size() - 1);
                    const double scaled_p = progress * segment_count;
                    const size_t idx = std::min(static_cast<size_t>(std::floor(scaled_p)), pts.size() - 2);
                    const float t = static_cast<float>(scaled_p - std::floor(scaled_p));
                    
                    if (pts[idx].tangent_out.length_squared() > 1e-6f || pts[idx+1].tangent_in.length_squared() > 1e-6f) {
                        pos = renderer2d::math_utils::sample_bezier_spatial(
                            pts[idx].position,
                            pts[idx].position + pts[idx].tangent_out,
                            pts[idx+1].position + pts[idx+1].tangent_in,
                            pts[idx+1].position,
                            t
                        );
                    } else {
                        pos = pts[idx].position * (1.0f - t) + pts[idx+1].position * t;
                    }
                } else if (!pts.empty()) {
                    pos = pts[0].position;
                }
            }
        }
    }

    const std::uint64_t rot_seed = hash_combine(layer_seed, stable_string_hash("rotation"));
    double rot = sample_scalar(
        layer.transform.rotation_property,
        fallback_rotation(layer),
        remapped_time,
        audio_analyzer,
        rot_seed,
        vars.numeric,
        vars.tables,
        static_cast<std::uint32_t>(layer_index),
        make_loop_sampler(layer.transform.rotation_property, fallback_rotation(layer), rot_seed));

    math::Vector2 scl = sample_vector2(
        layer.transform.scale_property,
        fallback_scale(layer),
        remapped_time,
        audio_analyzer,
        hash_combine(layer_seed, stable_string_hash("scale")),
        vars.numeric,
        vars.tables);
    
    evaluated.local_transform = make_transform2(pos, rot, scl);
    evaluated.world_matrix = evaluated.local_transform.to_matrix();
    evaluated.world_position3 = math::Vector3{pos.x, pos.y, 0.0f};

    if (layer.is_3d) {
        const std::uint64_t pos3_seed = hash_combine(layer_seed, stable_string_hash("position3"));
        const math::Vector3 pos3 = sample_vector3(
            layer.transform3d.position_property,
            {pos.x, pos.y, 0.0f},
            remapped_time,
            audio_analyzer,
            pos3_seed,
            vars.numeric,
            vars.tables);
        const math::Vector3 orient3 = sample_vector3(
            layer.transform3d.orientation_property,
            {0.0f, 0.0f, 0.0f},
            remapped_time,
            audio_analyzer,
            hash_combine(layer_seed, stable_string_hash("orientation3")),
            vars.numeric,
            vars.tables);
        const math::Vector3 rot3 = sample_vector3(
            layer.transform3d.rotation_property,
            {0.0f, 0.0f, static_cast<float>(rot)},
            remapped_time,
            audio_analyzer,
            hash_combine(layer_seed, stable_string_hash("rotation3")),
            vars.numeric,
            vars.tables);
        const math::Vector3 scl3 = sample_vector3(
            layer.transform3d.scale_property,
            {scl.x, scl.y, 1.0f},
            remapped_time,
            audio_analyzer,
            hash_combine(layer_seed, stable_string_hash("scale3")),
            vars.numeric,
            vars.tables);
        const math::Vector3 anchor3 = sample_vector3(
            layer.transform3d.anchor_point_property,
            {0.0f, 0.0f, 0.0f},
            remapped_time,
            audio_analyzer,
            hash_combine(layer_seed, stable_string_hash("anchor_point3")),
            vars.numeric,
            vars.tables);
        
        evaluated.orientation_xyz_deg = orient3;
        evaluated.anchor_point_3d = anchor3;
        evaluated.scale_3d = scl3;

        const auto t = math::Matrix4x4::translation(pos3);
        const auto o = math::Quaternion::from_euler({
            static_cast<float>(renderer2d::math_utils::degrees_to_radians(orient3.x)),
            static_cast<float>(renderer2d::math_utils::degrees_to_radians(orient3.y)),
            static_cast<float>(renderer2d::math_utils::degrees_to_radians(orient3.z))
        }).to_matrix();
        const auto r = math::Quaternion::from_euler({
            static_cast<float>(renderer2d::math_utils::degrees_to_radians(rot3.x)),
            static_cast<float>(renderer2d::math_utils::degrees_to_radians(rot3.y)),
            static_cast<float>(renderer2d::math_utils::degrees_to_radians(rot3.z))
        }).to_matrix();
        const auto s_matrix = math::Matrix4x4::scaling(scl3 / 100.0f);
        const auto inv_a = math::Matrix4x4::translation(-anchor3);
        
        evaluated.world_matrix = t * o * r * s_matrix * inv_a;
        evaluated.world_position3 = pos3;
        evaluated.world_normal = evaluated.world_matrix.transform_vector({0.0f, 0.0f, 1.0f}).normalized();
    }

    evaluated.material.ambient_coeff = static_cast<float>(sample_scalar(layer.ambient_coeff, 0.1, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("ambient")), vars.numeric, vars.tables));
    evaluated.material.diffuse_coeff = static_cast<float>(sample_scalar(layer.diffuse_coeff, 0.8, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("diffuse")), vars.numeric, vars.tables));
    evaluated.material.specular_coeff = static_cast<float>(sample_scalar(layer.specular_coeff, 0.5, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("specular")), vars.numeric, vars.tables));
    evaluated.material.shininess = static_cast<float>(sample_scalar(layer.shininess, 50.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("shininess")), vars.numeric, vars.tables));
    evaluated.material.metallic = static_cast<float>(sample_scalar(layer.metallic, 0.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("metallic")), vars.numeric, vars.tables));
    evaluated.material.roughness = static_cast<float>(sample_scalar(layer.roughness, 0.5, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("roughness")), vars.numeric, vars.tables));
    evaluated.material.emission = static_cast<float>(sample_scalar(layer.emission, 0.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("emission")), vars.numeric, vars.tables));
    evaluated.material.ior = static_cast<float>(sample_scalar(layer.ior, 1.45, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("ior")), vars.numeric, vars.tables));
    evaluated.material.metal = layer.metal;

    evaluated.extrusion_depth = static_cast<float>(sample_scalar(layer.extrusion_depth, 0.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("extrusion")), vars.numeric, vars.tables));
    evaluated.bevel_size = static_cast<float>(sample_scalar(layer.bevel_size, 0.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("bevel")), vars.numeric, vars.tables));

    evaluated.visible = evaluated.enabled && evaluated.active && evaluated.opacity > 0.0;
    evaluated.width = layer.width;
    evaluated.height = layer.height;

    // Resolve 3D mesh asset if path is provided
    if (layer.mesh_path.has_value() && !layer.mesh_path->empty()) {
        evaluated.mesh_asset = const_cast<media::MeshAsset*>(
            context.media->get_mesh(*layer.mesh_path, nullptr)
        );
        if (evaluated.mesh_asset) {
            evaluate_mesh_animations(evaluated, remapped_time);
        }
    }

    evaluated.text_content = resolve_template(layer.text_content, vars.strings, vars.numeric);
    evaluated.font_id = layer.font_id;
    evaluated.font_size = static_cast<float>(sample_scalar(layer.font_size, 48.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("font_size")), vars.numeric, vars.tables));
    
    if (layer.alignment == "center") evaluated.text_alignment = 1;
    else if (layer.alignment == "right") evaluated.text_alignment = 2;
    else evaluated.text_alignment = 0;
    
    evaluated.fill_color = sample_color(layer.fill_color, {255, 255, 255, 255}, remapped_time);
    evaluated.stroke_color = sample_color(layer.stroke_color, {255, 255, 255, 255}, remapped_time);
    evaluated.stroke_width = static_cast<float>(sample_scalar(
        layer.stroke_width_property,
        static_cast<double>(layer.stroke_width),
        remapped_time,
        audio_analyzer,
        hash_combine(layer_seed, stable_string_hash("stroke_width")),
        vars.numeric,
        vars.tables));
    evaluated.line_cap = layer.line_cap;
    evaluated.line_join = layer.line_join;
    evaluated.miter_limit = layer.miter_limit;
    
    evaluated.effects = layer.effects;
    evaluated.text_highlights = layer.text_highlights;
    evaluated.subtitle_path = layer.subtitle_path;
    evaluated.subtitle_outline_color = layer.subtitle_outline_color;
    evaluated.subtitle_outline_width = layer.subtitle_outline_width;

        if (layer.shape_path.has_value()) {
        EvaluatedShapePath shape_path;
        shape_path.closed = layer.shape_path->closed;
        shape_path.points.reserve(layer.shape_path->points.size());
        for (const auto& point : layer.shape_path->points) {
            shape_path.points.push_back(EvaluatedShapePathPoint{
                point.position,
                point.tangent_in,
                point.tangent_out
            });
        }

        // Shape Morphing
        if (!layer.morph_targets.empty()) {
            const double weight = sample_scalar(layer.morph_weight, 0.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("morph_weight")), vars.numeric, vars.tables) / 100.0;
            if (weight > 1e-4) {
                // For now, interpolate with the first target if point counts match
                const auto& target = layer.morph_targets[0];
                if (target.points.size() == shape_path.points.size()) {
                    const float w = static_cast<float>(std::clamp(weight, 0.0, 1.0));
                    for (size_t k = 0; k < shape_path.points.size(); ++k) {
                        shape_path.points[k].position = shape_path.points[k].position * (1.0f - w) + target.points[k].position * w;
                        shape_path.points[k].tangent_in = shape_path.points[k].tangent_in * (1.0f - w) + target.points[k].tangent_in * w;
                        shape_path.points[k].tangent_out = shape_path.points[k].tangent_out * (1.0f - w) + target.points[k].tangent_out * w;
                    }
                }
            }
        }

        const double t_start = sample_scalar(layer.trim_start, 0.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("trim_start")), vars.numeric, vars.tables) / 100.0;
        const double t_end = sample_scalar(layer.trim_end, 100.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("trim_end")), vars.numeric, vars.tables) / 100.0;
        const double t_offset = sample_scalar(layer.trim_offset, 0.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("trim_offset")), vars.numeric, vars.tables) / 360.0;

        if (t_start > 0.0 || t_end < 1.0 || std::abs(t_offset) > 1e-4) {
            // Apply Trim Path
            // Simplified version: truncate based on point indices for now
            // Future: accurate arc-length clipping
            const std::size_t n = shape_path.points.size();
            if (n > 1) {
                double start = std::clamp(t_start + t_offset, 0.0, 1.0);
                double end = std::clamp(t_end + t_offset, 0.0, 1.0);
                if (start > end) std::swap(start, end);
                
                const std::size_t start_idx = static_cast<std::size_t>(std::floor(start * (n - 1)));
                const std::size_t end_idx = static_cast<std::size_t>(std::ceil(end * (n - 1)));
                
                std::vector<EvaluatedShapePathPoint> trimmed;
                for (std::size_t k = start_idx; k <= end_idx && k < n; ++k) {
                    trimmed.push_back(shape_path.points[k]);
                }
                shape_path.points = std::move(trimmed);
                if (end < 1.0) shape_path.closed = false;
            }
        }

        evaluated.shape_path = std::move(shape_path);
    }

    evaluated.track_matte_type = layer.track_matte_type;
    evaluated.precomp_id = layer.precomp_id;

    if ((evaluated.type == LayerType::Image || evaluated.type == LayerType::Video) && scene) {
        for (const auto& asset : scene->assets) {
            if (asset.id == evaluated.id || asset.id == evaluated.name) {
                evaluated.asset_id = asset.id;
                evaluated.asset_path = asset.path;
                break;
            }
        }
    }

    evaluated.gradient_fill = layer.gradient_fill;
    evaluated.gradient_stroke = layer.gradient_stroke;

    return evaluated;
}

EvaluatedLightState evaluate_light_state(
    const EvaluatedLayerState& layer_state,
    const LayerSpec& spec,
    double remapped_time) {
    EvaluatedLightState light;
    light.layer_id = layer_state.id;
    light.type = spec.light_type.value_or("point");
    light.position = layer_state.world_position3;
    
    const auto forward = layer_state.world_matrix.transform_vector({0.0f, 0.0f, -1.0f}).normalized();
    light.direction = forward;
    
    const std::uint64_t light_seed = hash_combine(stable_string_hash(layer_state.id), stable_string_hash(light.type));
    light.color = sample_color(spec.fill_color, {255, 255, 255, 255}, remapped_time);
    light.intensity = static_cast<float>(sample_scalar(spec.intensity, 1.0, remapped_time, nullptr, hash_combine(light_seed, stable_string_hash("intensity"))));
    light.attenuation_near = static_cast<float>(sample_scalar(spec.attenuation_near, 0.0, remapped_time, nullptr, hash_combine(light_seed, stable_string_hash("attenuation_near"))));
    light.attenuation_far = static_cast<float>(sample_scalar(spec.attenuation_far, 1000.0, remapped_time, nullptr, hash_combine(light_seed, stable_string_hash("attenuation_far"))));
    
    light.cone_angle = static_cast<float>(sample_scalar(spec.cone_angle, 90.0, remapped_time, nullptr, hash_combine(light_seed, stable_string_hash("cone_angle"))));
    light.cone_feather = static_cast<float>(sample_scalar(spec.cone_feather, 50.0, remapped_time, nullptr, hash_combine(light_seed, stable_string_hash("cone_feather"))));
    light.falloff_type = spec.falloff_type;
    light.casts_shadows = spec.casts_shadows;
    light.shadow_darkness = static_cast<float>(sample_scalar(spec.shadow_darkness, 1.0, remapped_time, nullptr, hash_combine(light_seed, stable_string_hash("shadow_darkness"))));
    light.shadow_radius = static_cast<float>(sample_scalar(spec.shadow_radius, 0.0, remapped_time, nullptr, hash_combine(light_seed, stable_string_hash("shadow_radius"))));

    return light;
}


} // namespace scene
} // namespace tachyon
