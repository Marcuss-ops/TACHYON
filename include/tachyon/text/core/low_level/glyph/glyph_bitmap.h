#pragma once

#include <cstdint>
#include <vector>

namespace tachyon::renderer2d::text {

struct GlyphBitmap {
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::int32_t x_offset{0};
    std::int32_t y_offset{0};
    std::int32_t advance_x{0};
    std::vector<std::uint8_t> alpha_mask;

    // Atlas-based members (optional, for shared texture caching)
    const std::uint8_t* atlas_data{nullptr};
    std::uint32_t atlas_x{0};
    std::uint32_t atlas_y{0};
    std::uint32_t atlas_stride{0};
};

} // namespace tachyon::renderer2d::text

namespace tachyon::text {
    using GlyphBitmap = renderer2d::text::GlyphBitmap;
}
