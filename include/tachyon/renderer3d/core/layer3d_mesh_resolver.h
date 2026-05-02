#pragma once

#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/media/management/media_manager.h"

#include <memory>

namespace tachyon {
namespace renderer3d {

class Layer3DMeshResolver {
public:
    explicit Layer3DMeshResolver(media::MediaManager& media_manager);
    
    [[nodiscard]] std::unique_ptr<media::MeshAsset> resolve(const LayerSpec& layer) const;

private:
    media::MediaManager& media_manager_;
};

} // namespace renderer3d
} // namespace tachyon
