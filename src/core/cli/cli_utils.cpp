#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/report.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/media/resolution/asset_resolution.h"
#include "tachyon/media/resolution/asset_path_utils.h"
#include "tachyon/core/cli_scene_loader.h"
#include "tachyon/runtime/execution/native_render.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include <iostream>
#include <filesystem>

namespace tachyon {

void print_diagnostics(const DiagnosticBag& diagnostics, std::ostream& out) {
    for (const auto& d : diagnostics.diagnostics) {
        out << "[" << diagnostic_severity_string(d.severity) << "] " << d.path << ": " << d.message << "\n";
    }
}


bool run_preview_internal(const ::tachyon::CliOptions& options, std::ostream& out, std::ostream& err, const char* label) {
    SceneLoadOptions load_opts;
    load_opts.cpp_path = options.cpp_path;
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

    out << "[" << label << "] Rendering frame " << frame << " to " << output.string() << "\n";

    const bool success = NativeRenderer::render_still(scene, composition_id, frame, output);
    if (!success) {
        err << "Preview render failed.\n";
    } else {
        out << "[" << label << "] Wrote " << output.string() << "\n";
    }

    return success;
}

} // namespace tachyon
