#include "tachyon/core/assets/asset_resolution.h"
#include "tachyon/core/assets/asset_collector.h"
#include "tachyon/core/media/asset_resolver_interface.h"
#include <filesystem>
#include <algorithm>

namespace tachyon::core::assets {

ResolutionResult<AssetResolutionTable> resolve_assets(const SceneSpec& scene, const ::tachyon::media::IAssetResolver& resolver) {
    ResolutionResult<AssetResolutionTable> result;
    AssetResolutionTable table;

    auto refs = collect_asset_references(scene);
    for (const auto& ref : refs) {
        ::tachyon::media::ResolvedAsset resolved;
        resolved.id = ref.id;
        resolved.type_name = (ref.kind == ::tachyon::media::AssetKind::Audio) ? "audio" : 
                            (ref.kind == ::tachyon::media::AssetKind::Video) ? "video" : "image";

        ::tachyon::media::AssetType media_type = ::tachyon::media::AssetType::IMAGE;
        if (ref.kind == ::tachyon::media::AssetKind::Audio) media_type = ::tachyon::media::AssetType::AUDIO;
        else if (ref.kind == ::tachyon::media::AssetKind::Font) media_type = ::tachyon::media::AssetType::FONT;
        else if (ref.kind == ::tachyon::media::AssetKind::Video) media_type = ::tachyon::media::AssetType::VIDEO;

        auto path = resolver.resolve_path(ref.source, media_type);
        if (path) {
            resolved.source_path = *path;
            resolved.runtime_path = *path;
            resolved.exists = true;
        } else {
            resolved.source_path = ref.source;
            resolved.runtime_path = ref.source;
            resolved.exists = false;
            
            result.diagnostics.add_error(
                "asset.missing",
                "asset file not found on disk: " + ref.source,
                ref.owner_id
            );
        }

        if (!ref.id.empty()) {
            table[ref.id] = std::move(resolved);
        }
    }

    result.value = std::move(table);
    return result;
}

AssetReport build_asset_report(const SceneSpec& scene, const ::tachyon::media::IAssetResolver& resolver) {
    AssetReport report;
    auto refs = collect_asset_references(scene);

    for (const auto& ref : refs) {
        AssetReport::Entry entry;
        entry.type = (ref.kind == ::tachyon::media::AssetKind::Image) ? "image" :
                     (ref.kind == ::tachyon::media::AssetKind::Video) ? "video" :
                     (ref.kind == ::tachyon::media::AssetKind::Audio) ? "audio" :
                     (ref.kind == ::tachyon::media::AssetKind::Font) ? "font" :
                     (ref.kind == ::tachyon::media::AssetKind::DataSource) ? "data_source" :
                     (ref.kind == ::tachyon::media::AssetKind::Subtitle) ? "subtitle" : "unknown";

        ::tachyon::media::AssetType media_type = ::tachyon::media::AssetType::IMAGE;
        if (ref.kind == ::tachyon::media::AssetKind::Audio) media_type = ::tachyon::media::AssetType::AUDIO;
        else if (ref.kind == ::tachyon::media::AssetKind::Font) media_type = ::tachyon::media::AssetType::FONT;

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

        if (ref.kind == ::tachyon::media::AssetKind::Image) report.image_count++;
        else if (ref.kind == ::tachyon::media::AssetKind::Video) report.video_count++;
        else if (ref.kind == ::tachyon::media::AssetKind::Audio) report.audio_count++;
        else if (ref.kind == ::tachyon::media::AssetKind::Font) report.font_count++;
        else if (ref.kind == ::tachyon::media::AssetKind::DataSource) report.data_source_count++;
        else if (ref.kind == ::tachyon::media::AssetKind::Subtitle) report.subtitle_count++;
        else report.unknown_count++;
    }

    report.all_present = std::all_of(report.entries.begin(), report.entries.end(),
                                     [](const auto& e) { return e.exists; });

    return report;
}

} // namespace tachyon::core::assets
