#pragma once
#include "tachyon/core/cli_options.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/media/resolution/asset_resolution.h"
#include <iostream>
#include <filesystem>

namespace tachyon {

class TransitionRegistry;

void print_diagnostics(const DiagnosticBag& diagnostics, std::ostream& out);

bool run_validate_command(const CliOptions& options, std::ostream& out, std::ostream& err, TransitionRegistry& registry);
bool run_inspect_command(const CliOptions& options, std::ostream& out, std::ostream& err, TransitionRegistry& registry);
bool run_motion_map_command(const CliOptions& options, std::ostream& out, std::ostream& err, TransitionRegistry& registry);
bool run_inspect_fonts_command(const CliOptions& options, std::ostream& out, std::ostream& err, TransitionRegistry& registry);
bool run_render_command(const CliOptions& options, std::ostream& out, std::ostream& err, TransitionRegistry& registry);
bool run_preview_command(const CliOptions& options, std::ostream& out, std::ostream& err, TransitionRegistry& registry);
bool run_preview_frame_command(const CliOptions& options, std::ostream& out, std::ostream& err, TransitionRegistry& registry);
bool run_watch_command(const CliOptions& options, std::ostream& out, std::ostream& err, TransitionRegistry& registry);
bool run_fetch_fonts_command(const CliOptions& options, std::ostream& out, std::ostream& err, TransitionRegistry& registry);
bool run_metrics_command(const CliOptions& options, std::ostream& out, std::ostream& err, TransitionRegistry& registry);
bool run_doctor_command(const CliOptions& options, std::ostream& out, std::ostream& err, TransitionRegistry& registry);
bool run_output_presets_command(const CliOptions& options, std::ostream& out, std::ostream& err, TransitionRegistry& registry);
bool run_thumb_command(const CliOptions& options, std::ostream& out, std::ostream& err, TransitionRegistry& registry);
bool run_preview_internal(const CliOptions& options, std::ostream& out, std::ostream& err, const char* label, TransitionRegistry& registry);
bool run_catalog_demo_command(const CliOptions& options, std::ostream& out, std::ostream& err, TransitionRegistry& registry);
bool run_transition_command(const CliOptions& options, std::ostream& out, std::ostream& err, TransitionRegistry& registry);

} // namespace tachyon
