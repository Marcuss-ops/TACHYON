#pragma once

#include "tachyon/render/intent_builder.h"
#include "tachyon/renderer2d/resource/render_context.h"
#include <cstdint>

namespace tachyon::renderer2d {

/**
 * @brief Implementation of IResourceProvider for the 2D renderer.
 */
class RendererResourceProvider : public render::IResourceProvider {
public:
    explicit RendererResourceProvider(RenderContext2D& context) : m_context(context) {}
    
    std::shared_ptr<::tachyon::media::MeshAsset> get_mesh(const std::string& id) override;
    std::shared_ptr<std::uint8_t[]> get_texture_rgba(const std::string& id) override;
    std::shared_ptr<const render::IDeformMesh> get_deform(const std::string& id) override;

private:
    RenderContext2D& m_context;
};

} // namespace tachyon::renderer2d
