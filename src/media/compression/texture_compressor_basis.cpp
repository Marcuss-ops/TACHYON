#include "tachyon/media/compression/texture_compressor.h"

#ifdef TACHYON_ENABLE_BASIS
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

#include "encoder/basisu_comp.h"
#include "encoder/basisu_enc.h"

namespace tachyon::media {

class BasisTextureCompressor : public TextureCompressor {
public:
    BasisTextureCompressor() {
        static std::once_flag init_flag;
        std::call_once(init_flag, []() {
            basisu::basisu_encoder_init();
        });
    }

    TextureCompressionOutput compress(const TextureCompressionInput& input) override {
        basisu::basis_compressor_params params;
        params.m_uastc = true;
        params.m_pack_uastc_flags = basisu::cPackUASTCLevelDefault;
        params.m_mip_gen = false;
        params.m_perceptual = true;

        // We want the bytes in memory, not written to disk.
        params.m_write_output_basis_files = false;

        // Always provide a job pool, as UASTC path depends on it.
        uint32_t num_threads = std::thread::hardware_concurrency();
        if (num_threads < 1) num_threads = 1;
        basisu::job_pool jpool(num_threads);
        params.m_pJob_pool = &jpool;
        params.m_multithreading = (num_threads > 1);

        basisu::image img(input.width, input.height);

        // The builder always provides RGBA data. Use std::memcpy directly
        // (no Highway needed for a plain copy; std::memcpy is already SIMD-optimized by libc).
        std::memcpy(img.get_ptr(), input.rgba.data(), input.rgba.size());

        params.m_source_images.push_back(img);

        basisu::basis_compressor compressor;
        if (!compressor.init(params)) {
            return { {}, "none" };
        }

        if (compressor.process() != basisu::basis_compressor::cECSuccess) {
            return { {}, "none" };
        }

        TextureCompressionOutput output;
        const auto& basis_file = compressor.get_output_basis_file();
        output.bytes.assign(basis_file.begin(), basis_file.end());
        output.codec = "basis";
        return output;
    }
};

TextureCompressor& basis_texture_compressor() {
    static BasisTextureCompressor instance;
    return instance;
}

} // namespace tachyon::media

#else

namespace tachyon::media {
TextureCompressor& basis_texture_compressor() {
    return none_texture_compressor();
}
} // namespace tachyon::media

#endif
