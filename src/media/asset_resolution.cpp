#include "tachyon/media/resolution/asset_resolution.h"

namespace tachyon {

ResolutionResult<AssetResolutionTable> resolve_assets(const SceneSpec& scene, const std::filesystem::path& root_dir) {
    ResolutionResult<AssetResolutionTable> result;
    AssetResolutionTable table;

    for (const auto& asset : scene.assets) {
        ResolvedAsset resolved;
        resolved.asset_id = asset.id;
        resolved.type = asset.type;
        
        const std::filesystem::path source_value = asset.source.empty() ? std::filesystem::path(asset.path) : std::filesystem::path(asset.source);
        std::filesystem::path source_path(source_value);
        if (source_path.is_relative()) {
            resolved.absolute_path = std::filesystem::absolute(root_dir / source_path);
        } else {
            resolved.absolute_path = source_path;
        }

        resolved.exists = std::filesystem::exists(resolved.absolute_path);

        if (!resolved.exists) {
            result.diagnostics.add_error(
                "asset.missing",
                "asset file not found on disk: " + resolved.absolute_path.string(),
                "assets[" + asset.id + "].source"
            );
        } else {
            table[asset.id] = std::move(resolved);
        }
    }

    if (result.diagnostics.ok()) {
        result.value = std::move(table);
    }

    return result;
}

AssetReport build_asset_report(const SceneSpec& scene, const std::filesystem::path& root) {
    AssetReport report;

    for (const auto& asset : scene.assets) {
        AssetReport::Entry entry;
        entry.type = asset.type;

        const std::filesystem::path source_value = asset.source.empty() ? std::filesystem::path(asset.path) : std::filesystem::path(asset.source);
        std::filesystem::path full_path = source_value;
        if (full_path.is_relative()) {
            full_path = std::filesystem::absolute(root / full_path);
        }
        entry.path = full_path;

        entry.exists = std::filesystem::exists(full_path);
        if (!entry.exists) {
            entry.error = "Asset not found: " + full_path.string();
        }

        report.entries.push_back(std::move(entry));

        if (asset.type == "image") {
            report.image_count++;
        } else if (asset.type == "video") {
            report.video_count++;
        } else if (asset.type == "audio") {
            report.audio_count++;
        } else if (asset.type == "font") {
            report.font_count++;
        }
    }

    report.all_present = std::all_of(report.entries.begin(), report.entries.end(),
                                     [](const auto& e) { return e.exists; });

    return report;
}

} // namespace tachyon
