#pragma once
#include "tachyon/core/cli_options.h"
#include <string>
#include <vector>

namespace tachyon {

std::string require_argument(const std::vector<std::string>& args, std::size_t& index);

bool parse_render_option(const std::string& arg, const std::vector<std::string>& args, std::size_t& index, CliOptions& options, DiagnosticBag& diagnostics);
bool parse_inspect_option(const std::string& arg, const std::vector<std::string>& args, std::size_t& index, CliOptions& options, DiagnosticBag& diagnostics);
bool parse_metrics_option(const std::string& arg, const std::vector<std::string>& args, std::size_t& index, CliOptions& options, DiagnosticBag& diagnostics);
bool parse_tool_option(const std::string& arg, const std::vector<std::string>& args, std::size_t& index, CliOptions& options, DiagnosticBag& diagnostics);

} // namespace tachyon
