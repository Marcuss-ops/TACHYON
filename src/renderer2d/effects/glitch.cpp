#include "tachyon/renderer2d/effects/glitch.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

namespace tachyon::renderer2d {
namespace {

inline float sample_bilinear(const Framebuffer& fb, float x, float y, int channel) {
    const int width = static_cast<int>(fb.width());
    const int height = static_cast<int>(fb.height());

    x = std::clamp(x, 0.0f, static_cast<float>(width - 1));
    y = std::clamp(y, 0.0f, static_cast<float>(height - 1));

    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const int x1 = std::min(x0 + 1, width - 1);
    const int y1 = std::min(y0 + 1, height - 1);

    const float dx = x - static_cast<float>(x0);
    const float dy = y - static_cast<float>(y0);
    const auto& pixels = fb.pixels();

    const std::size_t idx00 = static_cast<std::size_t>(y0 * width + x0) * 4 + channel;
    const std::size_t idx10 = static_cast<std::size_t>(y0 * width + x1) * 4 + channel;
    const std::size_t idx01 = static_cast<std::size_t>(y1 * width + x0) * 4 + channel;
    const std::size_t idx11 = static_cast<std::size_t>(y1 * width + x1) * 4 + channel;

    const float v00 = pixels[idx00];
    const float v10 = pixels[idx10];
    const float v01 = pixels[idx01];
    const float v11 = pixels[idx11];

    const float v0 = v00 * (1.0f - dx) + v10 * dx;
    const float v1 = v01 * (1.0f - dx) + v11 * dx;
    return v0 * (1.0f - dy) + v1 * dy;
}

} // namespace

void apply_glitch(Framebuffer& fb, const GlitchEffect& params, int frame_number) {
    if (params.intensity <= 0.0f) {
        return;
    }

    const int width = static_cast<int>(fb.width());
    const int height = static_cast<int>(fb.height());
    if (width <= 0 || height <= 0) {
        return;
    }

    std::mt19937 rng(params.seed ^ static_cast<uint64_t>(frame_number));
    std::uniform_real_distribution<float> dist_01(0.0f, 1.0f);
    std::uniform_int_distribution<int> dist_shift(-20, 20);
    std::uniform_real_distribution<float> dist_rgb_shift(-params.rgb_shift_px, params.rgb_shift_px);

    std::vector<float> original_data(fb.pixels().begin(), fb.pixels().end());
    Framebuffer temp_fb;
    temp_fb.reset(fb.width(), fb.height());
    temp_fb.mutable_pixels() = std::move(original_data);

    const float block_height = std::max(4.0f, params.block_size);
    auto& dst = fb.mutable_pixels();

    int y = 0;
    while (y < height) {
        const int stripe_height = static_cast<int>(block_height * (0.5f + dist_01(rng) * 1.5f));
        const int stripe_end = std::min(y + stripe_height, height);

        if (dist_01(rng) < params.intensity) {
            const int x_shift = dist_shift(rng);
            const float r_shift = dist_rgb_shift(rng);
            const float b_shift = dist_rgb_shift(rng);

            for (int row = y; row < stripe_end; ++row) {
                for (int x = 0; x < width; ++x) {
                    const std::size_t idx = static_cast<std::size_t>(row * width + x) * 4;
                    const int src_x = std::clamp(x + x_shift, 0, width - 1);
                    const std::size_t src_idx = static_cast<std::size_t>(row * width + src_x) * 4;

                    dst[idx + 0] = sample_bilinear(temp_fb,
                                                   static_cast<float>(src_x) + r_shift,
                                                   static_cast<float>(row),
                                                   0);
                    dst[idx + 1] = temp_fb.pixels()[src_idx + 1];
                    dst[idx + 2] = sample_bilinear(temp_fb,
                                                   static_cast<float>(src_x) + b_shift,
                                                   static_cast<float>(row),
                                                   2);
                    dst[idx + 3] = temp_fb.pixels()[src_idx + 3];
                }
            }
        }

        if (dist_01(rng) < params.scan_line_chance * params.intensity) {
            for (int row = y; row < stripe_end; ++row) {
                for (int x = 0; x < width; ++x) {
                    const std::size_t idx = static_cast<std::size_t>(row * width + x) * 4;
                    dst[idx + 0] = dist_01(rng);
                    dst[idx + 1] = dist_01(rng);
                    dst[idx + 2] = dist_01(rng);
                    dst[idx + 3] = 1.0f;
                }
            }
        }

        y = stripe_end;
    }
}

} // namespace tachyon::renderer2d
