#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

namespace tachyon {
namespace renderer2d {

struct Color {
    float r{0.0f};
    float g{0.0f};
    float b{0.0f};
    float a{0.0f};

    static Color transparent() { return {0.0f, 0.0f, 0.0f, 0.0f}; }
    static Color black()       { return {0.0f, 0.0f, 0.0f, 1.0f}; }
    static Color white()       { return {1.0f, 1.0f, 1.0f, 1.0f}; }
    static Color red()         { return {1.0f, 0.0f, 0.0f, 1.0f}; }
    static Color green()       { return {0.0f, 1.0f, 0.0f, 1.0f}; }
    static Color blue() { return {0.0f, 0.0f, 1.0f, 1.0f}; } static Color lerp(const Color& a, const Color& b, float t) { return { a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t }; }
};

struct RectI {
    int x{0};
    int y{0};
    int width{0};
    int height{0};
};

class SurfaceRGBA {
public:
    SurfaceRGBA() = default;
    SurfaceRGBA(uint32_t width, uint32_t height);

    void reset(uint32_t width, uint32_t height);

    void clear(Color color);
    void clear_depth(float depth = 0.0f);

    bool set_clip_rect(const RectI& rect);
    void reset_clip_rect();
    RectI clip_rect() const;

    bool set_pixel(uint32_t x, uint32_t y, Color color);
    bool set_pixel_with_depth(uint32_t x, uint32_t y, Color color, float inv_z);
    bool blend_pixel(uint32_t x, uint32_t y, Color color);
    bool blend_pixel(uint32_t x, uint32_t y, Color color, float alpha);
    bool test_and_write_depth(uint32_t x, uint32_t y, float inv_z);

    std::optional<Color> try_get_pixel(uint32_t x, uint32_t y) const;
    Color get_pixel(uint32_t x, uint32_t y) const;
    float get_depth(uint32_t x, uint32_t y) const;

    void fill_rect(const RectI& rect, Color color, bool blend = true);

    bool save_png(const std::filesystem::path& path) const;

    uint32_t width() const { return m_width; }
    uint32_t height() const { return m_height; }
    const std::vector<float>& pixels() const { return m_pixels; }
    std::vector<float>& mutable_pixels() { return m_pixels; }
    const std::vector<float>& depth_buffer() const { return m_depth_buffer; }

private:
    bool in_bounds(uint32_t x, uint32_t y) const;
    bool in_clip(uint32_t x, uint32_t y) const;
    static Color blend_src_over_premultiplied(Color src_straight, Color dst_straight);
    RectI intersect_rects(const RectI& a, const RectI& b) const;

    uint32_t m_width{0};
    uint32_t m_height{0};
    std::vector<float> m_pixels;
    std::vector<float> m_depth_buffer;
    RectI m_clip_rect{0, 0, 0, 0};
};

using Framebuffer = SurfaceRGBA;

} // namespace renderer2d
} // namespace tachyon
