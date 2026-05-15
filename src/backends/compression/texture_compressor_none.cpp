#include "tachyon/backends/compression/texture_compressor_factory.h"

namespace tachyon::backends::compression {

using TextureCompressionInput = ::tachyon::core::media::TextureCompressionInput;
using TextureCompressionOutput = ::tachyon::core::media::TextureCompressionOutput;

class NoneTextureCompressor : public TextureCompressor {
public:
    TextureCompressionOutput compress(const TextureCompressionInput& input) override {
        TextureCompressionOutput output;
        output.bytes = input.rgba;
        output.codec = "none";
        return output;
    }
};

TextureCompressor& none_texture_compressor() {
    static NoneTextureCompressor instance;
    return instance;
}

} // namespace tachyon::backends::compression
