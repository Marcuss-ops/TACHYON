#pragma once

#include "tachyon/media/mesh_asset.h"
#include "tachyon/core/scene/evaluated_state.h"
#include <memory>

namespace tachyon::media {

class Extruder {
public:
    /**
     * Generates a 3D SubMesh from a 2D EvaluatedShapePath.
     * Uses earcut for triangulation and constructs front/back caps and side walls.
     */
    static MeshAsset::SubMesh extrude_shape(
        const scene::EvaluatedShapePath& path, 
        float depth, 
        float bevel_size);
};

} // namespace tachyon::media
