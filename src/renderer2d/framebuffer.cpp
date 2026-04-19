#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "tachyon/renderer2d/framebuffer.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace tachyon {
namespace renderer2d {

Framebuffer::Framebuffer(uint32_t width, uint32_t height)
    : m_width(width), m_height(height) {
    m_pixels.resize(width * height, 0xFF000000); // Initialize with opaque black
}

void Framebuffer::clear(Color color) {
    uint32_t packed = (static_cast<uint32_t>(color.a) << 24) |
                      (static_cast<uint32_t>(color.b) << 16) |
                      (static_cast<uint32_t>(color.g) << 8) |
                      (static_cast<uint32_t>(color.r));
    std::fill(m_pixels.begin(), m_pixels.end(), packed);
}

void Framebuffer::set_pixel(uint32_t x, uint32_t y, Color color) {
    if (x >= m_width || y >= m_height) return;
    
    uint32_t packed = (static_cast<uint32_t>(color.a) << 24) |
                      (static_cast<uint32_t>(color.b) << 16) |
                      (static_cast<uint32_t>(color.g) << 8) |
                      (static_cast<uint32_t>(color.r));
    m_pixels[y * m_width + x] = packed;
}

Color Framebuffer::get_pixel(uint32_t x, uint32_t y) const {
    if (x >= m_width || y >= m_height) return Color::black();
    
    uint32_t packed = m_pixels[y * m_width + x];
    Color color;
    color.r = static_cast<uint8_t>(packed & 0xFF);
    color.g = static_cast<uint8_t>((packed >> 8) & 0xFF);
    color.b = static_cast<uint8_t>((packed >> 16) & 0xFF);
    color.a = static_cast<uint8_t>((packed >> 24) & 0xFF);
    return color;
}

bool Framebuffer::save_png(const std::filesystem::path& path) const {
    std::string path_str = path.string();
    // stb_write_png expects 4 components (RGBA) and interleaved bytes.
    // Our uint32_t layout (AABBGGRR) on little-endian matches [R, G, B, A] in memory.
    int result = stbi_write_png(path_str.c_str(), m_width, m_height, 4, m_pixels.data(), m_width * 4);
    return result != 0;
}

} // namespace renderer2d
} // namespace tachyon
