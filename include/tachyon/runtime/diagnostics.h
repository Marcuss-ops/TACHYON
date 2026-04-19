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
    std::string code;
    std::string message;
    std::string path;
};

struct DiagnosticBag {
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

    void add(DiagnosticSeverity severity, std::string code, std::string message, std::string path = {}) {
        diagnostics.push_back(Diagnostic{severity, std::move(code), std::move(message), std::move(path)});
    }

    void add_info(std::string code, std::string message, std::string path = {}) {
        add(DiagnosticSeverity::Info, std::move(code), std::move(message), std::move(path));
    }

    void add_warning(std::string code, std::string message, std::string path = {}) {
        add(DiagnosticSeverity::Warning, std::move(code), std::move(message), std::move(path));
    }

    void add_error(std::string code, std::string message, std::string path = {}) {
        add(DiagnosticSeverity::Error, std::move(code), std::move(message), std::move(path));
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

struct ValidationResult {
    DiagnosticBag diagnostics;

    [[nodiscard]] bool ok() const noexcept {
        return diagnostics.ok();
    }
};

} // namespace tachyon
