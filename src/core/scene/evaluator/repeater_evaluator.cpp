#include "tachyon/core/scene/evaluator/repeater_evaluator.h"
#include "tachyon/core/scene/evaluator/property_sampler.h"
#include "tachyon/core/scene/evaluator/hashing.h"
#include "tachyon/core/scene/evaluator/layer_evaluator.h"
#include "tachyon/core/scene/math/evaluator_math.h"
#include <cmath>

namespace tachyon::scene {

void evaluate_repeater_expansion(
    const SceneSpec* scene,
    const CompositionSpec& composition,
    std::size_t layer_index,
    const EvaluatedLayerState& base_layer,
    EvaluationContext& context,
    std::vector<EvaluatedLayerState>& output_layers) {
    
    const auto& layer_spec = composition.layers[layer_index];
    const double remapped_time = base_layer.playback.local_time_seconds;
    const std::uint64_t layer_seed = make_property_expression_seed(scene, composition, layer_spec, "layer");
    
    // Resolve audio bands once for all repeater properties
    ::tachyon::audio::AudioBands bands = context.audio_analyzer ? context.audio_analyzer->analyze_frame(context.composition_time_seconds) : ::tachyon::audio::AudioBands{};

    const double rep_count = sample_scalar(layer_spec.repeater.count, 1.0, remapped_time, bands, hash_combine(layer_seed, stable_string_hash("repeater_count")), context.vars.numeric);
    const int iterations = std::max(1, static_cast<int>(std::floor(rep_count)));

    if (iterations <= 1) {
        output_layers.push_back(base_layer);
        return;
    }

    const double stagger_delay = sample_scalar(layer_spec.repeater.stagger_delay, 0.0, remapped_time, bands, hash_combine(layer_seed, stable_string_hash("stagger_delay")));
    const float off_x = static_cast<float>(sample_scalar(layer_spec.repeater.offset_position_x, 0.0, remapped_time, bands, hash_combine(layer_seed, stable_string_hash("rep_off_x"))));
    const float off_y = static_cast<float>(sample_scalar(layer_spec.repeater.offset_position_y, 0.0, remapped_time, bands, hash_combine(layer_seed, stable_string_hash("rep_off_y"))));
    const float off_rot = static_cast<float>(sample_scalar(layer_spec.repeater.offset_rotation, 0.0, remapped_time, bands, hash_combine(layer_seed, stable_string_hash("rep_off_rot"))));
    const float off_sx = static_cast<float>(sample_scalar(layer_spec.repeater.offset_scale_x, 100.0, remapped_time, bands, hash_combine(layer_seed, stable_string_hash("rep_off_sx")))) / 100.0f;
    const float off_sy = static_cast<float>(sample_scalar(layer_spec.repeater.offset_scale_y, 100.0, remapped_time, bands, hash_combine(layer_seed, stable_string_hash("rep_off_sy")))) / 100.0f;
    const float start_op = static_cast<float>(sample_scalar(layer_spec.repeater.start_opacity, 100.0, remapped_time, bands, hash_combine(layer_seed, stable_string_hash("rep_op_start")))) / 100.0f;
    const float end_op = static_cast<float>(sample_scalar(layer_spec.repeater.end_opacity, 100.0, remapped_time, bands, hash_combine(layer_seed, stable_string_hash("rep_op_end")))) / 100.0f;

    const math::Matrix3x3 step_transform = math::Matrix3x3::make_translation({off_x, off_y}) *
        math::Matrix3x3::make_rotation(static_cast<float>(degrees_to_radians(off_rot))) *
        math::Matrix3x3::make_scale(off_sx, off_sy);
    
    math::Matrix3x3 current_offset_transform = math::Matrix3x3::identity();

    for (int r = 0; r < iterations; ++r) {
        EvaluatedLayerState repeated = (stagger_delay != 0.0) 
            ? make_layer_state(context, layer_spec, layer_index, static_cast<double>(r) * stagger_delay)
            : base_layer;

        if (stagger_delay != 0.0 && layer_spec.parent.has_value()) {
            const auto parent_it = context.layer_indices.find(*layer_spec.parent);
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

        output_layers.push_back(std::move(repeated));
    }
}

} // namespace tachyon::scene
