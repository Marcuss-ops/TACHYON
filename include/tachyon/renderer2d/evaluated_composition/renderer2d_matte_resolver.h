#pragma once

#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/core/spec/schema/contracts/shared_contracts.h"
#include <vector>
#include <string>
#include <cstddef>
#include <unordered_map>
#include <memory>

namespace tachyon::renderer2d {

/**
 * @brief Matte resolution mode for a layer.
 */
using MatteMode = spec::MatteMode;

/**
 * @brief A matte dependency edge in the render graph.
 */
using MatteDependency = spec::MatteDependency;

/**
 * @brief Resolves matte dependencies before compositing.
 * 
 * This is a deterministic pass over the evaluated layer stack.
 * It produces a matte alpha buffer per layer that the compositor uses.
 * 
 * Rules:
 * - Matte sources are resolved before their targets.
 * - A matte source that is itself invisible produces a zero matte.
 * - Luma mattes are computed from the source's RGB in linear space.
 * - The result is a single float per pixel per target layer.
 * 
 * No global state. All inputs are explicit.
 */
class Renderer2DMatteResolver {
public:
    /**
     * @brief Resolve all matte dependencies for a composition.
     * 
     * @param layers Evaluated layer stack in compositing order.
     * @param dependencies Matte dependency edges. Must be validated (no cycles, no missing IDs).
     * @param rendered_surfaces Map of layer ID to rendered surface (RGBA).
     *                          The source layer's actual rendered output is used for matte extraction.
     * @param out_matte_buffers Output: one buffer per layer, same dimensions as composition.
     *                        Buffer i corresponds to layers[i].
     *                        If a layer has no matte dependency, its buffer is all 1.0f.
     * @param width Composition width in pixels.
     * @param height Composition height in pixels.
     */
    void resolve(
        const std::vector<scene::EvaluatedLayerState>& layers,
        const std::vector<MatteDependency>& dependencies,
        const std::unordered_map<std::string, std::shared_ptr<SurfaceRGBA>>& rendered_surfaces,
        std::vector<std::vector<float>>& out_matte_buffers,
        std::int64_t width,
        std::int64_t height) const;
    
    /**
     * @brief Validate matte dependencies.
     * 
     * Checks:
     * - No missing source or target layer IDs.
     * - No cyclic dependencies (A mattes B, B mattes C, C mattes A).
     * - No self-matting (A mattes A).
     * 
     * Returns true if valid. If invalid, fills out_error with a description.
     */
    [[nodiscard]] bool validate(
        const std::vector<scene::EvaluatedLayerState>& layers,
        const std::vector<MatteDependency>& dependencies,
        std::string& out_error) const;

private:
    void resolve_luma_matte(
        const std::vector<float>& source_r,
        const std::vector<float>& source_g,
        const std::vector<float>& source_b,
        std::vector<float>& out_matte,
        bool invert) const;
        
    void resolve_alpha_matte(
        const std::vector<float>& source_a,
        std::vector<float>& out_matte,
        bool invert) const;
};

using MatteResolver = Renderer2DMatteResolver;

} // namespace tachyon::renderer2d
