#include "tachyon/core/assets/asset_collector.h"
#include "tachyon/core/spec/schema/objects/composition_spec.h"
#include <variant>

namespace tachyon::core::assets {

std::vector<AssetReference> collect_asset_references(const SceneSpec& scene) {
    std::vector<AssetReference> refs;

    for (const auto& comp : scene.compositions) {
        for (const auto& layer : comp.layers) {
            // Use std::visit to handle different source types
            std::visit([&](auto&& source) {
                using T = std::decay_t<decltype(source)>;
                if constexpr (std::is_same_v<T, MediaSource>) {
                    if (!source.asset.id.empty()) {
                        AssetReference ref;
                        ref.id = layer.identity.id;
                        ref.source = source.asset.id;
                        
                        // Heuristic based on layer type
                        if (layer.identity.type == LayerType::Video) {
                            ref.kind = ::tachyon::media::AssetKind::Video;
                        } else if (layer.identity.type == LayerType::Image) {
                            ref.kind = ::tachyon::media::AssetKind::Image;
                        } else {
                            // Fallback to extension heuristic if type is ambiguous
                            ref.kind = (source.asset.id.find(".mp4") != std::string::npos || 
                                       source.asset.id.find(".mov") != std::string::npos) 
                                       ? ::tachyon::media::AssetKind::Video 
                                       : ::tachyon::media::AssetKind::Image;
                        }
                        
                        ref.owner_id = comp.id;
                        refs.push_back(std::move(ref));
                    }
                }
            }, layer.source);
        }

        // Check for audio tracks
        for (const auto& track : comp.audio_tracks) {
            if (!track.source_path.empty()) {
                AssetReference ref;
                ref.id = track.id;
                ref.source = track.source_path;
                ref.kind = ::tachyon::media::AssetKind::Audio;
                ref.owner_id = comp.id;
                refs.push_back(std::move(ref));
            }
        }
    }

    return refs;
}

} // namespace tachyon::core::assets
