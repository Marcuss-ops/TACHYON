#pragma once

#include "tachyon/core/media/compression/texture_compressor.h"
#include "tachyon/tachyon_build_config.h"

namespace tachyon::backends::compression {

using TextureCompressor = ::tachyon::core::media::TextureCompressor;

TextureCompressor& none_texture_compressor();

#if defined(TACHYON_ENABLE_BASIS)
TextureCompressor& basis_texture_compressor();
#endif

} // namespace tachyon::backends::compression
