#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/report.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/media/resolution/asset_resolution.h"
#include "tachyon/media/resolution/asset_path_utils.h"
#include "tachyon/core/cli_scene_loader.h"
#include "tachyon/runtime/execution/native_render.h"
#include <iostream>
#include <filesystem>

namespace tachyon {

void print_diagnostics(const DiagnosticBag& diagnostics, std::ostream& out) {
    for (const auto& diagnostic : diagnostics.diagnostics) {
        out << diagnostic.code << ": " << diagnostic.message;
        if (!diagnostic.path.empty()) {
            out << " (" << diagnostic.path << ")";
        }
        out << '\n';
    }
}

std::filesystem::path scene_asset_root(const std::filesystem::path& scene_path) {
    return tachyon::media::scene_asset_root(scene_path);
}

bool load_scene_context(const std::filesystem::path& scene_path, SceneSpec& scene, AssetResolutionTable& assets, std::ostream& err) {
    const auto parsed = parse_scene_spec_file(scene_path);
    if (!parsed.value.has_value()) {
        print_diagnostics(parsed.diagnostics, err);
        return false;
    }
    const auto validation = validate_scene_spec(*parsed.value);
    if (!validation.ok()) {
        print_diagnostics(validation.diagnostics, err);
        return false;
    }
    const auto resolved = resolve_assets(*parsed.value, scene_asset_root(scene_path));
    if (!resolved.value.has_value()) {
        print_diagnostics(resolved.diagnostics, err);
        return false;
    }
    scene = *parsed.value;
    assets = *resolved.value;
    return true;
}

bool run_preview_internal(const ::tachyon::CliOptions& options, std::ostream& out, std::ostream& err, const char* label) {
    SceneLoadOptions load_opts;
    load_opts.cpp_path = options.cpp_path;
    load_opts.scene_path = options.scene_path;
    load_opts.preset_id = options.preset_id;

    auto loaded = load_scene_for_cli(load_opts, SceneLoadMode::Preview, out, err);
    if (!loaded.success) {
        print_diagnostics(loaded.diagnostics, err);
        return false;
    }

    SceneSpec& scene = loaded.context->scene;
    if (scene.compositions.empty()) {
        err << "Scene has no compositions.\n";
        return false;
    }

    std::string composition_id = scene.compositions.front().id;
    std::int64_t frame = options.preview_frame_number.has_value() ? *options.preview_frame_number : 0;
    std::filesystem::path output = !options.preview_output.empty() ? options.preview_output : "preview.png";

    if (!options.json_output) {
        out << "[" << label << "] Rendering frame " << frame << " to " << output.string() << "\n";
    }

    const bool success = NativeRenderer::render_still(scene, composition_id, frame, output);
    if (!success) {
        err << "Preview render failed.\n";
    } else if (options.json_output) {
        out << "{\"status\": \"ok\", \"output\": \"" << output.string() << "\"}\n";
    } else {
        out << "[" << label << "] Wrote " << output.string() << "\n";
    }

    return success;
}

} // namespace tachyon
