#pragma once

#include <cstdint>
#include <vector>

namespace tachyon::renderer2d::text {

enum class GlyphType {
    AlphaMask,
    SDF
};

struct GlyphBitmap {
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::int32_t x_offset{0};
    std::int32_t y_offset{0};
    std::int32_t advance_x{0};
    std::vector<std::uint8_t> alpha_mask;
    GlyphType type{GlyphType::AlphaMask};
};

} // namespace tachyon::renderer2d::text

namespace tachyon::text {
    using GlyphBitmap = renderer2d::text::GlyphBitmap;
}
