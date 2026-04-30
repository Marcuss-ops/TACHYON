#pragma once

#include "tachyon/media/loading/mesh_asset.h"
#include "tachyon/core/spec/schema/common/common_spec.h"
#include <string>
#include <memory>

namespace tachyon::scene {

LayerType map_layer_type(const std::string& type);
std::shared_ptr<::tachyon::media::MeshAsset> create_quad_mesh(float width, float height);

} // namespace tachyon::scene
