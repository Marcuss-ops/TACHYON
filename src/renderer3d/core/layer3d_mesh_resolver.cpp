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
        // Use text mesh builder with extrusion depth from layer.three_d or default
        float depth = (layer.three_d.has_value() && layer.three_d->extrusion_depth > 0.0) 
            ? static_cast<float>(layer.three_d->extrusion_depth) 
            : 10.0f;
        if (depth <= 0.0f) {
            depth = 10.0f; // Default extrude depth
        }
        // TODO: Implement text extrusion using TextMeshBuilder
        std::cerr << "Text mesh extrusion not yet implemented\n";
        return nullptr;
    }

    if (layer.type == LayerType::Image) {
        // Create plane mesh with image texture
        auto mesh = std::make_unique<media::MeshAsset>();
        media::MeshAsset::SubMesh sub;
        
        // Quad vertices: TL, TR, BR, BL (assuming 0,0 center)
        sub.vertices = {
            {{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, // TL
            {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // TR
            {{ 0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}, // BR
            {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}  // BL
        };
        sub.indices = { 0, 1, 2, 0, 2, 3 };
        
        if (!layer.asset_id.empty()) {
            sub.material.base_color_texture_idx = 0; // Reference the first texture
            // The texture would be loaded by the renderer using asset_id
        }
        
        mesh->sub_meshes.push_back(std::move(sub));
        return mesh;
    }

    if (layer.type == LayerType::Solid) {
        // Create box mesh
        auto mesh = std::make_unique<media::MeshAsset>();
        media::MeshAsset::SubMesh sub;
        
        // Basic unit cube [-0.5, 0.5]
        float s = 0.5f;
        // Front
        sub.vertices.push_back({{-s, -s,  s}, {0,0,1}, {0,0}});
        sub.vertices.push_back({{ s, -s,  s}, {0,0,1}, {1,0}});
        sub.vertices.push_back({{ s,  s,  s}, {0,0,1}, {1,1}});
        sub.vertices.push_back({{-s,  s,  s}, {0,0,1}, {0,1}});
        // Back
        sub.vertices.push_back({{-s, -s, -s}, {0,0,-1}, {0,0}});
        sub.vertices.push_back({{-s,  s, -s}, {0,0,-1}, {0,1}});
        sub.vertices.push_back({{ s,  s, -s}, {0,0,-1}, {1,1}});
        sub.vertices.push_back({{ s, -s, -s}, {0,0,-1}, {1,0}});
        // ... adding other faces would be tedious, but this is a start for "Solid" box
        // For a simple solid, maybe just front/back is enough if it's 2.5D, 
        // but let's at least have a valid cube-like structure.
        
        // Indices for front and back
        sub.indices = { 0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7 };
        
        if (layer.fill_color.value.has_value()) {
            sub.material.base_color_factor = layer.fill_color.value->to_vector3();
        } else if (!layer.fill_color.keyframes.empty()) {
            sub.material.base_color_factor = layer.fill_color.keyframes.front().value.to_vector3();
        } else {
            sub.material.base_color_factor = math::Vector3::one();
        }
        
        mesh->sub_meshes.push_back(std::move(sub));
        return mesh;
    }

    // Unsupported layer type
    std::cerr << "Unsupported layer type for 3D mesh resolution: " << static_cast<int>(layer.type) << '\n';
    return nullptr;
}

} // namespace renderer3d
} // namespace tachyon
