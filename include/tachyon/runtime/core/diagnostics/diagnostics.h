#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace tachyon {

enum class DiagnosticSeverity {
    Info,
    Warning,
    Error
};

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

struct Diagnostic {
    DiagnosticSeverity severity{DiagnosticSeverity::Error};
    std::string category; ///< Category for grouping (e.g., "authoring", "runtime", "parsing")
    std::string code;     ///< Unique stable error code.
    std::string message;  ///< Human readable message.
    std::string path;     ///< Path to the offending element (if applicable).
    std::string help_link; ///< URL for detailed troubleshooting.
};

class DiagnosticBag {
public:
    std::vector<Diagnostic> diagnostics;

    [[nodiscard]] bool ok() const noexcept {
        for (const auto& diagnostic : diagnostics) {
            if (diagnostic.severity == DiagnosticSeverity::Error) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] bool has_warnings() const noexcept {
        for (const auto& diagnostic : diagnostics) {
            if (diagnostic.severity == DiagnosticSeverity::Warning) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] bool has_infos() const noexcept {
        for (const auto& diagnostic : diagnostics) {
            if (diagnostic.severity == DiagnosticSeverity::Info) {
                return true;
            }
        }
        return false;
    }

    void add(DiagnosticSeverity severity, std::string category, std::string code, std::string message, std::string path = {}, std::string help_link = {}) {
        diagnostics.push_back(Diagnostic{severity, std::move(category), std::move(code), std::move(message), std::move(path), std::move(help_link)});
    }

    void add_info(std::string code, std::string message, std::string path = {}) {
        add(DiagnosticSeverity::Info, "general", std::move(code), std::move(message), std::move(path));
    }

    void add_warning(std::string code, std::string message, std::string path = {}) {
        add(DiagnosticSeverity::Warning, "general", std::move(code), std::move(message), std::move(path));
    }

    void add_error(std::string code, std::string message, std::string path = {}) {
        add(DiagnosticSeverity::Error, "general", std::move(code), std::move(message), std::move(path));
    }

    void append(const DiagnosticBag& other) {
        diagnostics.insert(diagnostics.end(), other.diagnostics.begin(), other.diagnostics.end());
    }
};

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

struct ValidationResult {
    DiagnosticBag diagnostics;

    [[nodiscard]] bool ok() const noexcept {
        return diagnostics.ok();
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
};

} // namespace tachyon
