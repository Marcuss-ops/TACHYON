#pragma once

#include "tachyon/core/scene/evaluation/evaluator.h"

namespace media { struct MeshAsset; }

namespace tachyon::scene {

void evaluate_mesh_animations(EvaluatedLayerState& evaluated, const media::MeshAsset* asset, double time);

} // namespace tachyon::scene
