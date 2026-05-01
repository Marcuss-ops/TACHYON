#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace tachyon::hal {

enum class DeviceType { CPU, GPU, Accelerator };

enum class GPUVendor { Unknown, NVIDIA, AMD, Intel, Apple, Vulkan };

struct DeviceCapabilities {
    bool compute{false};
    bool ray_tracing{false};
    bool tensor_ops{false};
    bool denoising{false};
    uint32_t max_compute_units{0};
    uint32_t max_work_group_size{0};
    uint64_t total_memory{0};
    uint32_t driver_version_major{0};
    uint32_t driver_version_minor{0};
};

struct HardwareDevice {
    std::string name;
    DeviceType type{DeviceType::CPU};
    GPUVendor vendor{GPUVendor::Unknown};
    DeviceCapabilities caps;
    std::string device_id;
    bool is_available{false};
};

GPUVendor detect_gpu_vendor(const std::string& device_name, const std::string& vendor_string);

} // namespace tachyon::hal
