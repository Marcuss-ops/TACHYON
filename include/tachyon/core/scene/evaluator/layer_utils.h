#pragma once

#include "tachyon/core/api.h"
#include "tachyon/media/loading/mesh_asset.h"
#include "tachyon/core/spec/schema/common/common_spec.h"
#include <string>
#include <memory>

namespace tachyon::scene {

TACHYON_API LayerType map_layer_type(const std::string& type);
TACHYON_API std::shared_ptr<::tachyon::media::MeshAsset> create_quad_mesh(float width, float height);

} // namespace tachyon::scene
