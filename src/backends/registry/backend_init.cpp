#include "tachyon/backends/backend_init.h"
#include "tachyon/backends/backend_registry.h"
#include "tachyon/tachyon_build_config.h"
#include "tachyon/runtime/transitions/transition_ops.h"

namespace tachyon::backends::ffmpeg { void register_backend(); }
namespace tachyon::backends::whisper { void register_backend(); }
namespace tachyon::backends::simd { void register_backend(); }

namespace tachyon::backends {

void initialize_all_backends() {
    auto& reg = BackendRegistry::instance();

    // Register built-in transition renderer
    reg.register_transition_renderer("builtin", []() {
        return runtime::transitions::create_builtin_transition_renderer();
    });

#if TACHYON_ENABLE_MEDIA
    ffmpeg::register_backend();
#endif

#if TACHYON_ENABLE_WHISPER
    whisper::register_backend();
#endif

#if TACHYON_ENABLE_HIGHWAY || TACHYON_ENABLE_MANUAL_AVX2
    simd::register_backend();
#endif
}

} // namespace tachyon::backends
