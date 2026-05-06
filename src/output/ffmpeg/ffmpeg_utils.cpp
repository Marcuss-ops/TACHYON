#include "tachyon/renderer2d/color/color_transform_graph.h"
#include "tachyon/renderer2d/color/color_transfer.h"
#include "tachyon/math/sampling.h"
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

    renderer2d::ColorProfile src_profile;
    src_profile.curve = source_curve;
    src_profile.primaries = source_space;

    renderer2d::ColorProfile dst_profile;
    dst_profile.curve = output_curve;
    dst_profile.primaries = output_space;

    renderer2d::ColorTransformGraph graph;
    graph.build_from_to(src_profile, dst_profile);

    const uint32_t src_w = frame.width();
    const uint32_t src_h = frame.height();
    std::vector<unsigned char> bytes;
    bytes.reserve(static_cast<std::size_t>(target_width) * static_cast<std::size_t>(target_height) * 4U);

    const float scale_x = static_cast<float>(src_w) / static_cast<float>(target_width);
    const float scale_y = static_cast<float>(src_h) / static_cast<float>(target_height);

    for (std::uint32_t y = 0; y < target_height; ++y) {
        for (std::uint32_t x = 0; x < target_width; ++x) {
            renderer2d::Color pixel;
            
            if (src_w == target_width && src_h == target_height) {
                pixel = frame.get_pixel(x, y);
            } else {
                const float gx = (static_cast<float>(x) + 0.5f) * scale_x - 0.5f;
                const float gy = (static_cast<float>(y) + 0.5f) * scale_y - 0.5f;

                pixel = math::sample_bilinear(gx, gy, [&](int sx, int sy) {
                    return frame.get_pixel(
                        static_cast<uint32_t>(math::clamp_coord(sx, static_cast<int>(src_w))),
                        static_cast<uint32_t>(math::clamp_coord(sy, static_cast<int>(src_h)))
                    );
                });
            }

            pixel = graph.process(pixel);
            pixel = renderer2d::detail::apply_range_mode(pixel, output_range);
            
            bytes.push_back(static_cast<unsigned char>(std::clamp(pixel.r * 255.0f, 0.0f, 255.0f)));
            bytes.push_back(static_cast<unsigned char>(std::clamp(pixel.g * 255.0f, 0.0f, 255.0f)));
            bytes.push_back(static_cast<unsigned char>(std::clamp(pixel.b * 255.0f, 0.0f, 255.0f)));
            bytes.push_back(static_cast<unsigned char>(std::clamp(pixel.a * 255.0f, 0.0f, 255.0f)));
        }
    }

    return bytes;
}

} // namespace tachyon::output
