#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <tachyon/core/types/diagnostics.h>

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

// TimingSample has been moved to tachyon/core/types/diagnostics.h
 
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

    std::size_t node_cache_lookups{0};
    std::size_t node_cache_hits{0};
    std::size_t node_cache_misses{0};
    std::size_t node_cache_bytes{0};
    std::size_t static_nodes_detected{0};
    std::size_t animated_nodes_detected{0};

    // Canonical timing categories
    static constexpr const char* kCategoryRender = "render";
    static constexpr const char* kCategoryTransition = "transition";
    static constexpr const char* kCategoryDecode = "decode";
    static constexpr const char* kCategoryOutputWrite = "output_write";
    static constexpr const char* kCategoryEncode = "encode";
    static constexpr const char* kCategoryCache = "cache";
    static constexpr const char* kCategorySurfacePool = "surface_pool";

    std::vector<TimingSample> timings;
    DiagnosticBag diagnostics;

    // Cache key manifests for debugging
    std::string frame_key_manifest;
    std::string composition_key_manifest;

    void add_timing(std::string category, std::string label, double milliseconds) {
        timings.push_back({std::move(category), std::move(label), milliseconds});
    }

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

    bool has_category(const std::string& category, const std::string& label = {}) const {
        for (const auto& t : timings) {
            if (t.category == category) {
                if (label.empty() || t.label == label) return true;
            }
        }
        return false;
    }
};

} // namespace tachyon
