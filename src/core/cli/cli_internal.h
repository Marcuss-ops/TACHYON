#pragma once
#include "tachyon/core/cli_options.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/media/resolution/asset_resolution.h"
#include <iostream>
#include <filesystem>

namespace tachyon {

void print_diagnostics(const DiagnosticBag& diagnostics, std::ostream& out);

bool run_validate_command(const CliOptions& options, std::ostream& out, std::ostream& err);
bool run_inspect_command(const CliOptions& options, std::ostream& out, std::ostream& err);
bool run_motion_map_command(const CliOptions& options, std::ostream& out, std::ostream& err);
bool run_inspect_fonts_command(const CliOptions& options, std::ostream& out, std::ostream& err);
bool run_render_command(const CliOptions& options, std::ostream& out, std::ostream& err);
bool run_preview_command(const CliOptions& options, std::ostream& out, std::ostream& err);
bool run_preview_frame_command(const CliOptions& options, std::ostream& out, std::ostream& err);
bool run_watch_command(const CliOptions& options, std::ostream& out, std::ostream& err);
#ifdef TACHYON_ENABLE_STUDIO_COMMANDS
bool run_studio_demo_command(const CliOptions& options, std::ostream& out, std::ostream& err);
#endif
bool run_fetch_fonts_command(const CliOptions& options, std::ostream& out, std::ostream& err);
bool run_preview_internal(const CliOptions& options, std::ostream& out, std::ostream& err, const char* label);

} // namespace tachyon
