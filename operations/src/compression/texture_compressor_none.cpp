#include "tachyon/media/compression/texture_compressor.h"

namespace tachyon::media {
namespace {

class NoneTextureCompressor final : public TextureCompressor {
public:
    TextureCompressionOutput compress(const TextureCompressionInput& input) override {
        TextureCompressionOutput output;
        output.codec = "none";
        output.bytes = input.rgba;
        return output;
    }
};

} // namespace

TextureCompressor& none_texture_compressor() {
    static NoneTextureCompressor compressor;
    return compressor;
}

} // namespace tachyon::media