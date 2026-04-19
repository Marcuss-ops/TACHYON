#include "tachyon/renderer2d/evaluated_composition_renderer.h"

#include "tachyon/renderer2d/color_transfer.h"
#include "tachyon/renderer2d/rasterizer_ops.h"
#include "tachyon/renderer2d/raster/perspective_rasterizer.h"
#include "tachyon/scene/evaluator.h"
#include "tachyon/runtime/tile_scheduler.h"
#include "tachyon/text/font.h"
#include "tachyon/text/layout.h"
#include "tachyon/media/media_manager.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <future>
#include <mutex>
#include <filesystem>
#include <memory>
#include <optional>
#include <unordered_set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
namespace tachyon {
namespace {

using renderer2d::Color;
using renderer2d::RectI;
using renderer2d::SurfaceRGBA;

struct AccumulationBuffer {
    std::vector<float>& r;
    std::vector<float>& g;
    std::vector<float>& b;
    std::vector<float>& a;
    std::uint32_t width{0};
    std::uint32_t height{0};
    renderer2d::detail::TransferCurve transfer_curve{renderer2d::detail::TransferCurve::Srgb};

    AccumulationBuffer(std::uint32_t w, std::uint32_t h, renderer2d::detail::TransferCurve curve, renderer2d::RenderContext& context)
        : r(context.accumulation_buffer->r), g(context.accumulation_buffer->g), b(context.accumulation_buffer->b), a(context.accumulation_buffer->a), width(w), height(h), transfer_curve(curve) {
        const std::size_t size = static_cast<std::size_t>(w) * static_cast<std::size_t>(h);
        context.accumulation_buffer->ensure_capacity(size);
        std::fill(r.begin(), r.begin() + size, 0.0f);
        std::fill(g.begin(), g.begin() + size, 0.0f);
        std::fill(b.begin(), b.begin() + size, 0.0f);
        std::fill(a.begin(), a.begin() + size, 0.0f);
    }

    void add(const SurfaceRGBA& surface, float weight) {
        for (std::uint32_t y = 0; y < height; ++y) {
            for (std::uint32_t x = 0; x < width; ++x) {
                const auto p = surface.get_pixel(x, y);
                const std::size_t idx = static_cast<std::size_t>(y) * width + x;
                const auto linear = renderer2d::detail::to_premultiplied(p, transfer_curve);
                r[idx] += linear.r * weight;
                g[idx] += linear.g * weight;
                b[idx] += linear.b * weight;
                a[idx] += linear.a * weight;
            }
        }
    }

    void resolve(SurfaceRGBA& surface) const {
        for (std::uint32_t y = 0; y < height; ++y) {
            for (std::uint32_t x = 0; x < width; ++x) {
                const std::size_t idx = static_cast<std::size_t>(y) * width + x;
                surface.set_pixel(x, y, renderer2d::detail::from_premultiplied({
                    r[idx],
                    g[idx],
                    b[idx],
                    a[idx]
                }, transfer_curve));
            }
        }
    }
};

Color from_spec(const ColorSpec& spec) {
    return Color{
        static_cast<std::uint8_t>(std::clamp(static_cast<int>(spec.r), 0, 255)),
        static_cast<std::uint8_t>(std::clamp(static_cast<int>(spec.g), 0, 255)),
        static_cast<std::uint8_t>(std::clamp(static_cast<int>(spec.b), 0, 255)),
        static_cast<std::uint8_t>(std::clamp(static_cast<int>(spec.a), 0, 255))
    };
}

Color color_with_opacity(Color color, float opacity) {
    color.a = static_cast<std::uint8_t>(static_cast<float>(color.a) * std::clamp(opacity, 0.0f, 1.0f));
    return color;
}

RectI intersect_rects(const RectI& a, const RectI& b) {
    const int x0 = std::max(a.x, b.x);
    const int y0 = std::max(a.y, b.y);
    const int x1 = std::min(a.x + a.width, b.x + b.width);
    const int y1 = std::min(a.y + a.height, b.y + b.height);
    if (x1 <= x0 || y1 <= y0) return RectI{0, 0, 0, 0};
    return RectI{x0, y0, x1 - x0, y1 - y0};
}

RectI union_rects(const RectI& a, const RectI& b) {
    if (a.width <= 0 || a.height <= 0) {
        return b;
    }
    if (b.width <= 0 || b.height <= 0) {
        return a;
    }

    const int x0 = std::min(a.x, b.x);
    const int y0 = std::min(a.y, b.y);
    const int x1 = std::max(a.x + a.width, b.x + b.width);
    const int y1 = std::max(a.y + a.height, b.y + b.height);
    return RectI{x0, y0, x1 - x0, y1 - y0};
}

RectI layer_bounds(const scene::EvaluatedLayerState& layer, std::int64_t comp_w, std::int64_t comp_h) {
    int base_width = static_cast<int>(layer.width);
    int base_height = static_cast<int>(layer.height);

    if (base_width <= 0 || base_height <= 0) {
        if (layer.type == scene::LayerType::Text) {
            base_width = std::max(64, static_cast<int>(comp_w / 6));
            base_height = std::max(32, static_cast<int>(comp_h / 10));
        } else if (layer.type == scene::LayerType::Solid || layer.type == scene::LayerType::Shape || layer.type == scene::LayerType::Mask) {
            base_width = std::max(64, static_cast<int>(comp_w / 4));
            base_height = std::max(32, static_cast<int>(comp_h / 8));
        } else if (layer.precomp_id.has_value() && layer.nested_composition) {
            base_width = static_cast<int>(layer.nested_composition->width);
            base_height = static_cast<int>(layer.nested_composition->height);
        } else {
            base_width = 100;
            base_height = 100;
        }
    }

    const int w = std::max(1, static_cast<int>(std::round(static_cast<float>(base_width) * std::abs(layer.local_transform.scale.x))));
    const int h = std::max(1, static_cast<int>(std::round(static_cast<float>(base_height) * std::abs(layer.local_transform.scale.y))));
    return RectI{
        static_cast<int>(std::round(layer.local_transform.position.x)) - w / 2,
        static_cast<int>(std::round(layer.local_transform.position.y)) - h / 2,
        w, h
    };
}

bool point_in_polygon(const std::vector<scene::EvaluatedShapePathPoint>& points, float px, float py) {
    if (points.size() < 3) {
        return false;
    }

    bool inside = false;
    std::size_t previous = points.size() - 1;
    for (std::size_t current = 0; current < points.size(); previous = current++) {
        const auto& a = points[previous].position;
        const auto& b = points[current].position;
        const bool intersects = ((a.y > py) != (b.y > py)) &&
            (px < (b.x - a.x) * (py - a.y) / ((b.y - a.y) != 0.0f ? (b.y - a.y) : 1.0f) + a.x);
        if (intersects) {
            inside = !inside;
        }
    }
    return inside;
}

std::vector<std::uint8_t> build_adjustment_mask(
    const scene::EvaluatedLayerState& layer,
    const RectI& influence) {

    std::vector<std::uint8_t> mask(static_cast<std::size_t>(std::max(influence.width, 0)) * static_cast<std::size_t>(std::max(influence.height, 0)), 1U);
    if (!layer.shape_path.has_value() || layer.shape_path->points.size() < 3 || influence.width <= 0 || influence.height <= 0) {
        return mask;
    }

    const auto& path = *layer.shape_path;
    for (int y = 0; y < influence.height; ++y) {
        for (int x = 0; x < influence.width; ++x) {
            const float px = static_cast<float>(x) + 0.5f;
            const float py = static_cast<float>(y) + 0.5f;
            if (!point_in_polygon(path.points, px, py)) {
                mask[static_cast<std::size_t>(y) * static_cast<std::size_t>(influence.width) + static_cast<std::size_t>(x)] = 0U;
            }
        }
    }

    return mask;
}

float matte_strength_from_pixel(const renderer2d::Color& pixel, TrackMatteType matte_type) {
    const float alpha = static_cast<float>(pixel.a) / 255.0f;
    const float luma = (0.299f * static_cast<float>(pixel.r) +
                        0.587f * static_cast<float>(pixel.g) +
                        0.114f * static_cast<float>(pixel.b)) / 255.0f;
    const float strength = (matte_type == TrackMatteType::Alpha ||
                            matte_type == TrackMatteType::AlphaInverted)
        ? alpha
        : luma;
    if (matte_type == TrackMatteType::AlphaInverted ||
        matte_type == TrackMatteType::LumaInverted) {
        return std::clamp(1.0f - strength, 0.0f, 1.0f);
    }
    return std::clamp(strength, 0.0f, 1.0f);
}

void apply_track_matte(
    SurfaceRGBA& target,
    const SurfaceRGBA& matte_surface,
    TrackMatteType matte_type,
    const RectI& influence) {

    if (matte_type == TrackMatteType::None || influence.width <= 0 || influence.height <= 0) {
        return;
    }

    const int start_x = std::max(0, influence.x);
    const int start_y = std::max(0, influence.y);
    const int end_x = std::min(static_cast<int>(target.width()), influence.x + influence.width);
    const int end_y = std::min(static_cast<int>(target.height()), influence.y + influence.height);

    for (int y = start_y; y < end_y; ++y) {
        for (int x = start_x; x < end_x; ++x) {
            const Color matte_pixel = matte_surface.get_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y));
            const float strength = matte_strength_from_pixel(matte_pixel, matte_type);
            Color value = target.get_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y));
            if (strength <= 0.0f) {
                value = Color::transparent();
            } else if (strength < 1.0f) {
                value.r = static_cast<std::uint8_t>(std::clamp(std::lround(static_cast<float>(value.r) * strength), 0L, 255L));
                value.g = static_cast<std::uint8_t>(std::clamp(std::lround(static_cast<float>(value.g) * strength), 0L, 255L));
                value.b = static_cast<std::uint8_t>(std::clamp(std::lround(static_cast<float>(value.b) * strength), 0L, 255L));
                value.a = static_cast<std::uint8_t>(std::clamp(std::lround(static_cast<float>(value.a) * strength), 0L, 255L));
            }
            target.set_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), value);
        }
    }
}

renderer2d::BlendMode parse_blend_mode(const std::string& mode) {
    if (mode == "additive") return renderer2d::BlendMode::Additive;
    if (mode == "multiply") return renderer2d::BlendMode::Multiply;
    if (mode == "screen") return renderer2d::BlendMode::Screen;
    return renderer2d::BlendMode::Normal;
}

std::uint64_t fnv1a64(std::string_view value) {
    constexpr std::uint64_t kOffset = 1469598103934665603ULL;
    constexpr std::uint64_t kPrime = 1099511628211ULL;
    std::uint64_t hash = kOffset;
    for (unsigned char ch : value) {
        hash ^= static_cast<std::uint64_t>(ch);
        hash *= kPrime;
    }
    return hash;
}

std::string build_composition_hash(const scene::EvaluatedCompositionState& state) {
    std::ostringstream stream;
    stream << state.composition_id << ';'
           << state.width << 'x' << state.height << ';'
           << state.frame_rate.numerator << '/' << state.frame_rate.denominator << ';'
           << state.layers.size() << ';';
    for (const auto& layer : state.layers) {
        stream << layer.id << ':' << static_cast<int>(layer.type) << ':' << layer.visible << ':'
               << layer.active << ':' << layer.opacity << ':'
               << layer.local_transform.position.x << ':' << layer.local_transform.position.y << ':'
               << layer.local_transform.scale.x << ':' << layer.local_transform.scale.y << ';';
    }
    return stream.str();
}

std::string build_layer_cache_signature(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& nested,
    double child_time_seconds) {

    std::ostringstream stream;
    stream << layer.id << '|'
           << layer.name << '|'
           << static_cast<int>(layer.type) << '|'
           << layer.visible << '|'
           << layer.active << '|'
           << layer.is_3d << '|'
           << layer.is_adjustment_layer << '|'
           << layer.blend_mode << '|'
           << layer.opacity << '|'
           << layer.local_transform.position.x << ',' << layer.local_transform.position.y << '|'
           << layer.local_transform.scale.x << ',' << layer.local_transform.scale.y << '|'
           << layer.world_position3.x << ',' << layer.world_position3.y << ',' << layer.world_position3.z << '|'
           << layer.width << 'x' << layer.height << '|'
           << layer.precomp_id.value_or("") << '|'
           << std::fixed << std::setprecision(6) << child_time_seconds << '|'
           << build_composition_hash(nested);
    return stream.str();
}

std::string build_precomp_cache_key(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& nested,
    double child_time_seconds) {

    const std::string signature = build_layer_cache_signature(layer, nested, child_time_seconds);
    std::ostringstream stream;
    stream << std::hex << fnv1a64(signature);
    return stream.str();
}

std::string to_lower_ascii(std::string_view value) {
    std::string lower;
    lower.reserve(value.size());
    for (unsigned char ch : value) {
        lower.push_back(static_cast<char>(std::tolower(ch)));
    }
    return lower;
}

enum class MotionBlurCurve {
    Box,
    Triangle,
    Gaussian
};

MotionBlurCurve parse_motion_blur_curve(std::string_view value) {
    const std::string lower = to_lower_ascii(value);
    if (lower == "triangle") {
        return MotionBlurCurve::Triangle;
    }
    if (lower == "gaussian") {
        return MotionBlurCurve::Gaussian;
    }
    return MotionBlurCurve::Box;
}

std::vector<float> make_motion_blur_weights(int sample_count, std::string_view curve_name) {
    std::vector<float> weights(static_cast<std::size_t>(std::max(sample_count, 0)), 0.0f);
    if (sample_count <= 0) {
        return weights;
    }

    const MotionBlurCurve curve = parse_motion_blur_curve(curve_name);
    float total = 0.0f;
    for (int sample = 0; sample < sample_count; ++sample) {
        const float t = sample_count > 1
            ? (2.0f * static_cast<float>(sample) / static_cast<float>(sample_count - 1) - 1.0f)
            : 0.0f;

        float weight = 1.0f;
        switch (curve) {
        case MotionBlurCurve::Box:
            weight = 1.0f;
            break;
        case MotionBlurCurve::Triangle:
            weight = std::max(0.0f, 1.0f - std::abs(t));
            break;
        case MotionBlurCurve::Gaussian: {
            constexpr float sigma = 0.35f;
            const float variance = sigma * sigma;
            weight = std::exp(-(t * t) / (2.0f * variance));
            break;
        }
        }

        weights[static_cast<std::size_t>(sample)] = weight;
        total += weight;
    }

    if (total <= 0.0f) {
        const float uniform = 1.0f / static_cast<float>(sample_count);
        std::fill(weights.begin(), weights.end(), uniform);
        return weights;
    }

    for (float& weight : weights) {
        weight /= total;
    }
    return weights;
}

void composite_surface_at(SurfaceRGBA& dest, const SurfaceRGBA& src, int x, int y, const RectI& clip, renderer2d::BlendMode mode) {
    const RectI src_rect{x, y, static_cast<int>(src.width()), static_cast<int>(src.height())};
    const RectI clipped = intersect_rects(src_rect, clip);
    if (clipped.width <= 0 || clipped.height <= 0) return;

    for (int cy = clipped.y; cy < clipped.y + clipped.height; ++cy) {
        for (int cx = clipped.x; cx < clipped.x + clipped.width; ++cx) {
            const Color src_pixel = src.get_pixel(static_cast<std::uint32_t>(cx - x), static_cast<std::uint32_t>(cy - y));
            if (src_pixel.a == 0) {
                continue;
            }

            if (mode == renderer2d::BlendMode::Normal) {
                dest.blend_pixel(static_cast<std::uint32_t>(cx), static_cast<std::uint32_t>(cy), src_pixel);
                continue;
            }

            const auto dst_pixel = dest.try_get_pixel(static_cast<std::uint32_t>(cx), static_cast<std::uint32_t>(cy));
            if (!dst_pixel.has_value()) {
                continue;
            }

            dest.set_pixel(
                static_cast<std::uint32_t>(cx),
                static_cast<std::uint32_t>(cy),
                renderer2d::blend_mode_color(src_pixel, *dst_pixel, mode));
        }
    }
}

Color sample_bilinear(const SurfaceRGBA& surface, float u, float v) {
    const float x = u * (static_cast<float>(surface.width()) - 1.0f);
    const float y = v * (static_cast<float>(surface.height()) - 1.0f);
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const int x1 = std::min(x0 + 1, static_cast<int>(surface.width()) - 1);
    const int y1 = std::min(y0 + 1, static_cast<int>(surface.height()) - 1);

    const float dx = x - static_cast<float>(x0);
    const float dy = y - static_cast<float>(y0);

    const Color c00 = surface.get_pixel(static_cast<std::uint32_t>(x0), static_cast<std::uint32_t>(y0));
    const Color c10 = surface.get_pixel(static_cast<std::uint32_t>(x1), static_cast<std::uint32_t>(y0));
    const Color c01 = surface.get_pixel(static_cast<std::uint32_t>(x0), static_cast<std::uint32_t>(y1));
    const Color c11 = surface.get_pixel(static_cast<std::uint32_t>(x1), static_cast<std::uint32_t>(y1));

    auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };

    return Color{
        static_cast<std::uint8_t>(lerp(lerp(static_cast<float>(c00.r), static_cast<float>(c10.r), dx), lerp(static_cast<float>(c01.r), static_cast<float>(c11.r), dx), dy)),
        static_cast<std::uint8_t>(lerp(lerp(static_cast<float>(c00.g), static_cast<float>(c10.g), dx), lerp(static_cast<float>(c01.g), static_cast<float>(c11.g), dx), dy)),
        static_cast<std::uint8_t>(lerp(lerp(static_cast<float>(c00.b), static_cast<float>(c10.b), dx), lerp(static_cast<float>(c01.b), static_cast<float>(c11.b), dx), dy)),
        static_cast<std::uint8_t>(lerp(lerp(static_cast<float>(c00.a), static_cast<float>(c10.a), dx), lerp(static_cast<float>(c01.a), static_cast<float>(c11.a), dx), dy))
    };
}

void render_layer_to_surface(
    SurfaceRGBA& layer_surface,
    const scene::EvaluatedLayerState& layer,
    const std::vector<scene::EvaluatedLightState>& lights,
    std::int64_t comp_w,
    std::int64_t comp_h,
    const RectI& render_region,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    renderer2d::RenderContext& context,
    const scene::EvaluatedCameraState& camera) {

    (void)lights;
    (void)plan;
    (void)task;

    if (layer.is_3d && camera.available) {
        // Perspective Quad Warping logic placeholder
        return;
    }

    // Fallback 2D Rendering
    const RectI bounds = layer_bounds(layer, comp_w, comp_h);
    const RectI clipped = intersect_rects(bounds, render_region);
    if (clipped.width <= 0 || clipped.height <= 0) return;

    const int lx = bounds.x - render_region.x;
    const int ly = bounds.y - render_region.y;

    if (layer.type == scene::LayerType::Solid) {
        layer_surface.fill_rect({lx, ly, bounds.width, bounds.height}, color_with_opacity(from_spec(layer.fill_color), static_cast<float>(layer.opacity)));
    } else if (layer.type == scene::LayerType::Image) {
        layer_surface.fill_rect(
            {lx, ly, bounds.width, bounds.height},
            color_with_opacity(from_spec(layer.fill_color), static_cast<float>(layer.opacity)));
    } else if (layer.type == scene::LayerType::Text) {
        using namespace text;
        const Font* font = context.fonts.default_font();
        if (font) {
            TextStyle style;
            style.pixel_size = static_cast<std::uint32_t>(std::abs(layer.local_transform.scale.y) * 48.0f);
            style.fill_color = from_spec(layer.fill_color);
            
            TextBox box;
            box.width = static_cast<std::uint32_t>(bounds.width);
            box.height = static_cast<std::uint32_t>(bounds.height);
            
            TextRasterSurface raster = rasterize_text_rgba(*font, layer.text_content, style, box, TextAlignment::Center);
            
            for (std::uint32_t y = 0; y < raster.height(); ++y) {
                for (std::uint32_t x = 0; x < raster.width(); ++x) {
                    const int tx = lx + static_cast<int>(x);
                    const int ty = ly + static_cast<int>(y);
                    if (tx < 0 || ty < 0 || tx >= static_cast<int>(layer_surface.width()) || ty >= static_cast<int>(layer_surface.height())) {
                        continue;
                    }
                    Color p = raster.get_pixel(x, y);
                    layer_surface.set_pixel(static_cast<std::uint32_t>(tx), static_cast<std::uint32_t>(ty), color_with_opacity(p, static_cast<float>(layer.opacity)));
                }
            }
        }
    }
}

RasterizedFrame2D render_composition_recursive(
    const std::string& composition_id,
    const std::vector<scene::EvaluatedLayerState>& layers,
    const std::vector<scene::EvaluatedLightState>& lights,
    std::int64_t width,
    std::int64_t height,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    renderer2d::RenderContext& context,
    double composition_time_seconds,
    int precomp_recursion_depth);

void render_layer_recursive(
    const std::vector<scene::EvaluatedLayerState>& all_layers,
    const scene::EvaluatedLayerState& layer,
    const std::vector<scene::EvaluatedLightState>& lights,
    std::int64_t width,
    std::int64_t height,
    const RectI& tile,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    renderer2d::RenderContext& context,
    const scene::EvaluatedCameraState& camera,
    SurfaceRGBA& surface,
    int precomp_recursion_depth,
    bool allow_track_matte) {
    
    if (!layer.visible) return;

    const RectI bounds = layer_bounds(layer, width, height);
    const RectI influence = intersect_rects(bounds, tile);
    if (influence.width <= 0 || influence.height <= 0) {
        return;
    }

    SurfaceRGBA ls(static_cast<std::uint32_t>(tile.width), static_cast<std::uint32_t>(tile.height));
    ls.clear(Color::transparent());
    std::optional<SurfaceRGBA> adjustment_original;

    if (layer.is_adjustment_layer) {
        // Step 2: Adjustment Layers - Pass underlying composite to the layer
        SurfaceRGBA original(static_cast<std::uint32_t>(influence.width), static_cast<std::uint32_t>(influence.height));
        for (int y = 0; y < influence.height; ++y) {
            for (int x = 0; x < influence.width; ++x) {
                original.set_pixel(
                    static_cast<std::uint32_t>(x),
                    static_cast<std::uint32_t>(y),
                    surface.get_pixel(static_cast<std::uint32_t>(influence.x - tile.x + x), static_cast<std::uint32_t>(influence.y - tile.y + y)));
            }
        }
        adjustment_original = original;
        
        // Use a tile-sized surface for effects logic consistency
        ls = SurfaceRGBA(static_cast<std::uint32_t>(tile.width), static_cast<std::uint32_t>(tile.height));
        ls.clear(Color::transparent());
        for (int y = 0; y < influence.height; ++y) {
            for (int x = 0; x < influence.width; ++x) {
                ls.set_pixel(
                    static_cast<std::uint32_t>(influence.x - tile.x + x),
                    static_cast<std::uint32_t>(influence.y - tile.y + y),
                    original.get_pixel(x, y));
            }
        }
    } else if (layer.type == scene::LayerType::Precomp && layer.nested_composition) {
        if (precomp_recursion_depth < 8) {
            const std::string cache_key = build_precomp_cache_key(layer, *layer.nested_composition, layer.child_time_seconds);
            RasterizedFrame2D nested;
            bool cache_hit = context.policy.precomp_cache_enabled && context.precomp_cache && context.precomp_cache->lookup(cache_key, nested);
            if (!cache_hit) {
                nested = render_composition_recursive(
                    layer.nested_composition->composition_id,
                    layer.nested_composition->layers,
                    layer.nested_composition->lights,
                    layer.nested_composition->width,
                    layer.nested_composition->height,
                    plan, task, context, layer.child_time_seconds, precomp_recursion_depth + 1);
                if (context.policy.precomp_cache_enabled && context.precomp_cache) {
                    context.precomp_cache->store(cache_key, nested);
                }
            }

            if (nested.surface.has_value()) {
                const int nested_x = bounds.x - tile.x;
                const int nested_y = bounds.y - tile.y;
                composite_surface_at(
                    ls,
                    *nested.surface,
                    nested_x,
                    nested_y,
                    RectI{0, 0, tile.width, tile.height},
                    renderer2d::BlendMode::Normal);
            }
        }
    } else {
        render_layer_to_surface(ls, layer, lights, width, height, tile, plan, task, context, camera);
    }

    if (context.policy.effects_enabled && !layer.effects.empty()) {
        for (const auto& effect_spec : layer.effects) {
            if (effect_spec.enabled && context.effects.has_effect(effect_spec.type)) {
                renderer2d::EffectParams params;
                for (const auto& kv : effect_spec.scalars) {
                    params.scalars[kv.first] = static_cast<float>(kv.second);
                }
                for (const auto& kv : effect_spec.colors) {
                    params.colors[kv.first] = renderer2d::Color{kv.second.r, kv.second.g, kv.second.b, kv.second.a};
                }
                ls = context.effects.apply(effect_spec.type, ls, params);
            }
        }
    }

    if (allow_track_matte &&
        layer.track_matte_type != TrackMatteType::None &&
        layer.track_matte_layer_index.has_value() &&
        *layer.track_matte_layer_index < all_layers.size()) {

        const scene::EvaluatedLayerState& matte_layer = all_layers[*layer.track_matte_layer_index];
        if (matte_layer.visible) {
            SurfaceRGBA matte_surface(static_cast<std::uint32_t>(tile.width), static_cast<std::uint32_t>(tile.height));
            matte_surface.clear(Color::transparent());
            render_layer_recursive(
                all_layers,
                matte_layer,
                lights,
                width,
                height,
                tile,
                plan,
                task,
                context,
                camera,
                matte_surface,
                precomp_recursion_depth + 1,
                false);

            apply_track_matte(ls, matte_surface, layer.track_matte_type, RectI{0, 0, tile.width, tile.height});
        }
    }

    if (layer.is_adjustment_layer) {
        const std::vector<std::uint8_t> mask = build_adjustment_mask(layer, influence);
        const float alpha = std::clamp(static_cast<float>(layer.opacity), 0.0f, 1.0f);
        for (int y = 0; y < influence.height; ++y) {
            for (int x = 0; x < influence.width; ++x) {
                const std::size_t mask_index = static_cast<std::size_t>(y) * static_cast<std::size_t>(influence.width) + static_cast<std::size_t>(x);
                if (!mask.empty() && mask[mask_index] == 0U) {
                    continue;
                }

                const int tx = influence.x - tile.x + x;
                const int ty = influence.y - tile.y + y;
                
                const renderer2d::Color orig_pixel = adjustment_original->get_pixel(x, y);
                renderer2d::Color eff_pixel = ls.get_pixel(tx, ty);
                eff_pixel.r = static_cast<std::uint8_t>(std::clamp(orig_pixel.r * (1.0f - alpha) + eff_pixel.r * alpha, 0.0f, 255.0f));
                eff_pixel.g = static_cast<std::uint8_t>(std::clamp(orig_pixel.g * (1.0f - alpha) + eff_pixel.g * alpha, 0.0f, 255.0f));
                eff_pixel.b = static_cast<std::uint8_t>(std::clamp(orig_pixel.b * (1.0f - alpha) + eff_pixel.b * alpha, 0.0f, 255.0f));
                eff_pixel.a = orig_pixel.a;
                surface.set_pixel(static_cast<std::uint32_t>(tx), static_cast<std::uint32_t>(ty), eff_pixel);
            }
        }
    } else {
        composite_surface_at(surface, ls, 0, 0, RectI{0, 0, tile.width, tile.height}, parse_blend_mode(layer.blend_mode));
    }
}

RasterizedFrame2D render_composition_recursive(
    const std::string& composition_id,
    const std::vector<scene::EvaluatedLayerState>& layers,
    const std::vector<scene::EvaluatedLightState>& lights,
    std::int64_t width,
    std::int64_t height,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    renderer2d::RenderContext& context,
    double composition_time_seconds,
    int precomp_recursion_depth) {

    (void)lights;

    // 1. Resolution Scale (Quality Policy)
    const float scale = context.policy.resolution_scale;
    const std::int64_t scaled_width = static_cast<std::int64_t>(std::max(1.0f, static_cast<float>(width) * scale));
    const std::int64_t scaled_height = static_cast<std::int64_t>(std::max(1.0f, static_cast<float>(height) * scale));

    // 2. Precomp Cache Lookup
    std::string cache_key;
    if (context.precomp_cache) {
        // Build a robust cache key: ID + Size + Time + QualityTier
        cache_key = composition_id + "|" + 
                    std::to_string(scaled_width) + "x" + std::to_string(scaled_height) + "|" +
                    std::to_string(composition_time_seconds) + "|" +
                    plan.quality_tier;
        
        RasterizedFrame2D cached;
        if (context.precomp_cache->lookup(cache_key, cached)) {
            return cached;
        }
    }

    RasterizedFrame2D frame;
    frame.width = scaled_width;
    frame.height = scaled_height;
    frame.surface.emplace(static_cast<std::uint32_t>(scaled_width), static_cast<std::uint32_t>(scaled_height));
    frame.surface->clear(Color::transparent());

    const renderer2d::detail::TransferCurve working_curve = renderer2d::detail::parse_transfer_curve(plan.working_space);
    const int requested_samples = plan.motion_blur_samples > 0 ? static_cast<int>(plan.motion_blur_samples) : 8;
    const int capped_samples = std::min(requested_samples, context.policy.motion_blur_sample_cap);
    const int num_samples = plan.motion_blur_enabled ? std::max(1, capped_samples) : 1;
    const double shutter_angle = std::max(0.0, std::min(360.0, plan.motion_blur_shutter_angle));
    const double frame_duration = static_cast<double>(plan.composition.frame_rate.denominator) / static_cast<double>(plan.composition.frame_rate.numerator);
    const double shutter_seconds = frame_duration * (shutter_angle / 360.0);
    const std::vector<float> sample_weights = make_motion_blur_weights(num_samples, plan.motion_blur_curve);

    struct SampleRenderState {
        std::vector<scene::EvaluatedLayerState> layers;
        std::vector<scene::EvaluatedLightState> lights;
        scene::EvaluatedCameraState camera;
    };

    std::vector<SampleRenderState> sample_states;
    sample_states.reserve(static_cast<std::size_t>(num_samples));

    for (int sample = 0; sample < num_samples; ++sample) {
        const double time_offset = (num_samples > 1)
            ? (static_cast<double>(sample) / static_cast<double>(num_samples - 1) - 0.5) * shutter_seconds
            : 0.0;
        const double sample_time = composition_time_seconds + time_offset;

        SampleRenderState sample_state;
        sample_state.layers = layers;

        if (plan.scene_spec != nullptr) {
            const auto evaluated_state = scene::evaluate_scene_composition_state(*plan.scene_spec, composition_id, sample_time);
            if (evaluated_state.has_value()) {
                sample_state.layers = evaluated_state->layers;
                sample_state.lights = evaluated_state->lights;
                sample_state.camera = evaluated_state->camera;
            }
        }

        // Apply resolution scale to layer positions/sizes in the evaluated state
        if (scale != 1.0f) {
            for (auto& layer : sample_state.layers) {
                layer.local_transform.position.x *= scale;
                layer.local_transform.position.y *= scale;
                layer.local_transform.scale.x *= scale;
                layer.local_transform.scale.y *= scale;
            }
            for (auto& light : sample_state.lights) {
                light.position.x *= scale;
                light.position.y *= scale;
            }
        }

        std::stable_sort(sample_state.layers.begin(), sample_state.layers.end(), [&](const auto& a, const auto& b) {
            if (a.is_3d && b.is_3d && sample_state.camera.available) {
                const float dist_a = (a.world_position3 - sample_state.camera.position).length_squared();
                const float dist_b = (b.world_position3 - sample_state.camera.position).length_squared();
                return dist_a > dist_b;
            }
            return a.layer_index < b.layer_index;
        });

        sample_states.push_back(std::move(sample_state));
    }

    RectI roi{0, 0, 0, 0};
    for (const auto& sample_state : sample_states) {
        for (const auto& layer : sample_state.layers) {
            roi = union_rects(roi, layer_bounds(layer, scaled_width, scaled_height));
        }
    }
    const int tile_size = context.policy.tile_size > 0 ? context.policy.tile_size : 256;
    const TileGrid tile_grid = build_tile_grid(roi, scaled_width, scaled_height, tile_size);
    const std::vector<RectI> fallback_tiles{RectI{0, 0, static_cast<int>(scaled_width), static_cast<int>(scaled_height)}};
    const std::vector<RectI>& tiles = tile_grid.tiles.empty() ? fallback_tiles : tile_grid.tiles;

    for (const auto& tile : tiles) {
        AccumulationBuffer accumulation(static_cast<std::uint32_t>(tile.width), static_cast<std::uint32_t>(tile.height), working_curve, context);
        SurfaceRGBA tile_surface(static_cast<std::uint32_t>(tile.width), static_cast<std::uint32_t>(tile.height));
        SurfaceRGBA tile_resolved(static_cast<std::uint32_t>(tile.width), static_cast<std::uint32_t>(tile.height));

        std::vector<bool> matte_providers(sample_states.empty() ? 0U : sample_states.front().layers.size(), false);
        for (std::size_t sample = 0; sample < sample_states.size(); ++sample) {
            tile_surface.clear(Color::transparent());
            const auto& sample_state = sample_states[sample];

            if (matte_providers.size() != sample_state.layers.size()) {
                matte_providers.assign(sample_state.layers.size(), false);
            }
            std::fill(matte_providers.begin(), matte_providers.end(), false);
            for (const auto& matte_candidate : sample_state.layers) {
                if (matte_candidate.track_matte_type != TrackMatteType::None &&
                    matte_candidate.track_matte_layer_index.has_value() &&
                    *matte_candidate.track_matte_layer_index < matte_providers.size()) {
                    matte_providers[*matte_candidate.track_matte_layer_index] = true;
                }
            }

            for (const auto& layer : sample_state.layers) {
                if (!layer.visible || layer.type == scene::LayerType::Camera) {
                    continue;
                }
                if (layer.layer_index < matte_providers.size() && matte_providers[layer.layer_index]) {
                    continue;
                }

                const RectI bounds = layer_bounds(layer, scaled_width, scaled_height);
                if (intersect_rects(bounds, tile).width <= 0 || intersect_rects(bounds, tile).height <= 0) {
                    continue;
                }

                render_layer_recursive(
                    sample_state.layers,
                    layer,
                    sample_state.lights,
                    scaled_width,
                    scaled_height,
                    tile,
                    plan,
                    task,
                    context,
                    sample_state.camera,
                    tile_surface,
                    precomp_recursion_depth,
                    true);
            }

            accumulation.add(tile_surface, sample_weights[sample]);
        }

        accumulation.resolve(tile_resolved);
        composite_surface_at(*frame.surface, tile_resolved, tile.x, tile.y, RectI{tile.x, tile.y, tile.width, tile.height}, renderer2d::BlendMode::Normal);
    }

    // 3. Store in Precomp Cache
    if (context.precomp_cache && !cache_key.empty()) {
        context.precomp_cache->store(cache_key, frame);
    }

    return frame;
}

} // namespace

RasterizedFrame2D render_evaluated_composition_2d(
    const scene::EvaluatedCompositionState& state,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    renderer2d::RenderContext& context) {
    return render_composition_recursive(
        state.composition_id,
        state.layers,
        state.lights,
        state.width,
        state.height,
        plan,
        task,
        context,
        state.composition_time_seconds,
        0);
}

} // namespace tachyon
