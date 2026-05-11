#pragma once
#include <string>

namespace tachyon::core {

/**
 * @brief Represents the CPU instruction set capabilities detected at runtime.
 */
struct CpuCapabilities {
    bool sse3 = false;
    bool sse41 = false;
    bool sse42 = false;
    bool avx = false;
    bool avx2 = false;
    bool avx512f = false;
    bool neon = false;
    bool sve = false;
    
    std::string brand;
    int family = 0;
    int model = 0;
    int stepping = 0;
};

/**
 * @brief Retrieves the cached CPU capabilities of the host system.
 */
const CpuCapabilities& get_cpu_capabilities();

} // namespace tachyon::core
