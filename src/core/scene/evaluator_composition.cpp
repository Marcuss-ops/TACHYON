#include "tachyon/core/scene/composition/evaluator_composition.h"
#include "tachyon/core/scene/evaluator/hashing.h"
#include "tachyon/core/scene/evaluator/property_sampler.h"
#include "tachyon/core/scene/evaluator/layer_dependency_resolver.h"
#include "tachyon/core/scene/evaluator/precomp_evaluator.h"
#include "tachyon/core/scene/evaluator/repeater_evaluator.h"
#include "tachyon/core/scene/evaluator/camera2d_evaluator.h"
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
    context.current_layer_index = layer_index;

        const auto& layer = context.composition.layers[layer_index];
        EvaluatedLayerState evaluated = make_layer_state(
            context,
            layer,
            layer_index,
            0.0);

    resolve_layer_dependencies(layer_index, context, evaluated);

    attach_nested_precomp(context, evaluated);

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
    const ::tachyon::audio::IAudioAnalyzer* audio_analyzer,
    EvaluationVariables vars,
    media::IMediaProvider* media,
    std::optional<std::int64_t> main_frame_number,
    std::optional<double> main_frame_time_seconds) {
    
    EvaluatedCompositionState evaluated;
    evaluated.composition_id = composition.id;
    evaluated.width = static_cast<std::uint32_t>(composition.width);
    evaluated.height = static_cast<std::uint32_t>(composition.height);
    evaluated.duration_seconds = composition.duration;
    evaluated.composition_time_seconds = composition_time_seconds;
    evaluated.frame_number = frame_number;
    
    // Background baking is now handled by SceneNormalizer
    
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
        evaluate_repeater_expansion(scene, composition, index, base_layer, context, evaluated.layers);
    }

    // Camera Evaluation
    evaluated.active_camera = evaluate_camera2d_state(
        scene,
        composition,
        composition_time_seconds,
        audio_analyzer,
        vars
    );

    return evaluated;
}


} // namespace scene
} // namespace tachyon
