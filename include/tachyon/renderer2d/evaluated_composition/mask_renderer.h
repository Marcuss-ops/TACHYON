#pragma once

#include "tachyon/renderer2d/raster/rasterizer.h"
#include "tachyon/renderer2d/resource/render_context.h"
#include "tachyon/core/scene/state/evaluated_state.h"

#include <vector>

namespace tachyon {
namespace renderer2d {

struct RenderContext2D;

class MaskRenderer {
public:
    /**
     * @brief Apply mask stack from a single evaluated frame state.
     *
     * This is the standard path for non-motion-blurred rendering.
     */
    static void applyMask(
        const scene::EvaluatedCompositionState& state,
        RenderContext2D& context,
        std::vector<float>& accum_r,
        std::vector<float>& accum_g,
        std::vector<float>& accum_b,
        std::vector<float>& accum_a);

    /**
     * @brief Apply mask stack with motion-blur accumulation.
     *
     * When motion blur is enabled, the engine evaluates the scene at
     * multiple sub-frame times.  This variant takes an array of
     * evaluated states (one per sub-frame sample), renders the mask
     * for each, and accumulates the alpha with equal weight.
     *
     * The mask geometry (path vertices, feather) is interpolated at
     * each sub-frame time by the frame evaluator before this call.
     *
     * @param states    One EvaluatedCompositionState per sub-frame sample.
     * @param context   Render context (resolution, policy).
     * @param accum_r   Accumulated red channel (modified in-place).
     * @param accum_g   Accumulated green channel.
     * @param accum_b   Accumulated blue channel.
     * @param accum_a   Accumulated alpha channel.
     * @param sample_count Number of samples (for weighting).
     */
    static void applyMaskMotionBlur(
        const std::vector<const scene::EvaluatedCompositionState*>& states,
        RenderContext2D& context,
        std::vector<float>& accum_r,
        std::vector<float>& accum_g,
        std::vector<float>& accum_b,
        std::vector<float>& accum_a,
        int sample_count);

private:
    static void computeMaskBounds(
        const scene::EvaluatedLayerState& layer_state,
        int& x0, int& y0, int& x1, int& y1);
};

} // namespace renderer2d
} // namespace tachyon