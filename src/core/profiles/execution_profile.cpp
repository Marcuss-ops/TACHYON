#include "tachyon/core/profiles/execution_profile.h"

namespace tachyon::core::profiles {

ExecutionProfile ExecutionProfile::preview() {
    ExecutionProfile p;
    p.name = "preview";
    p.width = 1280;
    p.height = 720;
    p.fps = 30.0;
    p.quality = "draft";
    p.jit_optimization = false;
    p.cache_enabled = true;
    return p;
}

ExecutionProfile ExecutionProfile::final() {
    ExecutionProfile p;
    p.name = "final";
    p.width = 1920;
    p.height = 1080;
    p.fps = 60.0;
    p.quality = "high";
    p.jit_optimization = true;
    p.cache_enabled = true;
    return p;
}

} // namespace tachyon::core::profiles
