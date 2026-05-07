#pragma once

#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/resource/render_context.h"
#include "tachyon/renderer2d/evaluated_composition/render_intent.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <optional>

namespace tachyon::renderer2d {

/**
 * @brief Interface for decoupled layer rendering.
 * Implement this interface to add support for new layer types (e.g., text, solid, video)
 * without modifying the core renderer logic.
 */
class ILayerRenderer {
public:
    virtual ~ILayerRenderer() = default;

    /**
     * @brief Renders the specific layer type to the target surface.
     * @param layer The evaluated state of the layer.
     * @param state The state of the parent composition.
     * @param context The 2D rendering context (fonts, caches, etc.).
     * @param target_rect Optional global bounding rect for rendering normalization.
     * @param surface The target surface to render onto.
     * @return true if rendering was successful, false otherwise.
     */
    virtual bool render(
        const scene::EvaluatedLayerState& layer,
        const scene::EvaluatedCompositionState& state,
        const RenderIntent& intent,
        RenderContext2D& context,
        const std::optional<RectI>& target_rect,
        std::shared_ptr<SurfaceRGBA>& surface) const = 0;
};

/**
 * @brief Registry for dynamically instantiating layer renderers based on layer type.
 */
class LayerRendererRegistry {
public:
    static LayerRendererRegistry& get() {
        static LayerRendererRegistry instance;
        return instance;
    }

    void register_renderer(scene::LayerType layer_type, std::unique_ptr<ILayerRenderer> renderer) {
        m_renderers[layer_type] = std::move(renderer);
    }

    const ILayerRenderer* get_renderer(scene::LayerType layer_type) const {
        auto it = m_renderers.find(layer_type);
        return it != m_renderers.end() ? it->second.get() : nullptr;
    }

private:
    LayerRendererRegistry() = default;
    std::unordered_map<scene::LayerType, std::unique_ptr<ILayerRenderer>> m_renderers;
};

} // namespace tachyon::renderer2d
