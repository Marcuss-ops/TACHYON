#pragma once

#include "tachyon/core/media/decoding/video_decoder_interface.h"
#include <memory>

namespace tachyon::backends::decoding {

using IVideoDecoder = ::tachyon::core::media::IVideoDecoder;

std::unique_ptr<IVideoDecoder> create_video_decoder();

} // namespace tachyon::backends::decoding
