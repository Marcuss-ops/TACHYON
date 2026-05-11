#include "tachyon/media/compression/texture_compressor.h"

#include <mutex>
#include <vector>
#include <thread>
#include <cstring>

#include "encoder/basisu_comp.h"
#include "encoder/basisu_enc.h"

#if defined(TACHYON_ENABLE_HIGHWAY)
namespace tachyon::media::simd {
    void copy_rgba_to_basisu(uint8_t* dst, const uint8_t* src, std::size_t num_pixels);
}
#endif

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
        
        if (input.rgba.size() == input.width * input.height * 4) {
#if defined(TACHYON_ENABLE_HIGHWAY)
            simd::copy_rgba_to_basisu(reinterpret_cast<uint8_t*>(img.get_ptr()), input.rgba.data(), input.width * input.height);
#else
            std::memcpy(img.get_ptr(), input.rgba.data(), input.rgba.size());
#endif
        } else {
            // Fallback for non-RGBA or mismatched sizes
            for (int y = 0; y < input.height; ++y) {
                for (int x = 0; x < input.width; ++x) {
                    int idx = (y * input.width + x) * 4;
                    if (idx + 3 < input.rgba.size()) {
                        img(x, y).set(input.rgba[idx], input.rgba[idx+1], input.rgba[idx+2], input.rgba[idx+3]);
                    }
                }
            }
        }
        
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
