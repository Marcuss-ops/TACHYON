#include "tachyon/core/io/path_resolution.h"
#include <algorithm>

namespace tachyon::io {

std::string resolve_relative_path(const std::string& base, const std::string& rel) {
    if (std::filesystem::path(rel).is_absolute()) return rel;
    std::filesystem::path base_path(base);
    if (std::filesystem::is_regular_file(base_path)) {
        base_path = base_path.parent_path();
    }
    return (base_path / rel).string();
}

std::string resolve_relative_to_root(const std::string& rel) {
    // Implementation would depend on global asset root state
    // For now, just return as-is or relative to CWD
    return rel;
}

std::string extension_lower(const std::string& path_str) {
    std::filesystem::path p(path_str);
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
    return ext;
}

} // namespace tachyon::io
