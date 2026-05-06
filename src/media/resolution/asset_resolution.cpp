#include "tachyon/media/resolution/asset_resolution.h"
#include "tachyon/media/resolution/asset_collector.h"
#include "tachyon/media/resolution/project_context.h"
#include "tachyon/media/management/asset_resolver.h"

#include <algorithm>

namespace tachyon {

ResolutionResult<AssetResolutionTable> resolve_assets(const SceneSpec& scene, const std::filesystem::path& root_dir) {
    ResolutionResult<AssetResolutionTable> result;
    AssetResolutionTable table;

    // Use AssetResolver for consistent logic
    media::AssetResolver::Config config;
    config.project_root = root_dir;
    config.assets_root = root_dir;
    config.fonts_root = root_dir / "fonts";
    config.sfx_root = root_dir / "audio";

    media::AssetResolver resolver(config);
    
    auto refs = media::collect_asset_references(scene);
    for (const auto& ref : refs) {
        ResolvedAsset resolved;
        resolved.asset_id = ref.id;
        resolved.type = (ref.kind == media::AssetKind::Audio) ? "audio" : 
                        (ref.kind == media::AssetKind::Video) ? "video" : "image";

        media::AssetType media_type = media::AssetType::IMAGE;
        if (ref.kind == media::AssetKind::Audio) media_type = media::AssetType::AUDIO;
        else if (ref.kind == media::AssetKind::Font) media_type = media::AssetType::FONT;

        auto path = resolver.resolve_path(ref.source, media_type);
        if (path) {
            resolved.absolute_path = *path;
            resolved.exists = true;
        } else {
            resolved.absolute_path = ref.source; // Fallback to raw string
            resolved.exists = false;
            
            result.diagnostics.add_error(
                "asset.missing",
                "asset file not found on disk: " + ref.source,
                ref.owner_id
            );
        }

        // Only add to table if it has an ID
        if (!ref.id.empty()) {
            table[ref.id] = std::move(resolved);
        }
    }

    result.value = std::move(table);
    return result;
}

AssetReport build_asset_report(const SceneSpec& scene, const std::filesystem::path& root) {
    AssetReport report;

    media::AssetResolver::Config config;
    config.project_root = root;
    config.assets_root = root;
    config.fonts_root = root / "fonts";
    config.sfx_root = root / "audio";

    media::AssetResolver resolver(config);
    auto refs = media::collect_asset_references(scene);

    for (const auto& ref : refs) {
        AssetReport::Entry entry;
        entry.type = (ref.kind == media::AssetKind::Image) ? "image" :
                     (ref.kind == media::AssetKind::Video) ? "video" :
                     (ref.kind == media::AssetKind::Audio) ? "audio" :
                     (ref.kind == media::AssetKind::Font) ? "font" :
                     (ref.kind == media::AssetKind::DataSource) ? "data_source" :
                     (ref.kind == media::AssetKind::Subtitle) ? "subtitle" : "unknown";

        media::AssetType media_type = media::AssetType::IMAGE;
        if (ref.kind == media::AssetKind::Audio) media_type = media::AssetType::AUDIO;
        else if (ref.kind == media::AssetKind::Font) media_type = media::AssetType::FONT;

        auto path = resolver.resolve_path(ref.source, media_type);
        if (path) {
            entry.path = *path;
            entry.exists = true;
        } else {
            entry.path = ref.source;
            entry.exists = false;
            entry.error = "Asset not found: " + ref.source;
        }

        report.entries.push_back(std::move(entry));

        if (ref.kind == media::AssetKind::Image) report.image_count++;
        else if (ref.kind == media::AssetKind::Video) report.video_count++;
        else if (ref.kind == media::AssetKind::Audio) report.audio_count++;
        else if (ref.kind == media::AssetKind::Font) report.font_count++;
        else if (ref.kind == media::AssetKind::DataSource) report.data_source_count++;
        else if (ref.kind == media::AssetKind::Subtitle) report.subtitle_count++;
        else report.unknown_count++;
    }

    report.all_present = std::all_of(report.entries.begin(), report.entries.end(),
                                     [](const auto& e) { return e.exists; });

    return report;
}

} // namespace tachyon
