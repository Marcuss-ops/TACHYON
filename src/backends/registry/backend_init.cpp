#include "tachyon/backends/backend_init.h"
#include "tachyon/backends/backend_registry.h"
#include "tachyon/tachyon_build_config.h"
#include "tachyon/core/cli.h"

namespace tachyon::backends::ffmpeg { void register_backend(); }
namespace tachyon::backends::whisper { void register_backend(); }
namespace tachyon::backends::simd { void register_backend(); }

namespace tachyon::backends {

struct BackendInitRegister {
    BackendInitRegister() {
        g_init_backends_ptr = &initialize_all_backends;
    }
};
static BackendInitRegister g_backend_init_register;

void initialize_all_backends() {
    auto& reg = BackendRegistry::instance();

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
