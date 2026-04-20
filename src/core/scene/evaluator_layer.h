#pragma once

#include "evaluator_internal.h"
#include "evaluator_utils.h"
#include "tachyon/core/scene/evaluator.h"

#include <string>

namespace tachyon {

namespace scene {

namespace {

LayerType map_layer_type(const std::string& type);

math::Transform2 make_transform2(const math::Vector2& position, double rotation_degrees, const math::Vector2& scale);

void evaluate_mesh_animations(EvaluatedLayerState& evaluated, double time);

EvaluatedLayerState make_layer_state(EvaluationContext& context, const LayerSpec& layer, std::size_t layer_index);

EvaluatedLightState evaluate_light_state(const EvaluatedLayerState& layer_state, const LayerSpec& spec, double remapped_time);

} // namespace

} // namespace scene

} // namespace tachyon