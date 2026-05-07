#include "tachyon/renderer2d/resource/resource_provider.h"
#include "tachyon/media/management/media_manager.h"
#include <algorithm>

namespace tachyon::renderer2d {

std::shared_ptr<::tachyon::media::MeshAsset> RendererResourceProvider::get_mesh(const std::string& id) {
    if (!m_context.media_manager) return nullptr;
    
    // Convert const shared_ptr<const MeshAsset> to shared_ptr<MeshAsset> 
    // This is safe here because the renderer is allowed to observe but we might need non-const
    // actually RenderIntent uses shared_ptr<MeshAsset>.
    auto const_mesh = m_context.media_manager->get_mesh(id);
    return std::const_pointer_cast<::tachyon::media::MeshAsset>(const_mesh);
}

std::shared_ptr<std::uint8_t[]> RendererResourceProvider::get_texture_rgba(const std::string& id) {
    if (!m_context.media_manager) return nullptr;
    
    auto img = m_context.media_manager->get_image(id);
    if (!img) {
        return nullptr;
    }
    
    size_t width = img->width();
    size_t height = img->height();
    size_t size = width * height * 4;
    std::shared_ptr<std::uint8_t[]> data(new std::uint8_t[size]);
    
    const auto& pixels = img->pixels();
    for (size_t i = 0; i < width * height; ++i) {
        // Simple float to 8-bit conversion (assuming 0-1 range)
        data[i * 4 + 0] = static_cast<std::uint8_t>(std::clamp(pixels[i * 4 + 0] * 255.0f, 0.0f, 255.0f));
        data[i * 4 + 1] = static_cast<std::uint8_t>(std::clamp(pixels[i * 4 + 1] * 255.0f, 0.0f, 255.0f));
        data[i * 4 + 2] = static_cast<std::uint8_t>(std::clamp(pixels[i * 4 + 2] * 255.0f, 0.0f, 255.0f));
        data[i * 4 + 3] = static_cast<std::uint8_t>(std::clamp(pixels[i * 4 + 3] * 255.0f, 0.0f, 255.0f));
    }
    
    return data;
}

std::shared_ptr<const render::IDeformMesh> RendererResourceProvider::get_deform(const std::string& id) {
    // Implement when DeformMesh registry is available
    return nullptr;
}

} // namespace tachyon::renderer2d
