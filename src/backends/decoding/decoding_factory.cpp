#include "tachyon/backends/decoding/decoding_factory.h"
#include "tachyon/backends/decoding/ffmpeg_video_decoder.h"

namespace tachyon::backends::decoding {

std::unique_ptr<IVideoDecoder> create_video_decoder() {
    return std::make_unique<FfmpegVideoDecoder>();
}

} // namespace tachyon::backends::decoding
