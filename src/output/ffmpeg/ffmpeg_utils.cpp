#include "tachyon/renderer2d/color/color_transform_graph.h"
#include "tachyon/renderer2d/color/color_transfer.h"
#include "tachyon/math/sampling.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace tachyon::output {

void convert_and_pack_ffmpeg_frame(
    std::vector<unsigned char>& out_bytes,
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
    
    out_bytes.resize(static_cast<std::size_t>(target_width) * target_height * 4U);
    unsigned char* out_ptr = out_bytes.data();
    const float* src_ptr = frame.data();

    // Fast path: dimensions match exactly, no color transformation needed
    if (src_w == target_width && src_h == target_height &&
        source_curve == output_curve && source_space == output_space &&
        output_range == renderer2d::detail::ColorRange::Full) {
        
        const std::size_t num_pixels = static_cast<std::size_t>(target_width) * target_height;
        for (std::size_t i = 0; i < num_pixels; ++i) {
            const std::size_t idx = i * 4U;
            out_ptr[idx]     = static_cast<unsigned char>(std::clamp(src_ptr[idx] * 255.0f, 0.0f, 255.0f));
            out_ptr[idx + 1] = static_cast<unsigned char>(std::clamp(src_ptr[idx + 1] * 255.0f, 0.0f, 255.0f));
            out_ptr[idx + 2] = static_cast<unsigned char>(std::clamp(src_ptr[idx + 2] * 255.0f, 0.0f, 255.0f));
            out_ptr[idx + 3] = static_cast<unsigned char>(std::clamp(src_ptr[idx + 3] * 255.0f, 0.0f, 255.0f));
        }
        return;
    }

    renderer2d::ColorProfile src_profile;
    src_profile.curve = source_curve;
    src_profile.primaries = source_space;

    renderer2d::ColorProfile dst_profile;
    dst_profile.curve = output_curve;
    dst_profile.primaries = output_space;

    renderer2d::ColorTransformGraph graph;
    graph.build_from_to(src_profile, dst_profile);

    const float scale_x = static_cast<float>(src_w) / static_cast<float>(target_width);
    const float scale_y = static_cast<float>(src_h) / static_cast<float>(target_height);

    auto get_pixel_fast = [&](uint32_t px, uint32_t py) -> renderer2d::Color {
        const std::size_t idx = (static_cast<std::size_t>(py) * src_w + px) * 4U;
        return renderer2d::Color{src_ptr[idx], src_ptr[idx + 1], src_ptr[idx + 2], src_ptr[idx + 3]};
    };

    std::size_t dst_idx = 0;
    for (std::uint32_t y = 0; y < target_height; ++y) {
        for (std::uint32_t x = 0; x < target_width; ++x) {
            renderer2d::Color pixel;
            
            if (src_w == target_width && src_h == target_height) {
                pixel = get_pixel_fast(x, y);
            } else {
                const float gx = (static_cast<float>(x) + 0.5f) * scale_x - 0.5f;
                const float gy = (static_cast<float>(y) + 0.5f) * scale_y - 0.5f;

                pixel = math::sample_bilinear(gx, gy, [&](int sx, int sy) {
                    return get_pixel_fast(
                        static_cast<uint32_t>(math::clamp_coord(sx, static_cast<int>(src_w))),
                        static_cast<uint32_t>(math::clamp_coord(sy, static_cast<int>(src_h)))
                    );
                });
            }

            pixel = graph.process(pixel);
            pixel = renderer2d::detail::apply_range_mode(pixel, output_range);
            
            out_ptr[dst_idx]     = static_cast<unsigned char>(std::clamp(pixel.r * 255.0f, 0.0f, 255.0f));
            out_ptr[dst_idx + 1] = static_cast<unsigned char>(std::clamp(pixel.g * 255.0f, 0.0f, 255.0f));
            out_ptr[dst_idx + 2] = static_cast<unsigned char>(std::clamp(pixel.b * 255.0f, 0.0f, 255.0f));
            out_ptr[dst_idx + 3] = static_cast<unsigned char>(std::clamp(pixel.a * 255.0f, 0.0f, 255.0f));
            dst_idx += 4;
        }
    }
}

} // namespace tachyon::output
