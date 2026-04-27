#include "tachyon/text/rendering/text_raster_surface.h"
#include "tachyon/text/layout/layout.h"
#include <stb_image_write.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include <numeric>

namespace tachyon::text {

namespace {
 
float sample_glyph_alpha(const tachyon::text::GlyphBitmap& glyph, float src_x, float src_y) {
    if (glyph.width == 0U || glyph.height == 0U || (glyph.atlas_data == nullptr && glyph.pixels.empty())) {
        return 0.0f;
    }

    const float max_x = static_cast<float>(glyph.width - 1U);
    const float max_y = static_cast<float>(glyph.height - 1U);
    src_x = std::clamp(src_x, 0.0f, max_x);
    src_y = std::clamp(src_y, 0.0f, max_y);

    const std::uint32_t x0 = static_cast<std::uint32_t>(std::floor(src_x));
    const std::uint32_t y0 = static_cast<std::uint32_t>(std::floor(src_y));
    const std::uint32_t x1 = std::min(x0 + 1U, glyph.width - 1U);
    const std::uint32_t y1 = std::min(y0 + 1U, glyph.height - 1U);

    const float fx = src_x - static_cast<float>(x0);
    const float fy = src_y - static_cast<float>(y0);

    auto alpha_at = [&](std::uint32_t x, std::uint32_t y) -> float {
        return static_cast<float>(glyph.alpha_at(x, y)) / 255.0f;
    };

    const float a00 = alpha_at(x0, y0);
    const float a10 = alpha_at(x1, y0);
    const float a01 = alpha_at(x0, y1);
    const float a11 = alpha_at(x1, y1);

    const float ax0 = a00 + (a10 - a00) * fx;
    const float ax1 = a01 + (a11 - a01) * fx;
    return ax0 + (ax1 - ax0) * fy;
}
 
} // namespace

void TextRasterSurface::render_glyph(const tachyon::text::GlyphBitmap& glyph, int tx, int ty, int tw, int th, tachyon::renderer2d::Color gc) {
    if (tw <= 0 || th <= 0 || glyph.width == 0U || glyph.height == 0U || (glyph.atlas_data == nullptr && glyph.pixels.empty())) return;
    
    if (glyph.type == tachyon::renderer2d::text::GlyphType::SDF) {
        // SDF rendering: alpha is determined by distance from edge (usually 128/255)
        // smoothing helps avoid aliasing
        const float smoothing = 0.25f / (static_cast<float>(tw) / static_cast<float>(glyph.width));

        for (int y = 0; y < th; ++y) {
            const float src_y = ((static_cast<float>(y) + 0.5f) * static_cast<float>(glyph.height) / static_cast<float>(th)) - 0.5f;
            for (int x = 0; x < tw; ++x) {
                const float src_x = ((static_cast<float>(x) + 0.5f) * static_cast<float>(glyph.width) / static_cast<float>(tw)) - 0.5f;
                const float dist = sample_glyph_alpha(glyph, src_x, src_y);
                const float alpha = std::clamp((dist - 0.5f) / smoothing + 0.5f, 0.0f, 1.0f);
                if (alpha > 0.0f) {
                    blend_pixel(static_cast<std::uint32_t>(tx + x), static_cast<std::uint32_t>(ty + y), gc, static_cast<std::uint8_t>(alpha * 255.0f));
                }
            }
        }
    } else {
        // Standard alpha mask rendering
        for (int y = 0; y < th; ++y) {
            const float src_y = ((static_cast<float>(y) + 0.5f) * static_cast<float>(glyph.height) / static_cast<float>(th)) - 0.5f;
            for (int x = 0; x < tw; ++x) {
                const float src_x = ((static_cast<float>(x) + 0.5f) * static_cast<float>(glyph.width) / static_cast<float>(tw)) - 0.5f;
                const float alpha = sample_glyph_alpha(glyph, src_x, src_y);
                if (alpha > 0.0f) {
                    blend_pixel(static_cast<std::uint32_t>(tx + x), static_cast<std::uint32_t>(ty + y), gc, static_cast<std::uint8_t>(std::lround(std::clamp(alpha, 0.0f, 1.0f) * 255.0f)));
                }
            }
        }
    }
}

void TextRasterSurface::draw_rect(int x, int y, int w, int h, tachyon::renderer2d::Color color) {
    if (w <= 0 || h <= 0) return;
    for (int i = 0; i < w; ++i) {
        blend_pixel(static_cast<std::uint32_t>(x + i), static_cast<std::uint32_t>(y), color, 255);
        blend_pixel(static_cast<std::uint32_t>(x + i), static_cast<std::uint32_t>(y + h - 1), color, 255);
    }
    for (int i = 0; i < h; ++i) {
        blend_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y + i), color, 255);
        blend_pixel(static_cast<std::uint32_t>(x + w - 1), static_cast<std::uint32_t>(y + i), color, 255);
    }
}

void TextRasterSurface::draw_line(int x0, int y0, int x1, int y1, tachyon::renderer2d::Color color) {
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    while (true) {
        blend_pixel(static_cast<std::uint32_t>(x0), static_cast<std::uint32_t>(y0), color, 255);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

TextRasterSurface::TextRasterSurface(std::uint32_t width, std::uint32_t height)
    : m_width(width),
      m_height(height),
      m_pixels(width * height * 4U, 0U) {
}

tachyon::renderer2d::Color TextRasterSurface::get_pixel(std::uint32_t x, std::uint32_t y) const {
    if (x >= m_width || y >= m_height) {
        return {};
    }

    const std::size_t index = (static_cast<std::size_t>(y) * m_width + x) * 4U;
    return tachyon::renderer2d::Color{
        static_cast<float>(m_pixels[index + 0U]) / 255.0f,
        static_cast<float>(m_pixels[index + 1U]) / 255.0f,
        static_cast<float>(m_pixels[index + 2U]) / 255.0f,
        static_cast<float>(m_pixels[index + 3U]) / 255.0f
    };
}

void TextRasterSurface::blend_pixel(std::uint32_t x, std::uint32_t y, tachyon::renderer2d::Color color, std::uint8_t alpha) {
    if (x >= m_width || y >= m_height || alpha == 0U) {
        return;
    }

    const std::size_t index = (static_cast<std::size_t>(y) * m_width + x) * 4U;
    const std::uint32_t src_alpha = static_cast<std::uint32_t>(std::clamp(color.a * 255.0f, 0.0f, 255.0f)) * alpha / 255U;
    const std::uint32_t dst_alpha = m_pixels[index + 3U];
    const std::uint32_t out_alpha = src_alpha + ((dst_alpha * (255U - src_alpha)) / 255U);

    auto blend_channel = [&](float src_channel_f, std::size_t channel_index) {
        const std::uint32_t src_channel = static_cast<std::uint32_t>(std::clamp(src_channel_f * 255.0f, 0.0f, 255.0f));
        const std::uint32_t dst_channel = m_pixels[index + channel_index];
        if (out_alpha == 0U) {
            m_pixels[index + channel_index] = 0U;
            return;
        }

        const std::uint32_t src_premultiplied = src_channel * src_alpha;
        const std::uint32_t dst_premultiplied = dst_channel * dst_alpha * (255U - src_alpha) / 255U;
        m_pixels[index + channel_index] = static_cast<std::uint8_t>((src_premultiplied + dst_premultiplied) / out_alpha);
    };

    blend_channel(color.r, 0U);
    blend_channel(color.g, 1U);
    blend_channel(color.b, 2U);
    m_pixels[index + 3U] = static_cast<std::uint8_t>(out_alpha);
}

void TextRasterSurface::apply_gaussian_blur(float radius) {
    if (radius <= 0.0f || m_width == 0U || m_height == 0U) return;

    const int r = static_cast<int>(std::ceil(radius));
    const float sigma = radius;
    const int kernel_size = r * 2 + 1;
    std::vector<float> kernel(kernel_size);
    float sum = 0.0f;

    for (int i = 0; i < kernel_size; ++i) {
        const float x = static_cast<float>(i - r);
        kernel[i] = std::exp(-(x * x) / (2.0f * sigma * sigma));
        sum += kernel[i];
    }
    for (int i = 0; i < kernel_size; ++i) {
        kernel[i] /= sum;
    }

    std::vector<std::uint8_t> temp(m_pixels.size());
    for (std::size_t i = 0; i < m_pixels.size(); ++i) {
        temp[i] = m_pixels[i];
    }

    // Horizontal pass
#ifdef _OPENMP
    #pragma omp parallel for schedule(static)
#endif
    for (int y = 0; y < static_cast<int>(m_height); ++y) {
        for (std::uint32_t x = 0; x < m_width; ++x) {
            float r_acc = 0.0f, g_acc = 0.0f, b_acc = 0.0f, a_acc = 0.0f;
            for (int k = 0; k < kernel_size; ++k) {
                const int sample_x = static_cast<int>(x) + k - r;
                if (sample_x >= 0 && sample_x < static_cast<int>(m_width)) {
                    const std::size_t idx = (static_cast<std::size_t>(y) * m_width + static_cast<std::size_t>(sample_x)) * 4U;
                    const float w = kernel[k];
                    r_acc += temp[idx + 0U] * w;
                    g_acc += temp[idx + 1U] * w;
                    b_acc += temp[idx + 2U] * w;
                    a_acc += temp[idx + 3U] * w;
                }
            }
            const std::size_t idx = (static_cast<std::size_t>(y) * m_width + x) * 4U;
            m_pixels[idx + 0U] = static_cast<std::uint8_t>(std::clamp(r_acc, 0.0f, 255.0f));
            m_pixels[idx + 1U] = static_cast<std::uint8_t>(std::clamp(g_acc, 0.0f, 255.0f));
            m_pixels[idx + 2U] = static_cast<std::uint8_t>(std::clamp(b_acc, 0.0f, 255.0f));
            m_pixels[idx + 3U] = static_cast<std::uint8_t>(std::clamp(a_acc, 0.0f, 255.0f));
        }
    }

    for (std::size_t i = 0; i < m_pixels.size(); ++i) {
        temp[i] = m_pixels[i];
    }

    // Vertical pass
#ifdef _OPENMP
    #pragma omp parallel for schedule(static)
#endif
    for (int x = 0; x < static_cast<int>(m_width); ++x) {
        for (std::uint32_t y = 0; y < m_height; ++y) {
            float r_acc = 0.0f, g_acc = 0.0f, b_acc = 0.0f, a_acc = 0.0f;
            for (int k = 0; k < kernel_size; ++k) {
                const int sample_y = static_cast<int>(y) + k - r;
                if (sample_y >= 0 && sample_y < static_cast<int>(m_height)) {
                    const std::size_t idx = (static_cast<std::size_t>(sample_y) * m_width + static_cast<std::size_t>(x)) * 4U;
                    const float w = kernel[k];
                    r_acc += temp[idx + 0U] * w;
                    g_acc += temp[idx + 1U] * w;
                    b_acc += temp[idx + 2U] * w;
                    a_acc += temp[idx + 3U] * w;
                }
            }
            const std::size_t idx = (static_cast<std::size_t>(y) * m_width + x) * 4U;
            m_pixels[idx + 0U] = static_cast<std::uint8_t>(std::clamp(r_acc, 0.0f, 255.0f));
            m_pixels[idx + 1U] = static_cast<std::uint8_t>(std::clamp(g_acc, 0.0f, 255.0f));
            m_pixels[idx + 2U] = static_cast<std::uint8_t>(std::clamp(b_acc, 0.0f, 255.0f));
            m_pixels[idx + 3U] = static_cast<std::uint8_t>(std::clamp(a_acc, 0.0f, 255.0f));
        }
    }
}

void TextRasterSurface::apply_shadow(const TextShadowOptions& options) {
    if (!options.enabled) return;

    TextRasterSurface shadow_surface(m_width, m_height);
    shadow_surface.m_pixels = m_pixels;

    shadow_surface.apply_gaussian_blur(options.blur_radius);

    for (std::uint32_t y = 0; y < m_height; ++y) {
        for (std::uint32_t x = 0; x < m_width; ++x) {
            const int sx = static_cast<int>(x) + static_cast<int>(options.offset_x);
            const int sy = static_cast<int>(y) + static_cast<int>(options.offset_y);
            if (sx >= 0 && sx < static_cast<int>(m_width) && sy >= 0 && sy < static_cast<int>(m_height)) {
                const std::size_t src_idx = (static_cast<std::size_t>(sy) * m_width + static_cast<std::size_t>(sx)) * 4U;
                const float shadow_alpha = shadow_surface.m_pixels[src_idx + 3U] / 255.0f;
                const float src_alpha = options.color.a * shadow_alpha;
                if (src_alpha > 0.0f) {
                    blend_pixel(x, y, options.color, static_cast<std::uint8_t>(src_alpha * 255.0f));
                }
            }
        }
    }
}

void TextRasterSurface::apply_glow(const TextGlowOptions& options) {
    if (!options.enabled) return;

    TextRasterSurface glow_surface(m_width, m_height);
    for (std::uint32_t y = 0; y < m_height; ++y) {
        for (std::uint32_t x = 0; x < m_width; ++x) {
            const std::size_t idx = (static_cast<std::size_t>(y) * m_width + x) * 4U;
            glow_surface.m_pixels[idx + 0U] = m_pixels[idx + 3U];
            glow_surface.m_pixels[idx + 1U] = m_pixels[idx + 3U];
            glow_surface.m_pixels[idx + 2U] = m_pixels[idx + 3U];
            glow_surface.m_pixels[idx + 3U] = m_pixels[idx + 3U];
        }
    }

    glow_surface.apply_gaussian_blur(options.radius);

    for (std::uint32_t y = 0; y < m_height; ++y) {
        for (std::uint32_t x = 0; x < m_width; ++x) {
            const std::size_t idx = (static_cast<std::size_t>(y) * m_width + x) * 4U;
            const float glow_alpha = glow_surface.m_pixels[idx + 3U] / 255.0f;
            const float src_alpha = options.color.a * glow_alpha;
            if (src_alpha > 0.0f) {
                blend_pixel(x, y, options.color, static_cast<std::uint8_t>(src_alpha * 255.0f));
            }
        }
    }
}

bool TextRasterSurface::save_png(const std::filesystem::path& path) const {
    if (m_width == 0U || m_height == 0U) {
        return false;
    }

    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }

    return stbi_write_png(path.string().c_str(),
                          static_cast<int>(m_width),
                          static_cast<int>(m_height),
                          4,
                          m_pixels.data(),
                          static_cast<int>(m_width * 4U)) != 0;
}

} // namespace tachyon::text
