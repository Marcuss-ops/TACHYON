#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

#include "tachyon/renderer2d/color/color_profile.h"

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
    void blend_row(uint32_t x, uint32_t y, const Color* src_colors, size_t count);
    bool test_and_write_depth(uint32_t x, uint32_t y, float inv_z);

    std::optional<Color> try_get_pixel(uint32_t x, uint32_t y) const;
    Color get_pixel(uint32_t x, uint32_t y) const;
    float get_depth(uint32_t x, uint32_t y) const;

    void fill_rect(const RectI& rect, Color color, bool blend = true);
    void blit(const SurfaceRGBA& src, int x, int y);

    bool save_png(const std::filesystem::path& path) const;
    bool save_png(const std::filesystem::path& path, TransferCurve transfer_curve) const;


    uint32_t width() const { return m_width; }
    uint32_t height() const { return m_height; }
    const std::vector<float>& pixels() const { return m_pixels; }
    std::vector<float>& mutable_pixels() { return m_pixels; }
    const std::vector<float>& depth_buffer() const { return m_depth_buffer; }

    ColorProfile profile() const { return m_profile; }
    void set_profile(ColorProfile profile) { m_profile = profile; }

private:
    bool in_bounds(uint32_t x, uint32_t y) const;
    bool in_clip(uint32_t x, uint32_t y) const;
    static Color blend_src_over_premultiplied(Color src_straight, Color dst_straight);
    RectI intersect_rects(const RectI& a, const RectI& b) const;

    uint32_t m_width{0};
    uint32_t m_height{0};
    ColorProfile m_profile{ColorProfile::sRGB()};
    std::vector<float> m_pixels;
    std::vector<float> m_depth_buffer;
    RectI m_clip_rect{0, 0, 0, 0};
};

using Framebuffer = SurfaceRGBA;

// ---------------------------------------------------------------------------
// FramebufferRGBA8 – 8-bit per channel (32-bit total)
// ---------------------------------------------------------------------------
class FramebufferRGBA8 {
public:
    FramebufferRGBA8() = default;
    FramebufferRGBA8(uint32_t width, uint32_t height);

    void reset(uint32_t width, uint32_t height);

    uint32_t width() const { return m_width; }
    uint32_t height() const { return m_height; }
    const std::vector<uint8_t>& pixels() const { return m_pixels; }
    std::vector<uint8_t>& mutable_pixels() { return m_pixels; }

    bool set_pixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    Color get_pixel(uint32_t x, uint32_t y) const;

    void clear(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 0);

    bool save_png(const std::filesystem::path& path) const;

private:
    bool in_bounds(uint32_t x, uint32_t y) const;

    uint32_t m_width{0};
    uint32_t m_height{0};
    std::vector<uint8_t> m_pixels; // RGBA interleaved
};

// ---------------------------------------------------------------------------
// FramebufferRGBA16 – 16-bit per channel (64-bit total)
// ---------------------------------------------------------------------------
class FramebufferRGBA16 {
public:
    FramebufferRGBA16() = default;
    FramebufferRGBA16(uint32_t width, uint32_t height);

    void reset(uint32_t width, uint32_t height);

    uint32_t width() const { return m_width; }
    uint32_t height() const { return m_height; }
    const std::vector<uint16_t>& pixels() const { return m_pixels; }
    std::vector<uint16_t>& mutable_pixels() { return m_pixels; }

    bool set_pixel(uint32_t x, uint32_t y, uint16_t r, uint16_t g, uint16_t b, uint16_t a);
    Color get_pixel(uint32_t x, uint32_t y) const;

    void clear(uint16_t r = 0, uint16_t g = 0, uint16_t b = 0, uint16_t a = 0);

    bool save_png(const std::filesystem::path& path) const;

private:
    bool in_bounds(uint32_t x, uint32_t y) const;

    uint32_t m_width{0};
    uint32_t m_height{0};
    std::vector<uint16_t> m_pixels; // RGBA interleaved
};

} // namespace renderer2d
} // namespace tachyon
