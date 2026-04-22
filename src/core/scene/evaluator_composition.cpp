#include "tachyon/core/scene/evaluator_composition.h"
#include "tachyon/core/scene/evaluator/hashing.h"
#include "tachyon/core/scene/evaluator/property_sampler.h"
#include "tachyon/core/scene/evaluator/layer_evaluator.h"
#include "tachyon/core/scene/evaluator/light_evaluator.h"
#include "tachyon/core/scene/evaluator/layer_utils.h"
#include "tachyon/core/scene/evaluator_math.h"
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
        layer_index);

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

        if (!circular) {
            for (const auto& comp : context.scene->compositions) {
                if (comp.id == *evaluated.precomp_id) {
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
    evaluated.composition_name = composition.name;
    evaluated.width = composition.width;
    evaluated.height = composition.height;
    evaluated.frame_rate = composition.frame_rate;
    evaluated.frame_number = frame_number;
    evaluated.composition_time_seconds = composition_time_seconds;
    evaluated.layers.reserve(composition.layers.size());

    EvaluationContext context{
        scene,
        composition,
        frame_number,
        composition_time_seconds,
        {},
        std::vector<std::optional<EvaluatedLayerState>>(composition.layers.size()),
        std::vector<bool>(composition.layers.size(), false),
        std::move(stack),
        audio_analyzer,
        vars,
        {},
        media,
        {},
        main_frame_number,
        main_frame_time_seconds
    };

    for (std::size_t index = 0; index < composition.layers.size(); ++index) {
        context.layer_indices.emplace(composition.layers[index].id, index);
    }

    for (std::size_t index = 0; index < composition.layers.size(); ++index) {
        const auto& base_layer = resolve_layer_state(index, context);
        
        // Evaluate repeater count
        const double remapped_time = base_layer.child_time_seconds;
        const std::uint64_t layer_seed = make_property_expression_seed(scene, composition, composition.layers[index], "layer");
        const double rep_count = sample_scalar(composition.layers[index].repeater_count, 1.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("repeater_count")), vars.numeric);
        const int iterations = std::max(1, static_cast<int>(std::floor(rep_count)));

        if (iterations > 1) {
        const double stagger_delay = sample_scalar(composition.layers[index].repeater_stagger_delay, 0.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("stagger_delay")));
        const float off_x = static_cast<float>(sample_scalar(composition.layers[index].repeater_offset_position_x, 0.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("rep_off_x"))));
        const float off_y = static_cast<float>(sample_scalar(composition.layers[index].repeater_offset_position_y, 0.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("rep_off_y"))));
        const float off_rot = static_cast<float>(sample_scalar(composition.layers[index].repeater_offset_rotation, 0.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("rep_off_rot"))));
        const float off_sx = static_cast<float>(sample_scalar(composition.layers[index].repeater_offset_scale_x, 100.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("rep_off_sx")))) / 100.0f;
        const float off_sy = static_cast<float>(sample_scalar(composition.layers[index].repeater_offset_scale_y, 100.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("rep_off_sy")))) / 100.0f;
        const float start_op = static_cast<float>(sample_scalar(composition.layers[index].repeater_start_opacity, 100.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("rep_op_start")))) / 100.0f;
        const float end_op = static_cast<float>(sample_scalar(composition.layers[index].repeater_end_opacity, 100.0, remapped_time, audio_analyzer, hash_combine(layer_seed, stable_string_hash("rep_op_end")))) / 100.0f;

            for (int r = 0; r < iterations; ++r) {
                // For stagger, we re-evaluate the layer state with a time offset
                EvaluatedLayerState repeated = (stagger_delay != 0.0) 
                    ? make_layer_state(context, composition.layers[index], index, static_cast<double>(r) * stagger_delay)
                    : base_layer;

                repeated.id = base_layer.id + "_rep_" + std::to_string(r);
                
                // Cumulative transform
                math::Matrix4x4 offset_transform = math::Matrix4x4::identity();
                for (int step = 0; step < r; ++step) {
                   offset_transform = offset_transform * math::compose_trs(
                        {off_x, off_y, 0.0f},
                        math::Quaternion::from_euler({0, 0, off_rot}),
                        {off_sx, off_sy, 1.0f}
                    );
                }
                
                repeated.world_matrix = repeated.world_matrix * offset_transform;
                const auto wp3 = repeated.world_matrix.transform_point({0.0f, 0.0f, 0.0f});
                repeated.world_position3 = wp3;
                
                // Opacity ramp
                const float t_ramp = iterations > 1 ? static_cast<float>(r) / static_cast<float>(iterations - 1) : 0.0f;
                repeated.opacity *= (start_op * (1.0f - t_ramp) + end_op * t_ramp);

                if (repeated.type == LayerType::Light) {
                    evaluated.lights.push_back(evaluate_light_state(repeated, composition.layers[index], repeated.child_time_seconds));
                } else {
                    evaluated.layers.push_back(std::move(repeated));
                }
            }
        }
 else {
            if (base_layer.type == LayerType::Light) {
                evaluated.lights.push_back(evaluate_light_state(base_layer, composition.layers[index], base_layer.child_time_seconds));
            } else {
                evaluated.layers.push_back(base_layer);
            }
        }
    }

    evaluated.camera = evaluate_camera_state(composition, evaluated.layers, frame_number, composition_time_seconds);

    // Resolve environment map
    if (composition.environment_path.has_value() && !composition.environment_path->empty() && media) {
        evaluated.environment_map = media->get_hdr_image(*composition.environment_path);
    }

    return evaluated;
}


} // namespace scene
} // namespace tachyon
