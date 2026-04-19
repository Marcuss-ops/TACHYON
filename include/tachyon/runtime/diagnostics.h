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

    void add(DiagnosticSeverity severity, std::string code, std::string message, std::string path = {}) {
        diagnostics.push_back(Diagnostic{severity, std::move(code), std::move(message), std::move(path)});
    }

    void add_error(std::string code, std::string message, std::string path = {}) {
        add(DiagnosticSeverity::Error, std::move(code), std::move(message), std::move(path));
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
