#pragma once

#include <filesystem>
#include <string>

namespace tachyon::core::platform {

inline std::string quote_shell_arg(const std::string& arg) {
    std::string quoted = "\"";
    for (char c : arg) {
        if (c == '"') {
            quoted += "\\\"";
        } else {
            quoted += c;
        }
    }
    quoted += "\"";
    return quoted;
}

inline std::string quote_shell_arg(const std::filesystem::path& path) {
    return quote_shell_arg(path.string());
}

} // namespace tachyon::core::platform
