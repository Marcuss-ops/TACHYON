#include "tachyon/media/resolution/asset_resolution.h"
#include "tachyon/media/resolution/asset_path_utils.h"
#include "tachyon/text/fonts/management/font_manifest.h"

#include <algorithm>

namespace tachyon {

namespace {

void add_entry(AssetReport& report, const std::string& type, const std::filesystem::path& full_path) {
    AssetReport::Entry entry;
    entry.type = type;
    entry.path = full_path;
    entry.exists = std::filesystem::exists(full_path);
    if (!entry.exists) {
        entry.error = "Asset not found: " + full_path.string();
    }
    report.entries.push_back(std::move(entry));

    if (type == "image") {
        report.image_count++;
    } else if (type == "video") {
        report.video_count++;
    } else if (type == "audio") {
        report.audio_count++;
    } else if (type == "font") {
        report.font_count++;
    } else if (type == "data_source") {
        report.data_source_count++;
    } else if (type == "subtitle") {
        report.subtitle_count++;
    } else {
        report.unknown_count++;
    }
}

} // anonymous namespace

ResolutionResult<AssetResolutionTable> resolve_assets(const SceneSpec& scene, const std::filesystem::path& root_dir) {
    ResolutionResult<AssetResolutionTable> result;
    AssetResolutionTable table;

    for (const auto& asset : scene.assets) {
        ResolvedAsset resolved;
        resolved.asset_id = asset.id;
        resolved.type = asset.type;

        auto source_path = media::asset_source_or_path(asset);
        resolved.absolute_path = media::resolve_relative_to_root(source_path, root_dir);
        resolved.exists = std::filesystem::exists(resolved.absolute_path);

        if (!resolved.exists) {
            result.diagnostics.add_error(
                "asset.missing",
                "asset file not found on disk: " + resolved.absolute_path.string(),
                "assets[" + asset.id + "].source"
            );
        }

        table[asset.id] = std::move(resolved);
    }

    result.value = std::move(table);
    return result;
}

AssetReport build_asset_report(const SceneSpec& scene, const std::filesystem::path& root) {
    AssetReport report;

    // Gap 1 already fixed in resolve_assets; for report, scan all assets
    for (const auto& asset : scene.assets) {
        auto source_path = media::asset_source_or_path(asset);
        auto full_path = media::resolve_relative_to_root(source_path, root);
        add_entry(report, asset.type, full_path);
    }

    // Gap 4: check data sources
    for (const auto& ds : scene.data_sources) {
        auto full_path = media::resolve_relative_to_root(std::filesystem::path(ds.path), root);
        add_entry(report, "data_source", full_path);
    }

    // Gap 2: check fonts from font_manifest
    if (scene.font_manifest.has_value()) {
        for (const auto& font : scene.font_manifest->fonts) {
            auto full_path = media::resolve_relative_to_root(font.src, root);
            add_entry(report, "font", full_path);
        }
    }

    // Gap 3 & 4: check audio tracks in compositions and subtitle paths
    for (const auto& comp : scene.compositions) {
        for (const auto& track : comp.audio_tracks) {
            auto full_path = media::resolve_relative_to_root(track.source_path, root);
            add_entry(report, "audio", full_path);
        }
        for (const auto& layer : comp.layers) {
            if (!layer.subtitle_path.empty()) {
                auto full_path = media::resolve_relative_to_root(layer.subtitle_path, root);
                add_entry(report, "subtitle", full_path);
            }
        }
    }

    report.all_present = std::all_of(report.entries.begin(), report.entries.end(),
                                     [](const auto& e) { return e.exists; });

    return report;
}

} // namespace tachyon

