#include "tachyon/composite/composite_matte_resolver.h"
#include <algorithm>

namespace tachyon::composite {

void CompositeMatteResolver::resolve_mattes(
    const std::vector<scene::EvaluatedLayerState>& layers,
    std::vector<float*>& layer_alpha_buffers,
    int width, 
    int height) {
    
    const int size = width * height;
    
    for (std::size_t i = 0; i < layers.size(); ++i) {
        const auto& layer = layers[i];
        if (!layer.track_matte_layer_index.has_value()) continue;
        
        std::size_t source_idx = *layer.track_matte_layer_index;
        if (source_idx >= layer_alpha_buffers.size()) continue;
        
        float* target_alpha = layer_alpha_buffers[i];
        float* source_alpha = layer_alpha_buffers[source_idx];
        
        // We assume MatteMode::Alpha for now as per EvaluatedLayerState definition
        // In a full implementation, we would check the mode from the spec
        apply_alpha_matte(target_alpha, source_alpha, size, false);
    }
}

void CompositeMatteResolver::apply_alpha_matte(float* target, const float* source, int size, bool inverted) {
    if (inverted) {
        for (int i = 0; i < size; ++i) {
            target[i] *= (1.0f - source[i]);
        }
    } else {
        for (int i = 0; i < size; ++i) {
            target[i] *= source[i];
        }
    }
}

void CompositeMatteResolver::apply_luma_matte(float* target, const float* source_rgba, int size, bool inverted) {
    // Luma = 0.2126*R + 0.7152*G + 0.0722*B
    for (int i = 0; i < size; ++i) {
        float r = source_rgba[i * 4 + 0];
        float g = source_rgba[i * 4 + 1];
        float b = source_rgba[i * 4 + 2];
        float luma = 0.2126f * r + 0.7152f * g + 0.0722f * b;
        
        if (inverted) luma = 1.0f - luma;
        target[i] *= luma;
    }
}

} // namespace tachyon::composite