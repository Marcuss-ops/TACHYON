#include "tachyon/backends/backend_registry.h"
#include "tachyon/tachyon_build_config.h"
#include "simd_transition_renderers.h"

namespace tachyon::backends::simd {

void register_backend() {
    auto& reg = BackendRegistry::instance();

#if TACHYON_ENABLE_HIGHWAY
    reg.register_transition_renderer("highway", []() {
        return std::make_unique<HighwayTransitionRenderer>();
    });
#endif

}

} // namespace tachyon::backends::simd
