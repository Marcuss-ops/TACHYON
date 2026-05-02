#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/color/color_transfer.h"
#include <vector>
#include <cstdint>

namespace tachyon::output {

/**
 * @brief Convert and pack a framebuffer into a byte array for FFmpeg encoding.
 * 
 * Handles:
 * - Resizing from source to target dimensions (with bilinear interpolation)
 * - Color space conversion (via convert_color)
 * - Transfer curve application
 * - Color range conversion (limited vs full)
 * - Packing into RGBA byte format
 * 
 * @param frame Source framebuffer
 * @param target_width Desired output width
 * @param target_height Desired output height
 * @param source_curve Transfer curve of the source frame
 * @param source_space Color space of the source frame
 * @param output_curve Desired output transfer curve
 * @param output_space Desired output color space
 * @param output_range Desired output color range (limited/full)
 * @return Vector of bytes in RGBA order
 */
std::vector<unsigned char> convert_and_pack_ffmpeg_frame(
    const renderer2d::Framebuffer& frame,
    uint32_t target_width,
    uint32_t target_height,
    renderer2d::detail::TransferCurve source_curve,
    renderer2d::detail::ColorSpace source_space,
    renderer2d::detail::TransferCurve output_curve,
    renderer2d::detail::ColorSpace output_space,
    renderer2d::detail::ColorRange output_range);

} // namespace tachyon::output
