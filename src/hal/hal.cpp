#include "tachyon/hal/hal.h"
#include <thread>
#include <algorithm>

namespace tachyon::hal {

GPUVendor detect_gpu_vendor(const std::string& device_name, const std::string& vendor_string) {
    std::string name_lower = device_name;
    std::string vendor_lower = vendor_string;
    for (auto& c : name_lower) c = static_cast<char>(::tolower(c));
    for (auto& c : vendor_lower) c = static_cast<char>(::tolower(c));

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
    detect_embree();
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

void HAL::detect_embree() {
#ifdef TACHYON_EMBREE
    HardwareDevice embree_dev;
    embree_dev.name = "Embree Ray Tracer";
    embree_dev.type = DeviceType::CPU;
    embree_dev.vendor = GPUVendor::Intel;
    embree_dev.caps.compute = true;
    embree_dev.caps.ray_tracing = true;
    embree_dev.caps.denoising = true;
    embree_dev.caps.max_compute_units = static_cast<uint32_t>(std::thread::hardware_concurrency());
    embree_dev.is_available = true;
    embree_dev.device_id = "embree";
    devices_.push_back(embree_dev);
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
