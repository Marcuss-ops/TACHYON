#include "tachyon/core/integrity/integrity.h"
#include <crc32c/crc32c.h>
#include <fstream>
#include <vector>

namespace tachyon::core::integrity {

uint32_t compute_crc32c(const uint8_t* data, size_t size) {
    return crc32c::Crc32c(data, size);
}

uint32_t compute_file_crc32c(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return 0;
    }

    // Process in 64KB chunks
    constexpr size_t buffer_size = 64 * 1024;
    std::vector<char> buffer(buffer_size);
    
    uint32_t crc = 0;
    while (file.read(buffer.data(), buffer_size) || file.gcount() > 0) {
        crc = crc32c::Extend(crc, reinterpret_cast<const uint8_t*>(buffer.data()), file.gcount());
    }
    
    return crc;
}

} // namespace tachyon::core::integrity
