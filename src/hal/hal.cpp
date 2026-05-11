#include "tachyon/hal/hal.h"
#include "tachyon/core/string_utils.h"
#include <thread>
#include <algorithm>

namespace tachyon::hal {

GPUVendor detect_gpu_vendor(const std::string& device_name, const std::string& vendor_string) {
    std::string name_lower = device_name;
    std::string vendor_lower = vendor_string;
    ascii_lower_inplace(name_lower);
    ascii_lower_inplace(vendor_lower);

    if (vendor_lower.find("nvidia") != std::string::npos) return GPUVendor::NVIDIA;
    if (vendor_lower.find("amd") != std::string::npos || vendor_lower.find("advanced micro") != std::string::npos) return GPUVendor::AMD;
    if (vendor_lower.find("intel") != std::string::npos) return GPUVendor::Intel;
    if (name_lower.find("apple") != std::string::npos || vendor_lower.find("apple") != std::string::npos) return GPUVendor::Apple;
    if (vendor_lower.find("vulkan") != std::string::npos) return GPUVendor::Vulkan;
    return GPUVendor::Unknown;
}

HAL& HAL::instance() {
    static HAL hal;
    return hal;
}

void HAL::refresh() {
    devices_.clear();
    detect_cpu();
    detect_vulkan();
}

void HAL::detect_cpu() {
    HardwareDevice cpu;
    cpu.name = "CPU";
    cpu.type = DeviceType::CPU;
    cpu.vendor = GPUVendor::Unknown;
    cpu.caps.compute = true;
    cpu.caps.max_compute_units = static_cast<uint32_t>(std::thread::hardware_concurrency());
    cpu.caps.tensor_ops = false;
    cpu.caps.ray_tracing = true;
    cpu.is_available = true;
    cpu.device_id = "cpu";
    devices_.push_back(cpu);
}

void HAL::detect_vulkan() {
#ifdef TACHYON_VULKAN
    // Vulkan detection would go here
    // For now, we add a placeholder that would be filled by actual Vulkan queries
#endif
}

const HardwareDevice* HAL::best_gpu() const {
    for (const auto& d : devices_) {
        if (d.type == DeviceType::GPU && d.is_available) return &d;
    }
    return nullptr;
}

const HardwareDevice* HAL::cpu_device() const {
    for (const auto& d : devices_) {
        if (d.type == DeviceType::CPU && d.is_available) return &d;
    }
    return nullptr;
}

BackendType HAL::preferred_backend() const {
    if (best_gpu()) return BackendType::Vulkan;
    return BackendType::CPU;
}

} // namespace tachyon::hal
