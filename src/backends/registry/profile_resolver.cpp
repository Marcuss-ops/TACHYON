#include "tachyon/backends/profile_resolver.h"
#include "tachyon/tachyon_build_config.h"
#include <stdexcept>

namespace tachyon::backends {

ResolvedExecutionProfile resolve_profile(
    const core::profiles::ExecutionProfile& profile,
    const BackendRegistry& registry
) {
    ResolvedExecutionProfile resolved;
    resolved.width = profile.width;
    resolved.height = profile.height;
    resolved.fps = profile.fps;
    resolved.quality = profile.quality;
    resolved.cache_enabled = profile.cache_enabled;
    resolved.jit_optimization = profile.jit_optimization;

    // 1. Resolve Video Encoder
    if (profile.video_encoder == "default" || profile.video_encoder == "auto") {
#if TACHYON_ENABLE_MEDIA
        resolved.video_encoder_id = "ffmpeg";
#else
        resolved.video_encoder_id = "none";
#endif
    } else {
        resolved.video_encoder_id = profile.video_encoder;
    }

    // 2. Resolve Audio Analyzer
    if (profile.audio_analyzer == "default" || profile.audio_analyzer == "auto") {
#if TACHYON_ENABLE_WHISPER
        resolved.audio_analyzer_id = "whisper";
#else
        resolved.audio_analyzer_id = "none";
#endif
    } else {
        resolved.audio_analyzer_id = profile.audio_analyzer;
    }

    // 3. Resolve Transition Renderer
    if (profile.transition_kernel == "auto") {
        // Deterministic preference: highway > avx2 > builtin
        auto probes = registry.list_transition_renderers();
        bool has_highway = false;
        bool has_avx2 = false;
        for (const auto& p : probes) {
            if (p == "highway") has_highway = true;
            if (p == "avx2") has_avx2 = true;
        }

        if (has_highway) resolved.transition_renderer_id = "highway";
        else if (has_avx2) resolved.transition_renderer_id = "avx2";
        else resolved.transition_renderer_id = "builtin";
    } else if (profile.transition_kernel == "default") {
        resolved.transition_renderer_id = "builtin";
    } else {
        resolved.transition_renderer_id = profile.transition_kernel;
    }

    return resolved;
}

} // namespace tachyon::backends
