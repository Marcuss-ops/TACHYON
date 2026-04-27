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
    // Atlas-based storage (replaces per-glyph vector allocation)
    const std::uint8_t* atlas_data{nullptr}; // Pointer to parent atlas buffer
    std::uint32_t atlas_x{0}; // X offset in atlas
    std::uint32_t atlas_y{0}; // Y offset in atlas
    std::uint32_t atlas_stride{0}; // Atlas row stride (width of atlas)
    GlyphType type{GlyphType::AlphaMask};
    
    // Temporary storage for loading (transfer to atlas after load)
    std::vector<std::uint8_t> pixels; // Used during loading from BDF

    // Helper to access alpha value at (x,y) within glyph
    std::uint8_t alpha_at(std::uint32_t x, std::uint32_t y) const {
        if (atlas_data && x < width && y < height) {
            return atlas_data[(atlas_y + y) * atlas_stride + (atlas_x + x)];
        }
        if (x < width && y < height && y * width + x < pixels.size()) {
            return pixels[y * width + x];
        }
        return 0;
    }
};

} // namespace tachyon::renderer2d::text

namespace tachyon::text {
    using GlyphBitmap = renderer2d::text::GlyphBitmap;
}
