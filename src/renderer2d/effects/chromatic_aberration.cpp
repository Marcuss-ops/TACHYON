#include "tachyon/renderer2d/effects/chromatic_aberration.h"
#include <cmath>
#include <algorithm>

namespace tachyon::renderer2d {
namespace {

// Bilinear sampling helper
inline float sample_bilinear(const Framebuffer& fb, float x, float y, int channel) {
    const int width = static_cast<int>(fb.width);
    const int height = static_cast<int>(fb.height);
    
    // Clamp coordinates
    x = std::clamp(x, 0.0f, static_cast<float>(width - 1));
    y = std::clamp(y, 0.0f, static_cast<float>(height - 1));
    
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const int x1 = std::min(x0 + 1, width - 1);
    const int y1 = std::min(y0 + 1, height - 1);
    
    const float dx = x - static_cast<float>(x0);
    const float dy = y - static_cast<float>(y0);
    
    // Sample 4 corners for this channel (RGBA: 4 channels)
    const std::size_t idx00 = static_cast<std::size_t>(y0 * width + x0) * 4 + channel;
    const std::size_t idx10 = static_cast<std::size_t>(y0 * width + x1) * 4 + channel;
    const std::size_t idx01 = static_cast<std::size_t>(y1 * width + x0) * 4 + channel;
    const std::size_t idx11 = static_cast<std::size_t>(y1 * width + x1) * 4 + channel;
    
    // Bilinear interpolation
    const float v00 = fb.data[idx00];
    const float v10 = fb.data[idx10];
    const float v01 = fb.data[idx01];
    const float v11 = fb.data[idx11];
    
    const float v0 = v00 * (1.0f - dx) + v10 * dx;
    const float v1 = v01 * (1.0f - dx) + v11 * dx;
    
    return v0 * (1.0f - dy) + v1 * dy;
}

} // namespace

void apply_chromatic_aberration(Framebuffer& fb, const ChromaticAberrationEffect& params) {
    if (params.offset_pixels <= 0.0f) {
        return; // No effect
    }
    
    const int width = static_cast<int>(fb.width);
    const int height = static_cast<int>(fb.height);
    
    // Converti angolo in radianti e calcola direzione
    const float angle_rad = params.angle_deg * static_cast<float>(std::acos(-1.0)) / 180.0f;
    const float dx = params.offset_pixels * std::cos(angle_rad);
    const float dy = params.offset_pixels * std::sin(angle_rad);
    
    // Crea un buffer temporaneo per leggere i dati originali
    std::vector<float> original_data(fb.data.begin(), fb.data.end());
    Framebuffer temp_fb;
    temp_fb.width = fb.width;
    temp_fb.height = fb.height;
    temp_fb.data = std::move(original_data);
    
    // Per ogni pixel, campiona R, G, B da posizioni leggermente diverse
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const std::size_t idx = static_cast<std::size_t>(y * width + x) * 4;
            
            // R → campiona da (x + dx, y + dy)
            fb.data[idx + 0] = sample_bilinear(temp_fb, 
                static_cast<float>(x) + dx, 
                static_cast<float>(y) + dy, 0);
            
            // G → campiona da (x, y) - nessun offset
            fb.data[idx + 1] = temp_fb.data[idx + 1];
            
            // B → campiona da (x - dx, y - dy)
            fb.data[idx + 2] = sample_bilinear(temp_fb, 
                static_cast<float>(x) - dx, 
                static_cast<float>(y) - dy, 2);
            
            // Alpha rimane invariato
            fb.data[idx + 3] = temp_fb.data[idx + 3];
        }
    }
}

} // namespace tachyon::renderer2d
