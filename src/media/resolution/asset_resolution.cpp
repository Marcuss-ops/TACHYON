#include "tachyon/media/resolution/asset_resolution.h"
#include "tachyon/text/fonts/management/font_manifest.h"

#include <algorithm>

namespace tachyon {

namespace {

std::filesystem::path resolve_path(const std::string& path_str, const std::filesystem::path& root) {
    std::filesystem::path p(path_str);
    if (p.is_relative()) {
        return std::filesystem::absolute(root / p);
    }
    return p;
}

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

        std::filesystem::path source_value = asset.source.empty() ? std::filesystem::path(asset.path) : std::filesystem::path(asset.source);
        if (source_value.is_relative()) {
            resolved.absolute_path = std::filesystem::absolute(root_dir / source_value);
        } else {
            resolved.absolute_path = source_value;
        }

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

    if (result.diagnostics.ok()) {
        result.value = std::move(table);
    }
    return result;
}

AssetReport build_asset_report(const SceneSpec& scene, const std::filesystem::path& root) {
    AssetReport report;

    // Gap 1 already fixed in resolve_assets; for report, scan all assets
    for (const auto& asset : scene.assets) {
        std::filesystem::path source_value = asset.source.empty() ? std::filesystem::path(asset.path) : std::filesystem::path(asset.source);
        std::filesystem::path full_path = source_value;
        if (full_path.is_relative()) {
            full_path = std::filesystem::absolute(root / full_path);
        }
        add_entry(report, asset.type, full_path);
    }

    // Gap 4: check data sources
    for (const auto& ds : scene.data_sources) {
        std::filesystem::path full_path = resolve_path(ds.path, root);
        add_entry(report, "data_source", full_path);
    }

    // Gap 2: check fonts from font_manifest
    if (scene.font_manifest.has_value()) {
        for (const auto& font : scene.font_manifest->fonts) {
            std::filesystem::path full_path = resolve_path(font.src.string(), root);
            add_entry(report, "font", full_path);
        }
    }

    // Gap 3 & 4: check audio tracks in compositions and subtitle paths
    for (const auto& comp : scene.compositions) {
        for (const auto& track : comp.audio_tracks) {
            std::filesystem::path full_path = resolve_path(track.source_path, root);
            add_entry(report, "audio", full_path);
        }
        for (const auto& layer : comp.layers) {
            if (!layer.subtitle_path.empty()) {
                std::filesystem::path full_path = resolve_path(layer.subtitle_path, root);
                add_entry(report, "subtitle", full_path);
            }
        }
    }

    report.all_present = std::all_of(report.entries.begin(), report.entries.end(),
                                     [](const auto& e) { return e.exists; });

    return report;
}

} // namespace tachyon

