#pragma once

#include "tachyon/runtime/telemetry/process_resource_sampler.h"
#include <cstddef>

namespace tachyon {

class ScopedProcessSampler {
public:
    explicit ScopedProcessSampler(bool enabled) : enabled_(enabled) {
        if (enabled_) {
            sampler_.start();
        }
    }

    void stop() {
        if (enabled_) {
            sampler_.stop();
        }
    }

    bool enabled() const { return enabled_; }
    ProcessResourceSampler& sampler() { return sampler_; }

    std::size_t peak_working_set_bytes() const { return enabled_ ? sampler_.peak_working_set_bytes() : 0; }
    std::size_t avg_working_set_bytes() const { return enabled_ ? sampler_.avg_working_set_bytes() : 0; }
    std::size_t peak_private_bytes() const { return enabled_ ? sampler_.peak_private_bytes() : 0; }
    std::size_t avg_private_bytes() const { return enabled_ ? sampler_.avg_private_bytes() : 0; }
    double avg_cpu_percent_machine() const { return enabled_ ? sampler_.avg_cpu_percent_machine() : 0.0; }
    double avg_cpu_cores_used() const { return enabled_ ? sampler_.avg_cpu_cores_used() : 0.0; }

private:
    bool enabled_{false};
    ProcessResourceSampler sampler_;
};

} // namespace tachyon
