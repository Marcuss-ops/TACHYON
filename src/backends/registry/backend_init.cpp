#include "tachyon/backends/backend_init.h"
#include "tachyon/tachyon_build_config.h"

namespace tachyon::backends::ffmpeg { void register_backend(); }
namespace tachyon::backends::whisper { void register_backend(); }

namespace tachyon::backends {

void initialize_all_backends() {
#if TACHYON_ENABLE_MEDIA
    ffmpeg::register_backend();
#endif

    whisper::register_backend();
}

} // namespace tachyon::backends
