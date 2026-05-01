#pragma once

#include <filesystem>
#include <string>

namespace tachyon::core::platform {

// Quote a shell argument, escaping internal quotes.
// Use for any path or string passed to a subprocess command.
// Quotes arguments containing spaces or empty strings.
inline std::string quote_shell_arg(const std::string& arg) {
    if (arg.find(' ') != std::string::npos || arg.empty()) {
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
    return arg;
}

inline std::string quote_shell_arg(const std::filesystem::path& path) {
    return quote_shell_arg(path.string());
}

} // namespace tachyon::core::platform
