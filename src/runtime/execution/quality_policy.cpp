#include "tachyon/runtime/execution/quality_policy.h"

namespace tachyon {

QualityPolicy make_quality_policy(const std::string& tier) {
    QualityPolicy policy;
    if (tier == "draft") {
        policy.resolution_scale = 0.5f;
        policy.motion_blur_sample_cap = 1; // Disabled
        policy.effects_enabled = false;
        policy.precomp_cache_budget = 64ULL * 1024 * 1024;
        policy.tile_size = 256;
        policy.max_workers = 2;
    } else if (tier == "preview") {
        policy.resolution_scale = 0.75f;
        policy.motion_blur_sample_cap = 4;
        policy.effects_enabled = true;
        policy.precomp_cache_budget = 256ULL * 1024 * 1024;
        policy.tile_size = 512;
        policy.max_workers = 0;
    } else { // "production", "high", "cinematic"
        policy.resolution_scale = 1.0f;
        policy.motion_blur_sample_cap = 64;
        policy.effects_enabled = true;
        policy.precomp_cache_budget = 1024ULL * 1024 * 1024;
        policy.tile_size = 512;
        policy.max_workers = 0;
    }
    return policy;
}

} // namespace tachyon
