#include "tachyon/core/scene/composition/evaluator_composition.h"
#include "tachyon/core/scene/evaluator/hashing.h"
#include "tachyon/core/scene/evaluator/property_sampler.h"
#include "tachyon/core/scene/evaluator/layer_evaluator.h"
#include "tachyon/core/scene/evaluator/layer_utils.h"
#include "tachyon/core/scene/math/evaluator_math.h"
#include "tachyon/timeline/time.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/spec/schema/objects/composition_spec.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"

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


const EvaluatedLayerState& resolve_layer_state(
    std::size_t layer_index,
    EvaluationContext& context) {
    if (context.cache[layer_index].has_value()) {
        return *context.cache[layer_index];
    }

    if (context.visiting[layer_index]) {
        context.cache[layer_index] = make_layer_state(
            context,
            context.composition.layers[layer_index],
            layer_index);
        return *context.cache[layer_index];
    }

    context.visiting[layer_index] = true;

        const auto& layer = context.composition.layers[layer_index];
        EvaluatedLayerState evaluated = make_layer_state(
            context,
            layer,
            layer_index,
            0.0,
            context.vars);

    if (layer.parent.has_value() && !layer.parent->empty()) {
        const auto parent_it = context.layer_indices.find(*layer.parent);
        if (parent_it != context.layer_indices.end() && parent_it->second != layer_index) {
            const auto& parent = resolve_layer_state(parent_it->second, context);
            evaluated.transform.world_matrix = parent.transform.world_matrix * evaluated.transform.world_matrix;
            evaluated.identity.visible = evaluated.identity.enabled && evaluated.identity.active && parent.identity.visible && evaluated.transform.opacity > 0.0;
        }
    }

    if (layer.track_matte_layer_id.has_value() && !layer.track_matte_layer_id->empty()) {
        const auto matte_it = context.layer_indices.find(*layer.track_matte_layer_id);
        if (matte_it != context.layer_indices.end()) {
            evaluated.track_matte_layer_index = matte_it->second;
        }
    }

    if (evaluated.source.precomp_id.has_value() && !evaluated.source.precomp_id->empty() && context.scene) {
        bool circular = false;
        for (const auto& id : context.composition_stack) {
            if (id == *evaluated.source.precomp_id) {
                circular = true;
                break;
            }
        }

        if (!circular) {
            for (const auto& comp : context.scene->compositions) {
                if (comp.id == *evaluated.source.precomp_id) {
                    std::vector<std::string> next_stack = context.composition_stack;
                    next_stack.push_back(context.composition.id);
                    
                    const std::int64_t child_frame_number = static_cast<std::int64_t>(std::llround(
                        evaluated.playback.local_time_seconds * 
                        static_cast<double>(comp.frame_rate.numerator) / 
                        static_cast<double>(comp.frame_rate.denominator)
                    ));

                    evaluated.nested_composition = std::make_unique<EvaluatedCompositionState>(
                        evaluate_composition_internal(context.scene, comp, child_frame_number, evaluated.playback.local_time_seconds, std::move(next_stack), context.audio_analyzer, context.vars, context.media, context.main_frame_number, context.main_frame_time_seconds)
                    );
                    break;
                }
            }
        }
    }

    context.visiting[layer_index] = false;
    context.cache[layer_index] = std::move(evaluated);
    return *context.cache[layer_index];
}

EvaluatedCompositionState evaluate_composition_internal(
    const SceneSpec* scene,
    const CompositionSpec& composition,
    std::int64_t frame_number,
    double composition_time_seconds,
    std::vector<std::string> stack,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer,
    EvaluationVariables vars,
    media::MediaManager* media,
    std::optional<std::int64_t> main_frame_number,
    std::optional<double> main_frame_time_seconds) {
    
    EvaluatedCompositionState evaluated;
    evaluated.composition_id = composition.id;
    evaluated.width = static_cast<std::uint32_t>(composition.width);
    evaluated.height = static_cast<std::uint32_t>(composition.height);
    evaluated.duration_seconds = composition.duration;
    evaluated.composition_time_seconds = composition_time_seconds;
    evaluated.frame_number = frame_number;
    
    bool has_baked_background = std::any_of(composition.layers.begin(), composition.layers.end(), 
        [](const auto& l) { return l.identity.id == "__background__" || l.identity.name == "Background"; });

    if (composition.background.has_value() && !has_baked_background) {
        if (composition.background->is_color()) {
            auto color = composition.background->get_color();
            if (color.has_value()) {
                EvaluatedLayerState bg_layer;
                bg_layer.identity.id = "__background__";
                bg_layer.identity.name = "Background";
                bg_layer.identity.type = LayerType::Solid;
                bg_layer.transform.width = composition.width;
                bg_layer.transform.height = composition.height;
                bg_layer.text.fill_color = *color;
                bg_layer.identity.visible = true;
                bg_layer.identity.enabled = true;
                bg_layer.identity.active = true;
                bg_layer.transform.opacity = 1.0f;
                bg_layer.playback.in_time = 0.0;
                bg_layer.playback.out_time = composition.duration;
                bg_layer.transform.world_matrix = math::Matrix3x3::identity();
                evaluated.layers.push_back(std::move(bg_layer));
            }
        } else if (composition.background->type == BackgroundType::Asset) {
            EvaluatedLayerState bg_layer;
            bg_layer.identity.id = "__background__";
            bg_layer.identity.name = "Background";
            bg_layer.identity.type = LayerType::Image;
            bg_layer.transform.width = composition.width;
            bg_layer.transform.height = composition.height;
            bg_layer.source.asset_path = composition.background->value;
            bg_layer.identity.visible = true;
            bg_layer.identity.enabled = true;
            bg_layer.identity.active = true;
            bg_layer.transform.opacity = 1.0f;
            bg_layer.playback.in_time = 0.0;
            bg_layer.playback.out_time = composition.duration;
            bg_layer.transform.world_matrix = math::Matrix3x3::identity();
            evaluated.layers.push_back(std::move(bg_layer));
        }
    }
    
    std::size_t total_estimated_layers = composition.layers.size();
    for (const auto& l : composition.layers) {
        if (l.repeater.count.binding.active || l.repeater.count.keyframes.size() > 0) {
            total_estimated_layers += 10;
        } else {
            total_estimated_layers += std::max(0, static_cast<int>(l.repeater.count.value.value_or(1.0)) - 1);
        }
    }
    evaluated.layers.reserve(total_estimated_layers);

    EvaluationContext context{
        .scene = scene,
        .composition = composition,
        .frame_number = frame_number,
        .composition_time_seconds = composition_time_seconds,
        .cache = std::vector<std::optional<EvaluatedLayerState>>(composition.layers.size()),
        .visiting = std::vector<bool>(composition.layers.size(), false),
        .composition_stack = std::move(stack),
        .audio_analyzer = audio_analyzer,
        .vars = vars,
        .media = media,
        .main_frame_number = main_frame_number,
        .main_frame_time_seconds = main_frame_time_seconds
    };

    for (std::size_t index = 0; index < composition.layers.size(); ++index) {
        context.layer_indices.emplace(composition.layers[index].identity.id, index);
    }
    for (std::size_t index = 0; index < composition.cameras_2d.size(); ++index) {
        context.camera2d_indices.emplace(composition.cameras_2d[index].id, index);
    }

    for (std::size_t index = 0; index < composition.layers.size(); ++index) {
        const auto& base_layer = resolve_layer_state(index, context);
        
        const double remapped_time = base_layer.playback.local_time_seconds;
        const std::uint64_t layer_seed = make_property_expression_seed(scene, composition, composition.layers[index], "layer");
        const double rep_count = sample_scalar(composition.layers[index].repeater.count, 1.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("repeater_count")), vars.numeric);
        const int iterations = std::max(1, static_cast<int>(std::floor(rep_count)));

        if (iterations > 1) {
            const double stagger_delay = sample_scalar(composition.layers[index].repeater.stagger_delay, 0.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("stagger_delay")));
            const float off_x = static_cast<float>(sample_scalar(composition.layers[index].repeater.offset_position_x, 0.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("rep_off_x"))));
            const float off_y = static_cast<float>(sample_scalar(composition.layers[index].repeater.offset_position_y, 0.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("rep_off_y"))));
            const float off_rot = static_cast<float>(sample_scalar(composition.layers[index].repeater.offset_rotation, 0.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("rep_off_rot"))));
            const float off_sx = static_cast<float>(sample_scalar(composition.layers[index].repeater.offset_scale_x, 100.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("rep_off_sx")))) / 100.0f;
            const float off_sy = static_cast<float>(sample_scalar(composition.layers[index].repeater.offset_scale_y, 100.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("rep_off_sy")))) / 100.0f;
            const float start_op = static_cast<float>(sample_scalar(composition.layers[index].repeater.start_opacity, 100.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("rep_op_start")))) / 100.0f;
            const float end_op = static_cast<float>(sample_scalar(composition.layers[index].repeater.end_opacity, 100.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("rep_op_end")))) / 100.0f;

            const math::Matrix3x3 step_transform = math::Matrix3x3::make_translation({off_x, off_y}) *
                math::Matrix3x3::make_rotation(static_cast<float>(degrees_to_radians(off_rot))) *
                math::Matrix3x3::make_scale(off_sx, off_sy);
            
            math::Matrix3x3 current_offset_transform = math::Matrix3x3::identity();

            for (int r = 0; r < iterations; ++r) {
                EvaluatedLayerState repeated = (stagger_delay != 0.0) 
                    ? make_layer_state(context, composition.layers[index], index, static_cast<double>(r) * stagger_delay, context.vars)
                    : base_layer;

                if (stagger_delay != 0.0 && composition.layers[index].parent.has_value()) {
                    const auto parent_it = context.layer_indices.find(*composition.layers[index].parent);
                    if (parent_it != context.layer_indices.end()) {
                        const auto& parent_state = resolve_layer_state(parent_it->second, context);
                        repeated.transform.world_matrix = parent_state.transform.world_matrix * repeated.transform.world_matrix;
                        repeated.identity.visible = repeated.identity.enabled && repeated.identity.active && parent_state.identity.visible && repeated.transform.opacity > 0.0;
                    }
                }

                repeated.identity.id = base_layer.id() + "_rep_" + std::to_string(r);
                
                repeated.transform.world_matrix = repeated.transform.world_matrix * current_offset_transform;
                current_offset_transform = current_offset_transform * step_transform;
                
                const float t_ramp = iterations > 1 ? static_cast<float>(r) / static_cast<float>(iterations - 1) : 0.0f;
                repeated.transform.opacity *= (start_op * (1.0f - t_ramp) + end_op * t_ramp);

                evaluated.layers.push_back(std::move(repeated));
            }
        } else {
            evaluated.layers.push_back(base_layer);
        }
    }

    // Camera Evaluation
    if (!composition.cameras_2d.empty()) {
        const Camera2DSpec* active_cam = nullptr;
        if (composition.active_camera2d_id.has_value() && !composition.active_camera2d_id->empty()) {
            for (const auto& cam : composition.cameras_2d) {
                if (cam.id == *composition.active_camera2d_id) {
                    active_cam = &cam;
                    break;
                }
            }
        }
        
        if (!active_cam) active_cam = &composition.cameras_2d[0];

        if (active_cam && active_cam->enabled) {
            EvaluatedCameraState cam_state;
            cam_state.id = active_cam->id;
            cam_state.active = true;
            
            const double t = composition_time_seconds;
            const std::uint64_t cam_seed = stable_string_hash(active_cam->id);
            
            const float zoom = static_cast<float>(sample_scalar(active_cam->zoom, 1.0, t, audio_analyzer, hash_combine(cam_seed, stable_string_hash("zoom"))));
            const math::Vector2 pos = sample_vector2(active_cam->position, {static_cast<float>(composition.width)/2.0f, static_cast<float>(composition.height)/2.0f}, t, audio_analyzer, hash_combine(cam_seed, stable_string_hash("pos")));
            const float rot = static_cast<float>(sample_scalar(active_cam->rotation, 0.0, t, audio_analyzer, hash_combine(cam_seed, stable_string_hash("rot"))));
            const math::Vector2 scale = sample_vector2(active_cam->scale, {100.0f, 100.0f}, t, audio_analyzer, hash_combine(cam_seed, stable_string_hash("scale")));
            const math::Vector2 anchor = sample_vector2(active_cam->anchor_point, {0.0f, 0.0f}, t, audio_analyzer, hash_combine(cam_seed, stable_string_hash("anchor")));

            cam_state.zoom = zoom;

            // View matrix is the inverse of the camera's world transform
            math::Transform2 cam_xform;
            cam_xform.position = pos;
            cam_xform.rotation = static_cast<float>(degrees_to_radians(rot));
            cam_xform.scale = {scale.x / 100.0f, scale.y / 100.0f};
            cam_xform.anchor_point = anchor;

            cam_state.view_matrix = cam_xform.to_matrix().inverse();
            
            // Projection: zoom around the center of the composition
            const float cx = static_cast<float>(composition.width) * 0.5f;
            const float cy = static_cast<float>(composition.height) * 0.5f;
            
            cam_state.projection_matrix = 
                math::Matrix3x3::make_translation({cx, cy}) *
                math::Matrix3x3::make_scale(zoom, zoom) *
                math::Matrix3x3::make_translation({-cx, -cy});

            cam_state.view_projection = cam_state.projection_matrix * cam_state.view_matrix;
            evaluated.active_camera = std::move(cam_state);
        }
    }

    return evaluated;
}


} // namespace scene
} // namespace tachyon
