#include "tachyon/core/scene/evaluator/layer_evaluator.h"
#include "tachyon/core/scene/evaluator/layer_utils.h"
#include "tachyon/core/scene/evaluator/mesh_animator.h"
#include "tachyon/core/scene/evaluator/property_sampler.h"
#include "tachyon/core/scene/evaluator/hashing.h"
#include "tachyon/core/scene/evaluator/templates.h"

#include "tachyon/core/scene/math/evaluator_math.h"
#include "tachyon/renderer2d/animation/easing.h"
#include "tachyon/core/math/quaternion.h"
#include "tachyon/core/math/transform2.h"
#include "tachyon/timeline/time.h"
#include "tachyon/renderer2d/math/math_utils.h"
#include "tachyon/media/management/media_manager.h"

#include <algorithm>
#include <cmath>

namespace tachyon::scene {

EvaluatedLayerState make_layer_state(
    EvaluationContext& context,
    const LayerSpec& layer,
    std::size_t layer_index,
    double time_offset) {
    
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
    
    const double effective_comp_time = (!layer.motion_blur && context.main_frame_time_seconds.has_value()) 
        ? *context.main_frame_time_seconds 
        : composition_time_seconds;

    evaluated.local_time_seconds = timeline::local_time_from_composition(effective_comp_time, layer.start_time) + time_offset;
    
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
    evaluated.parent_id = layer.parent.value_or("");
    evaluated.local_position3 = {pos.x, pos.y, 0.0f};
    evaluated.local_rotation3 = math::Quaternion::from_axis_angle({0, 0, 1}, static_cast<float>(renderer2d::math_utils::degrees_to_radians(rot)));
    evaluated.local_scale3 = {scl.x / 100.0f, scl.y / 100.0f, 1.0f};

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

        const math::Quaternion q_orient = math::Quaternion::from_euler(orient3);
        const math::Quaternion q_rot = math::Quaternion::from_euler(rot3);
        const math::Quaternion combined_rot = q_orient * q_rot;

        evaluated.local_position3 = pos3;
        evaluated.local_rotation3 = combined_rot;
        evaluated.local_scale3 = scl3 / 100.0f;

        const auto t = math::Matrix4x4::translation(pos3);
        const auto r = combined_rot.to_matrix();
        const auto s_matrix = math::Matrix4x4::scaling(evaluated.local_scale3);
        const auto inv_a = math::Matrix4x4::translation(-anchor3);
        
        evaluated.world_matrix = t * r * s_matrix * inv_a;
        evaluated.world_position3 = pos3;
        evaluated.world_normal = evaluated.world_matrix.transform_vector({0.0f, 0.0f, 1.0f}).normalized();
    }

    evaluated.material.metallic = static_cast<float>(sample_scalar(layer.metallic, 0.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("metallic")), vars.numeric, vars.tables));
    evaluated.material.roughness = static_cast<float>(sample_scalar(layer.roughness, 0.5, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("roughness")), vars.numeric, vars.tables));
    evaluated.material.emission = static_cast<float>(sample_scalar(layer.emission, 0.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("emission")), vars.numeric, vars.tables));
    evaluated.material.ior = static_cast<float>(sample_scalar(layer.ior, 1.45, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("ior")), vars.numeric, vars.tables));
    evaluated.material.specular = static_cast<float>(sample_scalar(layer.specular_coeff, 0.5, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("specular")), vars.numeric, vars.tables));
    evaluated.material.subsurface = 0.0f;
    evaluated.material.specular_tint = 0.0f;
    evaluated.material.anisotropic = 0.0f;
    evaluated.material.sheen = 0.0f;
    evaluated.material.sheen_tint = 0.5f;
    evaluated.material.clearcoat = 0.0f;
    evaluated.material.clearcoat_roughness = 0.03f;
    evaluated.material.transmission = 0.0f;

    evaluated.extrusion_depth = static_cast<float>(sample_scalar(layer.extrusion_depth, 0.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("extrusion")), vars.numeric, vars.tables));
    evaluated.bevel_size = static_cast<float>(sample_scalar(layer.bevel_size, 0.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("bevel")), vars.numeric, vars.tables));

    evaluated.visible = evaluated.enabled && evaluated.active && evaluated.opacity > 0.0;
    evaluated.width = layer.width;
    evaluated.height = layer.height;

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
    else if (layer.alignment == "justify") evaluated.text_alignment = 3;
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
    evaluated.text_animators = layer.text_animators;
    evaluated.text_highlights = layer.text_highlights;
    evaluated.mask_feather = static_cast<float>(sample_scalar(
        layer.mask_feather,
        0.0,
        remapped_time,
        audio_analyzer,
        hash_combine(layer_seed, stable_string_hash("mask_feather")),
        vars.numeric,
        vars.tables));
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

        if (!layer.morph_targets.empty()) {
            const double weight = sample_scalar(layer.morph_weight, 0.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("morph_weight")), vars.numeric, vars.tables) / 100.0;
            if (weight > 1e-4) {
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

        evaluated.trim_start = static_cast<float>(sample_scalar(layer.trim_start, 0.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("trim_start")), vars.numeric, vars.tables) / 100.0);
        evaluated.trim_end = static_cast<float>(sample_scalar(layer.trim_end, 100.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("trim_end")), vars.numeric, vars.tables) / 100.0);
        evaluated.trim_offset = static_cast<float>(sample_scalar(layer.trim_offset, 0.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("trim_offset")), vars.numeric, vars.tables) / 360.0);

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
    
    evaluated.constraints = layer.constraints;
    evaluated.ik_chains = layer.ik_chains;

    return evaluated;
}

} // namespace tachyon::scene
