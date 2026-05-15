#include "tachyon/media/compression/texture_compressor.h"
#include "tachyon/backends/compression/texture_compressor_factory.h"

namespace tachyon::media {

// For legacy compatibility, we provide these factory functions in the tachyon::media namespace
// but they now delegate to the backends implementation.

TextureCompressor& none_texture_compressor() {
    return ::tachyon::backends::compression::none_texture_compressor();
}

#if defined(TACHYON_ENABLE_BASIS)
TextureCompressor& basis_texture_compressor() {
    return ::tachyon::backends::compression::basis_texture_compressor();
}
#endif

} // namespace tachyon::media
