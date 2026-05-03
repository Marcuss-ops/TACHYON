#include "tachyon/core/scene/evaluator/layer_utils.h"

namespace tachyon::scene {

LayerType map_layer_type(const std::string& type) {
    if (type == "solid") return LayerType::Solid;
    if (type == "shape") return LayerType::Shape;
    if (type == "mask") return LayerType::Mask;
    if (type == "image") return LayerType::Image;
    if (type == "video") return LayerType::Video;
    if (type == "text") return LayerType::Text;
    if (type == "procedural") return LayerType::Procedural;
    if (type == "camera") return LayerType::Camera;
    if (type == "precomp") return LayerType::Precomp;
    if (type == "light") return LayerType::Light;
    if (type == "null") return LayerType::NullLayer;
    return LayerType::Unknown;
}

std::shared_ptr<::tachyon::media::MeshAsset> create_quad_mesh(float width, float height) {
    auto mesh = std::make_shared<::tachyon::media::MeshAsset>();
    ::tachyon::media::MeshAsset::SubMesh sub;
    
    float w2 = width * 0.5f;
    float h2 = height * 0.5f;

    // Standard AE centered quad: anchor at [width/2, height/2]
    // Here we define vertices relative to anchor point [0,0] in object space
    // and let the world matrix handle placement.
    sub.vertices = {
        { {-w2, -h2, 0.0f}, {0,0,-1}, {0,0} },
        { { w2, -h2, 0.0f}, {0,0,-1}, {1,0} },
        { { w2,  h2, 0.0f}, {0,0,-1}, {1,1} },
        { {-w2,  h2, 0.0f}, {0,0,-1}, {0,1} }
    };
    sub.indices = { 0, 3, 2, 0, 2, 1 };
    mesh->sub_meshes.push_back(std::move(sub));
    return mesh;
}

} // namespace tachyon::scene
