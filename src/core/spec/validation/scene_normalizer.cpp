#include "tachyon/core/spec/validation/scene_normalizer.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/spec/schema/objects/composition_spec.h"

namespace tachyon::core {

void SceneNormalizer::normalize(SceneSpec& scene) {
    for (auto& comp : scene.compositions) {
        for (auto& layer : comp.layers) {
            // 1. Ensure duration is consistent with source_in/out if needed
            // Currently, builders set these directly.
            
            // 2. Placeholder for future normalization logic
        }
    }
}

} // namespace tachyon::core
