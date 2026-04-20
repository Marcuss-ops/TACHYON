#pragma once

#include <cstddef>
#include <string>

namespace tachyon {

struct QualityPolicy {
    float resolution_scale{1.0f};
    int motion_blur_sample_cap{64};
    bool effects_enabled{true};
    bool precomp_cache_enabled{true};
    std::size_t precomp_cache_budget{1ULL * 1024 * 1024 * 1024};
    int tile_size{0};            // 0 = full frame
    std::size_t max_workers{0};  // 0 = all cores
    int ray_tracer_spp{1};       // Samples per pixel for 3D ray tracer
    int motion_blur_samples{1};  // 1 = disabled
    float shutter_angle{180.0f}; // 180 degrees = half frame
};

QualityPolicy make_quality_policy(const std::string& tier);

} // namespace tachyon
