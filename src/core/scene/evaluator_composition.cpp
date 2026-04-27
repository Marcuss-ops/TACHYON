#include "tachyon/core/scene/composition/evaluator_composition.h"
#include "tachyon/core/scene/evaluator/hashing.h"
#include "tachyon/core/scene/evaluator/property_sampler.h"
#include "tachyon/core/scene/evaluator/layer_evaluator.h"
#include "tachyon/core/scene/evaluator/light_evaluator.h"
#include "tachyon/core/scene/evaluator/layer_utils.h"
#include "tachyon/core/scene/math/evaluator_math.h"
#include "tachyon/timeline/time.h"

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
            evaluated.world_matrix = parent.world_matrix * evaluated.world_matrix;
            const auto wp3 = evaluated.world_matrix.transform_point({0.0f, 0.0f, 0.0f});
            evaluated.world_position3 = wp3;
            evaluated.visible = evaluated.enabled && evaluated.active && parent.visible && evaluated.opacity > 0.0;
        }
    }

    if (layer.track_matte_layer_id.has_value() && !layer.track_matte_layer_id->empty()) {
        const auto matte_it = context.layer_indices.find(*layer.track_matte_layer_id);
        if (matte_it != context.layer_indices.end()) {
            evaluated.track_matte_layer_index = matte_it->second;
        }
    }

    if (evaluated.precomp_id.has_value() && !evaluated.precomp_id->empty() && context.scene) {
        bool circular = false;
        for (const auto& id : context.composition_stack) {
            if (id == *evaluated.precomp_id) {
                circular = true;
                break;
            }
        }

        if (!circular && context.scene) {
            auto comp_it = context.composition_indices.find(*evaluated.precomp_id);
            if (comp_it != context.composition_indices.end()) {
                const auto& comp = context.scene->compositions[comp_it->second];
                std::vector<std::string> next_stack = context.composition_stack;
                next_stack.push_back(context.composition.id);
                
                const std::int64_t child_frame_number = static_cast<std::int64_t>(std::llround(
                    evaluated.child_time_seconds * 
                    static_cast<double>(comp.frame_rate.numerator) / 
                    static_cast<double>(comp.frame_rate.denominator)
                ));
                
                evaluated.nested_composition = std::make_unique<EvaluatedCompositionState>(
                    evaluate_composition_internal(context.scene, comp, child_frame_number, evaluated.child_time_seconds, std::move(next_stack), context.audio_analyzer, context.vars, context.media, context.main_frame_number, context.main_frame_time_seconds)
                );
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
    
    // Build component indices for O(1) lookup before expansion
    std::unordered_map<std::string, std::size_t> component_indices;
    component_indices.reserve(composition.components.size());
    for (std::size_t i = 0; i < composition.components.size(); ++i) {
        component_indices.emplace(composition.components[i].id, i);
    }

    // Expand component instances into layers (use pre-built index for O(1) lookup)
    CompositionSpec expanded = composition;
    for (const auto& inst : composition.component_instances) {
        auto comp_it = component_indices.find(inst.component_id);
        if (comp_it == component_indices.end()) continue;
        const auto& component = composition.components[comp_it->second];

        for (const auto& layer : component.layers) {
            LayerSpec new_layer = layer;
            new_layer.id = inst.instance_id + "_" + layer.id;
            // Apply param_values (basic implementation - just copy for now)
            // TODO: Apply param_values to layer properties
            expanded.layers.push_back(new_layer);
        }
    }
    
    const CompositionSpec& comp = expanded;
    
    EvaluatedCompositionState evaluated;
    evaluated.composition_id = comp.id;
    evaluated.composition_name = comp.name;
    evaluated.width = comp.width;
    evaluated.height = comp.height;
    evaluated.frame_rate = comp.frame_rate;
    evaluated.frame_number = frame_number;
    evaluated.composition_time_seconds = composition_time_seconds;
    evaluated.layers.reserve(comp.layers.size());

    vars.input_props = &comp.input_props;

    // Pre-build indices for O(1) lookups (avoid building in hot loop)
    std::unordered_map<std::string, std::size_t> layer_indices;
    layer_indices.reserve(comp.layers.size());
    for (std::size_t index = 0; index < comp.layers.size(); ++index) {
        layer_indices.emplace(comp.layers[index].id, index);
    }
    
    std::unordered_map<std::string, std::size_t> composition_indices;
    if (scene) {
        composition_indices.reserve(scene->compositions.size());
        for (std::size_t i = 0; i < scene->compositions.size(); ++i) {
            composition_indices.emplace(scene->compositions[i].id, i);
        }
    }

    // component_indices already built earlier (line 113-115), reuse it

    EvaluationContext context{
        scene,
        comp,
        frame_number,
        composition_time_seconds,
        std::move(layer_indices),
        std::move(composition_indices),
        std::move(component_indices),
        std::vector<std::optional<EvaluatedLayerState>>(comp.layers.size()),
        std::vector<bool>(comp.layers.size(), false),
        std::move(stack),
        audio_analyzer,
        vars,
        {},
        media,
        {},
        main_frame_number,
        main_frame_time_seconds
    };

    for (std::size_t index = 0; index < comp.layers.size(); ++index) {
        const auto& base_layer = resolve_layer_state(index, context);
        
        // Evaluate repeater count
        const double remapped_time = base_layer.child_time_seconds;
        const std::uint64_t layer_seed = make_property_expression_seed(scene, comp, comp.layers[index], "layer");
        const double rep_count = sample_scalar(comp.layers[index].repeater_count, 1.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("repeater_count")), vars.numeric);
        const int iterations = std::max(1, static_cast<int>(std::floor(rep_count)));

        if (iterations > 1) {
            const double stagger_delay = sample_scalar(comp.layers[index].repeater_stagger_delay, 0.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("stagger_delay")));
            const float off_x = static_cast<float>(sample_scalar(comp.layers[index].repeater_offset_position_x, 0.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("rep_off_x"))));
            const float off_y = static_cast<float>(sample_scalar(comp.layers[index].repeater_offset_position_y, 0.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("rep_off_y"))));
            const float off_rot = static_cast<float>(sample_scalar(comp.layers[index].repeater_offset_rotation, 0.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("rep_off_rot"))));
            const float off_sx = static_cast<float>(sample_scalar(comp.layers[index].repeater_offset_scale_x, 100.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("rep_off_sx")))) / 100.0f;
            const float off_sy = static_cast<float>(sample_scalar(comp.layers[index].repeater_offset_scale_y, 100.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("rep_off_sy")))) / 100.0f;
            const float start_op = static_cast<float>(sample_scalar(comp.layers[index].repeater_start_opacity, 100.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("rep_op_start")))) / 100.0f;
            const float end_op = static_cast<float>(sample_scalar(comp.layers[index].repeater_end_opacity, 100.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("rep_op_end")))) / 100.0f;

            // Pre-compute the per-step transform matrix (constant for all steps)
            const math::Matrix4x4 step_transform = math::compose_trs(
                {off_x, off_y, 0.0f},
                math::Quaternion::from_euler({0, 0, off_rot}),
                {off_sx, off_sy, 1.0f}
            );
            
            // Pre-allocate ID prefix to avoid repeated string allocations
            const std::string id_prefix = base_layer.id + "_rep_";
            
            // Iterative cumulative transform (O(N) instead of O(N²))
            math::Matrix4x4 cumulative_transform = math::Matrix4x4::identity();
            
            for (int r = 0; r < iterations; ++r) {
                // For stagger, we re-evaluate the layer state with a time offset
                EvaluatedLayerState repeated = (stagger_delay != 0.0) 
                    ? make_layer_state(context, comp.layers[index], index, static_cast<double>(r) * stagger_delay, context.vars)
                    : base_layer;
            
                // Build ID without allocation in loop (use pre-allocated prefix)
                repeated.id = id_prefix + std::to_string(r);
            
                // Update cumulative transform iteratively (O(1) per iteration)
                cumulative_transform = cumulative_transform * step_transform;
                repeated.world_matrix = repeated.world_matrix * cumulative_transform;
                const auto wp3 = repeated.world_matrix.transform_point({0.0f, 0.0f, 0.0f});
                repeated.world_position3 = wp3;
            
                // Opacity ramp
                const float t_ramp = iterations > 1 ? static_cast<float>(r) / static_cast<float>(iterations - 1) : 0.0f;
                repeated.opacity *= (start_op * (1.0f - t_ramp) + end_op * t_ramp);
            
                if (repeated.type == LayerType::Light) {
                    evaluated.lights.push_back(evaluate_light_state(repeated, comp.layers[index], repeated.child_time_seconds));
                } else {
                    evaluated.layers.push_back(std::move(repeated));
                }
            }
        }
    else {
            if (base_layer.type == LayerType::Light) {
                evaluated.lights.push_back(evaluate_light_state(base_layer, comp.layers[index], base_layer.child_time_seconds));
            } else {
                evaluated.layers.push_back(base_layer);
            }
        }
    }

    solve_constraints(evaluated.layers);

    evaluated.camera = evaluate_camera_state(comp, evaluated.layers, frame_number, composition_time_seconds);

    // Resolve environment map
    if (comp.environment_path.has_value() && !comp.environment_path->empty() && media) {
        evaluated.environment_map = media->get_hdr_image(*comp.environment_path);
    }

    return evaluated;
}


} // namespace scene
} // namespace tachyon
