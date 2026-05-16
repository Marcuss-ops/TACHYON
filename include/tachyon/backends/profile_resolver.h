#pragma once

#include "tachyon/core/profiles/execution_profile.h"
#include "tachyon/backends/backend_registry.h"
#include <string>

namespace tachyon::backends {

/**
 * @brief Resolved version of an ExecutionProfile where all "auto" and "default" values
 * are mapped to concrete backend IDs.
 */
struct ResolvedExecutionProfile {
    std::string video_encoder_id;
    std::string audio_analyzer_id;
    std::string transition_renderer_id;
    
    // Original profile values
    int width;
    int height;
    double fps;
    std::string quality;
    bool cache_enabled;
    bool jit_optimization;
};

/**
 * @brief Deterministically resolves an ExecutionProfile using the current BackendRegistry.
 */
ResolvedExecutionProfile resolve_profile(
    const core::profiles::ExecutionProfile& profile,
    const BackendRegistry& registry
);

} // namespace tachyon::backends
