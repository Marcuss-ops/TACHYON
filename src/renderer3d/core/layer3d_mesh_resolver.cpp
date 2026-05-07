#include "tachyon/renderer3d/core/layer3d_mesh_resolver.h"
#include "tachyon/media/loading/mesh_asset.h"
#include "tachyon/renderer2d/evaluated_composition/rendering/primitives/text_mesh_builder.h"
#include "tachyon/core/math/vector3.h"

#include <stdexcept>
#include <iostream>

namespace tachyon {
namespace renderer3d {

Layer3DMeshResolver::Layer3DMeshResolver(media::MediaManager& media_manager)
    : media_manager_(media_manager) {}

std::unique_ptr<media::MeshAsset> Layer3DMeshResolver::resolve(const LayerSpec& layer) const {
    if (layer.type == LayerType::Mesh) {
        // Load mesh from asset path
        if (layer.asset_id.empty()) {
            std::cerr << "Mesh layer missing asset_id\n";
            return nullptr;
        }
        auto mesh = media_manager_.get_mesh(layer.asset_id);
        if (!mesh) {
            std::cerr << "Failed to load mesh: " << layer.asset_id << '\n';
            return nullptr;
        }
        return std::make_unique<media::MeshAsset>(*mesh);
    }

    if (layer.type == LayerType::Text) {
        // Extrude text to mesh
        if (layer.text_content.empty()) {
            std::cerr << "Text layer missing text content\n";
            return nullptr;
        }
        // Use text mesh builder with extrusion depth from layer.extrude or default
        float depth = layer.extrude.has_value() ? static_cast<float>(layer.extrude.value()) : 10.0f;
        if (depth <= 0.0f) {
            depth = 10.0f; // Default extrude depth
        }
        // TODO: Implement text extrusion using TextMeshBuilder
        std::cerr << "Text mesh extrusion not yet implemented\n";
        return nullptr;
    }

    if (layer.type == LayerType::Image) {
        // Create plane mesh with image texture
        if (layer.asset_id.empty()) {
            std::cerr << "Image layer missing asset_id\n";
            return nullptr;
        }
        // TODO: Implement image plane mesh creation
        std::cerr << "Image mesh creation not yet implemented\n";
        return nullptr;
    }

    if (layer.type == LayerType::Solid) {
        // Create box mesh
        // TODO: Implement solid box mesh creation
        std::cerr << "Solid mesh creation not yet implemented\n";
        return nullptr;
    }

    // Unsupported layer type
    std::cerr << "Unsupported layer type for 3D mesh resolution: " << static_cast<int>(layer.type) << '\n';
    return nullptr;
}

} // namespace renderer3d
} // namespace tachyon
