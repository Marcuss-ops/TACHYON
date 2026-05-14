#include "tachyon/renderer2d/evaluated_composition/rendering/pipeline/pipeline_helpers.h"
#include "tachyon/media/management/asset_resolver.h"

namespace tachyon {

std::optional<std::filesystem::path> resolve_media_source(
    const scene::EvaluatedLayerState& layer,
    const RenderContext& context) {
    
    if (layer.identity.type == LayerType::Image || layer.identity.type == LayerType::Video) {
        if (context.asset_resolver && layer.source.asset_path().has_value()) {
            const media::AssetType type = (layer.identity.type == LayerType::Video) ? media::AssetType::VIDEO : media::AssetType::IMAGE;
            return context.asset_resolver->resolve_path(*layer.source.asset_path(), type);
        }
    }
    
    return std::nullopt;
}

} // namespace tachyon
