#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <filesystem>

namespace tachyon {
namespace renderer2d {

struct Color {
    uint8_t r{0};
    uint8_t g{0};
    uint8_t b{0};
    uint8_t a{255};

    static Color black() { return {0, 0, 0, 255}; }
    static Color white() { return {255, 255, 255, 255}; }
    static Color red()   { return {255, 0, 0, 255}; }
    static Color green() { return {0, 255, 0, 255}; }
    static Color blue()  { return {0, 0, 255, 255}; }
};

class Framebuffer {
public:
    Framebuffer(uint32_t width, uint32_t height);

    void clear(Color color);
    void set_pixel(uint32_t x, uint32_t y, Color color);
    Color get_pixel(uint32_t x, uint32_t y) const;

    bool save_png(const std::filesystem::path& path) const;

    uint32_t width() const { return m_width; }
    uint32_t height() const { return m_height; }

private:
    uint32_t m_width;
    uint32_t m_height;
    std::vector<uint32_t> m_pixels; // Pack as 0xAABBGGRR or similar for STB
};

} // namespace renderer2d
} // namespace tachyon
