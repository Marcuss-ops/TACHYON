#pragma once

#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/expressions/expression_engine.h"

namespace tachyon {
namespace scene {

// Forward declaration for EvaluatedLayerState to minimize compilation dependencies
struct EvaluatedLayerState;
struct EvaluatedCameraState;
struct EvaluatedCompositionState;
struct EvaluatedShapePath;
struct EvaluatedLightState;

} // namespace scene
} // namespace tachyon