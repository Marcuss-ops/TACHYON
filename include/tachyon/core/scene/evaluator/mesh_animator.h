#pragma once

#include "tachyon/core/scene/state/evaluated_state.h"

namespace tachyon::media { struct MeshAsset; }

namespace tachyon::scene {

void evaluate_mesh_animations(EvaluatedLayerState& evaluated, const tachyon::media::MeshAsset* asset, double time);

} // namespace tachyon::scene
