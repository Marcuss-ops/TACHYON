#pragma once

#include <optional>
#include <iterator>
#include <string>
#include <vector>

namespace tachyon {

enum class DiagnosticSeverity {
    Info,
    Warning,
    Error
};

struct Diagnostic {
    DiagnosticSeverity severity{DiagnosticSeverity::Error};
    std::string category;
    std::string code;
    std::string message;
    std::string path;
    std::string help_link;
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

    void append(const DiagnosticBag& other) {
        diagnostics.insert(diagnostics.end(), other.diagnostics.begin(), other.diagnostics.end());
    }

    void append(DiagnosticBag&& other) {
        diagnostics.insert(diagnostics.end(),
                           std::make_move_iterator(other.diagnostics.begin()),
                           std::make_move_iterator(other.diagnostics.end()));
    }
};

struct ValidationResult {
    DiagnosticBag diagnostics;

    [[nodiscard]] bool ok() const noexcept {
        return diagnostics.ok();
    }
};

} // namespace tachyon
