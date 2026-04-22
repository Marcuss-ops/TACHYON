#include "batch_runner_internal.h"

namespace tachyon {

std::filesystem::path resolve_relative_path(const std::filesystem::path& base, const std::filesystem::path& value) {
    if (value.empty() || value.is_absolute()) return value;
    return base.empty() ? value : base / value;
}

std::string summarize_diagnostics(const DiagnosticBag& diagnostics, const std::string& fallback) {
    if (diagnostics.diagnostics.empty()) return fallback;
    const auto& first = diagnostics.diagnostics.front();
    return first.code + ": " + first.message;
}

} // namespace tachyon
