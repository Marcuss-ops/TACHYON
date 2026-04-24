#pragma once
#include "tachyon/core/cli_options.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/media/resolution/asset_resolution.h"
#include <iostream>
#include <filesystem>

namespace tachyon {

void print_diagnostics(const DiagnosticBag& diagnostics, std::ostream& out);
std::filesystem::path scene_asset_root(const std::filesystem::path& scene_path);
bool load_scene_context(const std::filesystem::path& scene_path, SceneSpec& scene, AssetResolutionTable& assets, std::ostream& err);

bool run_validate_command(const CliOptions& options, std::ostream& out, std::ostream& err);
bool run_inspect_command(const CliOptions& options, std::ostream& out, std::ostream& err);
bool run_inspect_fonts_command(const CliOptions& options, std::ostream& out, std::ostream& err);
bool run_render_command(const CliOptions& options, std::ostream& out, std::ostream& err);
bool run_watch_command(const CliOptions& options, std::ostream& out, std::ostream& err);
bool run_studio_demo_command(const CliOptions& options, std::ostream& out, std::ostream& err);

} // namespace tachyon
