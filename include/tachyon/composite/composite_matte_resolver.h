#pragma once

#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/composite/matte_types.h"
#include <vector>

namespace tachyon::composite {

/**
 * @brief Resolves layer mattes during the compositing pass.
 */
class CompositeMatteResolver {
public:
    /**
     * @brief Resolve and apply mattes for a set of layers.
     * 
     * Modifies the alpha channel of target layers based on their source matte layers.
     */
    void resolve_mattes(
        const std::vector<scene::EvaluatedLayerState>& layers,
        std::vector<float*>& layer_alpha_buffers, // Pointers to alpha channels in memory
        int width, 
        int height);

private:
    void apply_alpha_matte(float* target, const float* source, int size, bool inverted);
    void apply_luma_matte(float* target, const float* source_rgba, int size, bool inverted);
};

} // namespace tachyon::composite