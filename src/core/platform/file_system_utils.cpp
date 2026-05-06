#include "tachyon/core/platform/file_system_utils.h"
#include <filesystem>

namespace tachyon::core::platform {

bool remove_file_silent(const std::string& path) {
    if (path.empty()) {
        return true;
    }
    
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        return true;
    }
    
    return std::filesystem::remove(path, ec);
}

} // namespace tachyon::core::platform
