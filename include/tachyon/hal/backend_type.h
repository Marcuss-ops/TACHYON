#pragma once

#include <cstdint>

namespace tachyon::hal {

enum class BackendType : uint8_t { CPU = 0, Vulkan = 1, Metal = 2, CUDA = 3, OpenCL = 4 };

constexpr const char* to_string(BackendType t) {
    switch (t) {
        case BackendType::CPU:    return "CPU";
        case BackendType::Vulkan: return "Vulkan";
        case BackendType::Metal:  return "Metal";
        case BackendType::CUDA:   return "CUDA";
        case BackendType::OpenCL: return "OpenCL";
        default:                  return "Unknown";
    }
}

inline bool is_gpu_backend(BackendType t) {
    return t != BackendType::CPU;
}

} // namespace tachyon::hal
