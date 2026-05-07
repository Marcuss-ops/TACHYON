#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "tachyon/core/types/diagnostics.h"

namespace tachyon {

/**
 * @brief Controls how resolution failures are handled.
 */
enum class ResolveMode {
    Strict,                 ///< Fail immediately on missing assets/features (Error)
    PermissiveWithWarning,  ///< Use fallback if possible, but record a warning (Warning)
    PermissiveSilent        ///< Use fallback silently (Info)
};

[[nodiscard]] constexpr const char* diagnostic_severity_string(DiagnosticSeverity severity) noexcept {
    switch (severity) {
        case DiagnosticSeverity::Info:
            return "info";
        case DiagnosticSeverity::Warning:
            return "warning";
        case DiagnosticSeverity::Error:
            return "error";
    }
    return "error";
}

// RuntimeDiagnosticBag is now just an alias for DiagnosticBag to ensure consistency
using RuntimeDiagnosticBag = DiagnosticBag;

template <typename T>
struct ParseResult {
    std::optional<T> value;
    DiagnosticBag diagnostics;

    [[nodiscard]] bool ok() const noexcept {
        return value.has_value() && diagnostics.ok();
    }
};

template <typename T>
struct ResolutionResult {
    std::optional<T> value;
    DiagnosticBag diagnostics;

    [[nodiscard]] bool ok() const noexcept {
        return value.has_value() && diagnostics.ok();
    }
};

struct TimingSample {
    std::string category;
    std::string label;
    double milliseconds{0.0};
};

struct FrameDiagnostics {
    std::size_t property_hits{0};
    std::size_t property_misses{0};
    std::size_t layer_hits{0};
    std::size_t layer_misses{0};
    std::size_t composition_hits{0};
    std::size_t composition_misses{0};

    std::size_t properties_evaluated{0};
    std::size_t layers_evaluated{0};
    std::size_t compositions_evaluated{0};

    std::vector<TimingSample> timings;
    DiagnosticBag diagnostics;

    // Cache key manifests for debugging
    std::string frame_key_manifest;
    std::string composition_key_manifest;

    // Ergonomic proxies for internal diagnostics bag
    void add_info(std::string code, std::string message, std::string path = {}) {
        diagnostics.add_info(std::move(code), std::move(message), std::move(path));
    }
    void add_warning(std::string code, std::string message, std::string path = {}) {
        diagnostics.add_warning(std::move(code), std::move(message), std::move(path));
    }
    void add_error(std::string code, std::string message, std::string path = {}) {
        diagnostics.add_error(std::move(code), std::move(message), std::move(path));
    }
};

} // namespace tachyon
