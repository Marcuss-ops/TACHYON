#include "tachyon/backends/decoding/decoding_factory.h"
#include "tachyon/tachyon_build_config.h"

#if TACHYON_ENABLE_MEDIA
#include "tachyon/backends/decoding/ffmpeg_video_decoder.h"
#endif

// Add other headers as they are implemented
// #include "tachyon/backends/decoding/stb_decoder.h"
// #include "tachyon/backends/decoding/svg_decoder.h"

namespace tachyon::backends::decoding {

std::unique_ptr<IVideoDecoder> create_video_decoder() {
#if TACHYON_ENABLE_MEDIA
    return std::make_unique<FfmpegVideoDecoder>();
#else
    return nullptr;
#endif
}

// TODO: Implement create_image_decoder() and create_svg_decoder()
// that return specific backend implementations.

} // namespace tachyon::backends::decoding
