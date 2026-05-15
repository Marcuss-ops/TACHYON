#pragma once
#include "tachyon/core/cli_options.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include <string>
#include <vector>

namespace tachyon {

bool parse_inspect_option(const std::string& arg, const std::vector<std::string>& args, std::size_t& index, CliOptions& options, DiagnosticBag& diagnostics);

} // namespace tachyon
