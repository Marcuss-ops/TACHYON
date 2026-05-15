#pragma once
#include "tachyon/core/cli_options.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/media/resolution/asset_resolution.h"
#include <iostream>
#include <filesystem>

namespace tachyon {

namespace runtime { struct EngineRegistry; }

void print_diagnostics(const DiagnosticBag& diagnostics, std::ostream& out);

bool run_validate_command(const CliOptions& options, std::ostream& out, std::ostream& err, runtime::EngineRegistry& bundle);
bool run_inspect_command(const CliOptions& options, std::ostream& out, std::ostream& err, runtime::EngineRegistry& bundle);
bool run_motion_map_command(const CliOptions& options, std::ostream& out, std::ostream& err, runtime::EngineRegistry& bundle);
bool run_inspect_fonts_command(const CliOptions& options, std::ostream& out, std::ostream& err, runtime::EngineRegistry& bundle);
bool run_render_command(const CliOptions& options, std::ostream& out, std::ostream& err, runtime::EngineRegistry& bundle);
bool run_preview_command(const CliOptions& options, std::ostream& out, std::ostream& err, runtime::EngineRegistry& bundle);
bool run_preview_frame_command(const CliOptions& options, std::ostream& out, std::ostream& err, runtime::EngineRegistry& bundle);
bool run_watch_command(const CliOptions& options, std::ostream& out, std::ostream& err, runtime::EngineRegistry& bundle);
bool run_fetch_fonts_command(const CliOptions& options, std::ostream& out, std::ostream& err, runtime::EngineRegistry& bundle);
bool run_metrics_command(const CliOptions& options, std::ostream& out, std::ostream& err, runtime::EngineRegistry& bundle);
bool run_doctor_command(const CliOptions& options, std::ostream& out, std::ostream& err, runtime::EngineRegistry& bundle);
bool run_output_presets_command(const CliOptions& options, std::ostream& out, std::ostream& err, runtime::EngineRegistry& bundle);
bool run_thumb_command(const CliOptions& options, std::ostream& out, std::ostream& err, runtime::EngineRegistry& bundle);
bool run_preview_internal(const CliOptions& options, std::ostream& out, std::ostream& err, const char* label, runtime::EngineRegistry& bundle);
bool run_probe_command(const CliOptions& options, std::ostream& out, std::ostream& err, runtime::EngineRegistry& bundle);
bool run_concat_command(const CliOptions& options, std::ostream& out, std::ostream& err, runtime::EngineRegistry& bundle);

struct CommandDescriptor;
CommandDescriptor make_render_command();
CommandDescriptor make_preview_command();
CommandDescriptor make_preview_frame_command();
CommandDescriptor make_validate_command();
CommandDescriptor make_inspect_command();
CommandDescriptor make_motion_map_command();
CommandDescriptor make_watch_command();
CommandDescriptor make_metrics_command();
CommandDescriptor make_doctor_command();
CommandDescriptor make_output_presets_command();
CommandDescriptor make_thumb_command();
CommandDescriptor make_probe_command();
CommandDescriptor make_concat_command();
CommandDescriptor make_fetch_fonts_command();
CommandDescriptor make_inspect_fonts_command();
CommandDescriptor make_version_command();

} // namespace tachyon
