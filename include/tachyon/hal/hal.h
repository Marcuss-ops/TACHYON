#pragma once

#include "tachyon/hal/backend_type.h"
#include "tachyon/hal/device_info.h"
#include <memory>
#include <vector>

namespace tachyon::hal {

class HAL {
public:
    static HAL& instance();

    void refresh();
    const std::vector<HardwareDevice>& devices() const { return devices_; }
    const HardwareDevice* best_gpu() const;
    const HardwareDevice* cpu_device() const;
    BackendType preferred_backend() const;

private:
    HAL() = default;
    void detect_cpu();
    void detect_vulkan();
    void detect_embree();

    std::vector<HardwareDevice> devices_;
};

} // namespace tachyon::hal
