#include "tachyon/media/resolution/asset_collector.h"
#include "tachyon/media/resolution/asset_path_utils.h"
#include "tachyon/text/fonts/management/font_manifest.h"

namespace tachyon::media {

std::vector<AssetReference> collect_asset_references(const SceneSpec& scene) {
    std::vector<AssetReference> refs;

    // 1. Scene Assets (Image/Video)
    for (const auto& asset : scene.assets) {
        AssetReference ref;
        ref.id = asset.id;
        ref.source = asset_source_or_path(asset).string();
        ref.kind = (asset.type == "audio") ? AssetKind::Audio : 
                   (asset.type == "video") ? AssetKind::Video : AssetKind::Image;
        ref.owner_id = "scene.assets";
        refs.push_back(std::move(ref));
    }

    // 2. Data Sources
    for (const auto& ds : scene.data_sources) {
        AssetReference ref;
        ref.id = ds.id;
        ref.source = ds.path;
        ref.kind = AssetKind::DataSource;
        ref.owner_id = "scene.data_sources";
        refs.push_back(std::move(ref));
    }

    // 3. Font Manifest
    if (scene.font_manifest) {
        for (const auto& font : scene.font_manifest->fonts) {
            AssetReference ref;
            ref.id = font.id;
            ref.source = font.src.string();
            ref.kind = AssetKind::Font;
            ref.owner_id = "scene.font_manifest";
            refs.push_back(std::move(ref));
        }
    }

    // 4. Compositions (Audio Tracks & Subtitles)
    for (const auto& comp : scene.compositions) {
        for (const auto& track : comp.audio_tracks) {
            AssetReference ref;
            ref.id = track.id;
            ref.source = track.source_path;
            ref.kind = AssetKind::Audio;
            ref.owner_id = "composition." + comp.id + ".audio_tracks";
            refs.push_back(std::move(ref));
        }

        for (const auto& layer : comp.layers) {
            if (!layer.subtitle_path.empty()) {
                AssetReference ref;
                ref.source = layer.subtitle_path;
                ref.kind = AssetKind::Subtitle;
                ref.owner_id = "layer." + layer.id;
                refs.push_back(std::move(ref));
            }
            // If it's a text layer, it might have a font_id
            if (layer.type == LayerType::Text && !layer.font_id.empty()) {
                AssetReference ref;
                ref.source = layer.font_id; // Usually a name, not a path, but still a dependency
                ref.kind = AssetKind::Font;
                ref.owner_id = "layer." + layer.id;
                refs.push_back(std::move(ref));
            }
        }
    }

    return refs;
}

} // namespace tachyon::media
