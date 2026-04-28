#include "tachyon/renderer2d/color/color_transfer.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace tachyon::output {

std::vector<unsigned char> convert_and_pack_ffmpeg_frame(
    const renderer2d::Framebuffer& frame,
    uint32_t target_width,
    uint32_t target_height,
    renderer2d::detail::TransferCurve source_curve,
    renderer2d::detail::ColorSpace source_space,
    renderer2d::detail::TransferCurve output_curve,
    renderer2d::detail::ColorSpace output_space,
    renderer2d::detail::ColorRange output_range) {

    const uint32_t src_w = frame.width();
    const uint32_t src_h = frame.height();
    std::vector<unsigned char> bytes;
    bytes.resize(static_cast<std::size_t>(target_width) * static_cast<std::size_t>(target_height) * 4U);
    
    const float scale_x = static_cast<float>(src_w) / static_cast<float>(target_width);
    const float scale_y = static_cast<float>(src_h) / static_cast<float>(target_height);
    
    const auto& pixels = frame.pixels();
    const uint32_t pixel_stride = 4;

    for (std::uint32_t y = 0; y < target_height; ++y) {
        for (std::uint32_t x = 0; x < target_width; ++x) {
            renderer2d::Color pixel;
            
            if (src_w == target_width && src_h == target_height) {
                const std::size_t idx = (static_cast<std::size_t>(y) * src_w + x) * pixel_stride;
                pixel = {pixels[idx + 0], pixels[idx + 1], pixels[idx + 2], pixels[idx + 3]};
            } else {
                const float gx = (static_cast<float>(x) + 0.5f) * scale_x - 0.5f;
                const float gy = (static_cast<float>(y) + 0.5f) * scale_y - 0.5f;
                const int gxi = static_cast<int>(std::floor(gx));
                const int gyi = static_cast<int>(std::floor(gy));
                const float tx = gx - static_cast<float>(gxi);
                const float ty = gy - static_cast<float>(gyi);

                auto sample = [&](int sx, int sy) {
                    const uint32_t si = static_cast<uint32_t>(std::clamp(sx, 0, static_cast<int>(src_w) - 1));
                    const uint32_t sj = static_cast<uint32_t>(std::clamp(sy, 0, static_cast<int>(src_h) - 1));
                    const std::size_t idx = (static_cast<std::size_t>(sj) * src_w + si) * pixel_stride;
                    return renderer2d::Color{pixels[idx + 0], pixels[idx + 1], pixels[idx + 2], pixels[idx + 3]};
                };

                const auto c00 = sample(gxi, gyi);
                const auto c10 = sample(gxi + 1, gyi);
                const auto c01 = sample(gxi, gyi + 1);
                const auto c11 = sample(gxi + 1, gyi + 1);

                const auto top = renderer2d::Color::lerp(c00, c10, tx);
                const auto bottom = renderer2d::Color::lerp(c01, c11, tx);
                pixel = renderer2d::Color::lerp(top, bottom, ty);
            }

            pixel = renderer2d::detail::convert_color(
                pixel,
                source_curve, source_space,
                output_curve, output_space);
            pixel = renderer2d::detail::apply_range_mode(pixel, output_range);
            
            const std::size_t byte_idx = (static_cast<std::size_t>(y) * target_width + x) * 4U;
            bytes[byte_idx + 0] = static_cast<unsigned char>(std::clamp(pixel.r * 255.0f, 0.0f, 255.0f));
            bytes[byte_idx + 1] = static_cast<unsigned char>(std::clamp(pixel.g * 255.0f, 0.0f, 255.0f));
            bytes[byte_idx + 2] = static_cast<unsigned char>(std::clamp(pixel.b * 255.0f, 0.0f, 255.0f));
            bytes[byte_idx + 3] = static_cast<unsigned char>(std::clamp(pixel.a * 255.0f, 0.0f, 255.0f));
        }
    }

    return bytes;
}

} // namespace tachyon::output
