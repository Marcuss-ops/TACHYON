#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/media/resolution/asset_resolution.h"
#include "tachyon/core/cli_scene_loader.h"
#include "tachyon/runtime/execution/native_render.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include "tachyon/presets/text/text_registry.h"
#include "command.h"
#include <iostream>
#include <filesystem>

namespace tachyon {

void print_diagnostics(const DiagnosticBag& diagnostics, std::ostream& out) {
    for (const auto& d : diagnostics.diagnostics) {
        out << "[" << diagnostic_severity_string(d.severity) << "] " << d.path << ": " << d.message << "\n";
    }
}

bool run_preview_internal(const CommandContext& context, const char* label) {
    const auto& options = context.options;
    auto& out = context.out;
    auto& err = context.err;
    auto& transition_registry = context.runtime.transitions;
    auto& text_registry = *context.runtime.text_registry;

    SceneLoadOptions load_opts;
    load_opts.cpp_path = options.cpp_path;
    load_opts.preset_id = options.preset_id;

    auto load_result = load_scene_for_cli(load_opts, SceneLoadMode::Preview, out, err);
    if (!load_result.success) {
        return false;
    }

    SceneSpec& scene = load_result.context->scene;
    
    if (scene.compositions.empty()) {
        err << "[" << label << "] Scene has no compositions.\n";
        return false;
    }

    std::int64_t frame_number = options.preview_frame_number.has_value()
                                    ? *options.preview_frame_number
                                    : 0;
    std::filesystem::path output_path = options.preview_output;

    if (output_path.empty()) {
        output_path = "preview.png";
    }

    // Always use the first composition for preview if not specified
    const std::string& comp_id = scene.compositions.front().id;

    out << "[" << label << "] Generating preview for frame " << frame_number << "...\n";

    bool success = NativeRenderer::render_still(
        scene, 
        comp_id,
        frame_number,
        output_path,
        transition_registry,
        text_registry
    );

    if (!success) {
        err << "[" << label << "] Preview generation failed.\n";
        return false;
    }

    out << "[" << label << "] Preview saved to: " << output_path << "\n";
    return true;
}

} // namespace tachyon
