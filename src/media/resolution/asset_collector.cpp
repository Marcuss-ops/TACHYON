#include "tachyon/media/resolution/asset_collector.h"
#include "tachyon/core/spec/schema/objects/composition_spec.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"

namespace tachyon::media {

std::vector<AssetReference> collect_asset_references(const SceneSpec& scene) {
    std::vector<AssetReference> refs;

    for (const auto& comp : scene.compositions) {
        // Collect background assets
        if (comp.background.has_value()) {
            if (comp.background->type == BackgroundType::Asset) {
                refs.push_back({"", comp.background->value, AssetKind::Image, comp.id});
            }
        }

        for (const auto& layer : comp.layers) {
            // Source-based assets
            if (auto* media = std::get_if<MediaSource>(&layer.source)) {
                if (layer.identity.type == LayerType::Image) {
                    refs.push_back({media->asset_path, media->asset_path, AssetKind::Image, layer.identity.id});
                } else if (layer.identity.type == LayerType::Video) {
                    refs.push_back({media->asset_path, media->asset_path, AssetKind::Video, layer.identity.id});
                }
            }
            
            // Text layers (fonts)
            if (layer.identity.type == LayerType::Text) {
                if (!layer.text.font_id.empty()) {
                    refs.push_back({"", layer.text.font_id, AssetKind::Font, layer.identity.id});
                }
            }
            
            // Subtitles
            if (!layer.subtitles.path.empty()) {
                refs.push_back({"", layer.subtitles.path, AssetKind::Subtitle, layer.identity.id});
            }
        }

        // SFX (if any)
        for (const auto& audio : comp.audio_tracks) {
            if (!audio.source_path.empty()) {
                refs.push_back({"", audio.source_path, AssetKind::Audio, comp.id});
            }
        }
    }

    return refs;
}

} // namespace tachyon::media
