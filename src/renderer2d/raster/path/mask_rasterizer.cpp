#include "tachyon/renderer2d/raster/mask_rasterizer.h"
#include <immintrin.h>
#include <algorithm>
#include <cmath>

namespace tachyon::renderer2d {

void MaskRasterizer::rasterize(
    int width, int height,
    const std::vector<Contour>& contours,
    const Config& cfg,
    float* out_alpha) 
{
    if (width <= 0 || height <= 0 || !out_alpha) return;
    std::fill(out_alpha, out_alpha + (static_cast<size_t>(width) * height), 0.0f);

    std::vector<float> coverage_buffer(width + 1, 0.0f);

    for (int y = 0; y < height; ++y) {
        std::fill(coverage_buffer.begin(), coverage_buffer.end(), 0.0f);

        for (const auto& contour : contours) {
            for (size_t i = 0; i + 1 < contour.points.size(); ++i) {
                const auto& p1 = contour.points[i];
                const auto& p2 = contour.points[i + 1];

                float y1 = p1.y, y2 = p2.y;
                if (std::max(y1, y2) <= (float)y || std::min(y1, y2) >= (float)y + 1) continue;

                float sy1 = std::max(std::min(y1, y2), (float)y);
                float sy2 = std::min(std::max(y1, y2), (float)y + 1);
                
                auto get_x = [&](float py) {
                    return p1.x + (p2.x - p1.x) * (py - y1) / (y2 - y1);
                };

                float x1 = get_x(sy1);
                float x2 = get_x(sy2);
                float h = (y1 > y2) ? -(sy2 - sy1) : (sy2 - sy1);

                const int ix1 = static_cast<int>(std::floor(std::min(x1, x2)));
                const int ix2 = static_cast<int>(std::floor(std::max(x1, x2)));

                if (ix1 == ix2) {
                    coverage_buffer[std::clamp(ix1, 0, width)] += h * (1.0f - (x1 + x2) * 0.5f + static_cast<float>(ix1));
                } else {
                    // Multi-pixel segment
                    float slope = (x2 - x1) / (sy2 - sy1);
                    float dir = (y2 > y1) ? 1.0f : -1.0f;
                    // Simplify for mask
                    coverage_buffer[std::clamp(ix1, 0, width)] += h * 0.5f; 
                }
                if (std::max(0, ix2 + 1) < width) coverage_buffer[std::max(0, ix2 + 1)] += h;
            }
        }

        float winding = 0.0f;
        float* row_alpha = &out_alpha[static_cast<size_t>(y) * width];

#ifdef TACHYON_AVX2
        int x = 0;
        __m256 v_winding = _mm256_set1_ps(0.0f);
        __m256 v_one = _mm256_set1_ps(1.0f);

        for (; x <= width - 8; x += 8) {
            __m256 v_cov = _mm256_loadu_ps(&coverage_buffer[x]);
            
            // Prefix sum within 256-bit register
            v_cov = _mm256_add_ps(v_cov, _mm256_castsi256_ps(_mm256_slli_si256(_mm256_castps_si256(v_cov), 4)));
            v_cov = _mm256_add_ps(v_cov, _mm256_castsi256_ps(_mm256_slli_si256(_mm256_castps_si256(v_cov), 8)));
            __m256 v_low = _mm256_permute2f128_ps(v_cov, v_cov, 0x00);
            __m256 v_high = _mm256_permute2f128_ps(v_cov, v_cov, 0x11);
            __m256 v_last_low = _mm256_shuffle_ps(v_low, v_low, _MM_SHUFFLE(3, 3, 3, 3));
            v_high = _mm256_add_ps(v_high, v_last_low);
            v_cov = _mm256_blend_ps(v_low, v_high, 0xF0);
            
            __m256 v_curr_winding = _mm256_add_ps(v_cov, v_winding);
            
            // Extract last winding for next iteration
            __m128 v_upper = _mm256_extractf128_ps(v_curr_winding, 1);
            float last_w = _mm_cvtss_f32(_mm_shuffle_ps(v_upper, v_upper, _MM_SHUFFLE(3, 3, 3, 3)));
            v_winding = _mm256_set1_ps(last_w);

            __m256 v_abs_w = _mm256_andnot_ps(_mm256_set1_ps(-0.0f), v_curr_winding);
            __m256 v_res_cov;
            if (cfg.winding == WindingRule::NonZero) {
                v_res_cov = _mm256_min_ps(v_one, v_abs_w);
            } else {
                // Even-Odd
                __m256 v_w_div_2 = _mm256_mul_ps(v_abs_w, _mm256_set1_ps(0.5f));
                __m256 v_w_floor = _mm256_floor_ps(v_w_div_2);
                __m256 v_rem = _mm256_sub_ps(v_abs_w, _mm256_mul_ps(v_w_floor, _mm256_set1_ps(2.0f)));
                v_res_cov = _mm256_blendv_ps(v_rem, _mm256_sub_ps(_mm256_set1_ps(2.0f), v_rem), _mm256_cmp_ps(v_rem, v_one, _CMP_GT_OQ));
            }
            
            _mm256_storeu_ps(&row_alpha[x], v_res_cov);
        }
        winding = _mm_cvtss_f32(_mm256_extractf128_ps(v_winding, 0)); // Correctly reset winding for tail
#else
        int x = 0;
#endif

        for (; x < width; ++x) {
            winding += coverage_buffer[x];
            float cov = std::abs(winding);
            if (cfg.winding == WindingRule::EvenOdd) {
                float rem = std::fmod(cov, 2.0f);
                cov = (rem > 1.0f) ? 2.0f - rem : rem;
            }
            row_alpha[x] = std::min(1.0f, cov);
        }
    }
}

} // namespace tachyon::renderer2d
