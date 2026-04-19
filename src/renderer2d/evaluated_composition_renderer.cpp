#include "tachyon/renderer2d/evaluated_composition_renderer.h"

#include "tachyon/renderer2d/color_transfer.h"
#include "tachyon/renderer2d/rasterizer_ops.h"
#include "tachyon/renderer2d/raster/perspective_rasterizer.h"
#include "tachyon/scene/evaluator.h"
#include "tachyon/text/font.h"
#include "tachyon/text/layout.h"

#include <algorithm>
#include <cmath>
#include <future>
#include <mutex>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <map>

namespace tachyon {
namespace {

using renderer2d::Color;
using renderer2d::RectI;
using renderer2d::SurfaceRGBA;

struct RenderContext {
    std::map<std::string, RasterizedFrame2D> precomp_cache;
};

int g_precomp_recursion_depth = 0;

struct AccumulationBuffer {
    std::vector<float> r;
    std::vector<float> g;
    std::vector<float> b;
    std::vector<float> a;
    std::uint32_t width{0};
    std::uint32_t height{0};
    renderer2d::detail::TransferCurve transfer_curve{renderer2d::detail::TransferCurve::Srgb};

    AccumulationBuffer(std::uint32_t w, std::uint32_t h, renderer2d::detail::TransferCurve curve)
        : width(w), height(h), transfer_curve(curve) {
        const std::size_t size = static_cast<std::size_t>(w) * static_cast<std::size_t>(h);
        r.assign(size, 0.0f);
        g.assign(size, 0.0f);
        b.assign(size, 0.0f);
        a.assign(size, 0.0f);
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

RectI layer_bounds(const scene::EvaluatedLayerState& layer, std::int64_t comp_w, std::int64_t comp_h) {
    (void)comp_w;
    (void)comp_h;
    // Basic 2D bounds for ROI filtering
    const int w = static_cast<int>(std::round(static_cast<float>(layer.width) * std::abs(layer.local_transform.scale.x)));
    const int h = static_cast<int>(std::round(static_cast<float>(layer.height) * std::abs(layer.local_transform.scale.y)));
    return RectI{
        static_cast<int>(std::round(layer.local_transform.position.x)) - w / 2,
        static_cast<int>(std::round(layer.local_transform.position.y)) - h / 2,
        w, h
    };
}

renderer2d::BlendMode parse_blend_mode(const std::string& mode) {
    if (mode == "additive") return renderer2d::BlendMode::Additive;
    if (mode == "multiply") return renderer2d::BlendMode::Multiply;
    if (mode == "screen") return renderer2d::BlendMode::Screen;
    return renderer2d::BlendMode::Normal;
}

void composite_surface_at(SurfaceRGBA& dest, const SurfaceRGBA& src, int x, int y, const RectI& clip, renderer2d::BlendMode mode) {
    (void)mode;
    const RectI src_rect{x, y, static_cast<int>(src.width()), static_cast<int>(src.height())};
    const RectI clipped = intersect_rects(src_rect, clip);
    if (clipped.width <= 0 || clipped.height <= 0) return;

    for (int cy = clipped.y; cy < clipped.y + clipped.height; ++cy) {
        for (int cx = clipped.x; cx < clipped.x + clipped.width; ++cx) {
            Color p = src.get_pixel(cx - x, cy - y);
            if (p.a > 0) {
                dest.blend_pixel(cx, cy, p);
            }
        }
    }
}

void render_layer_to_surface(
    SurfaceRGBA& layer_surface,
    const scene::EvaluatedLayerState& layer,
    std::int64_t comp_w,
    std::int64_t comp_h,
    const RectI& render_region,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext& context,
    const scene::EvaluatedCameraState& camera) {

    (void)plan;

    if (layer.is_3d && camera.available) {
        // Perspective Quad Warping logic
        const float w = static_cast<float>(layer.width);
        const float h = static_cast<float>(layer.height);
        
        // Generate 4 corners in local space
        math::Vector3 p[4] = {
            {-w/2, -h/2, 0}, {w/2, -h/2, 0}, {w/2, h/2, 0}, {-w/2, h/2, 0}
        };

        // Transform to world space
        for (int i = 0; i < 4; ++i) {
            p[i].x = p[i].x * layer.local_transform.scale.x + layer.world_position3.x;
            p[i].y = p[i].y * layer.local_transform.scale.y + layer.world_position3.y;
            p[i].z = p[i].z + layer.world_position3.z;
        }

        renderer2d::raster::PerspectiveWarpQuad quad;
        quad.opacity = static_cast<float>(layer.opacity);
        
        auto project = [&](const math::Vector3& v, renderer2d::raster::Vertex3D& out, const math::Vector2& uv) {
            const math::Vector3 eye_vec = v - camera.position;
            const float z = eye_vec.length();
            const math::Vector2 screen = camera.camera.project_point(v, static_cast<float>(comp_w), static_cast<float>(comp_h));
            out.position = {screen.x - render_region.x, screen.y - render_region.y, 0.0f};
            out.uv = uv;
            out.one_over_w = z > 0.01f ? 1.0f / z : 1.0f;
        };

        project(p[0], quad.v0, {0,0});
        project(p[1], quad.v1, {1,0});
        project(p[2], quad.v2, {1,1});
        project(p[3], quad.v3, {0,1});

        // Resolve texture
        if (layer.type == scene::LayerType::Precomp && layer.nested_composition) {
            std::string ck = "precomp_" + layer.id + "_" + std::to_string(task.frame_number);
            auto it = context.precomp_cache.find(ck);
            if (it != context.precomp_cache.end() && it->second.surface.has_value()) {
                quad.texture = &(*it->second.surface);
                renderer2d::raster::PerspectiveRasterizer::draw_quad(layer_surface, quad);
            }
        }
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
    }
}

// Forward declaration
RasterizedFrame2D render_composition_recursive(
    const std::vector<scene::EvaluatedLayerState>& layers,
    std::int64_t width,
    std::int64_t height,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext& context);

void render_layer_recursive(
    const scene::EvaluatedLayerState& layer,
    std::int64_t width, std::int64_t height,
    const RectI& tile,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext& context,
    const scene::EvaluatedCameraState& camera,
    SurfaceRGBA& surface) {
    
    if (!layer.visible) return;

    if (layer.is_adjustment_layer) {
        return;
    }

    if (layer.type == scene::LayerType::Precomp && layer.nested_composition) {
        if (g_precomp_recursion_depth < 8) {
            g_precomp_recursion_depth++;
            RasterizedFrame2D nested = render_composition_recursive(
                layer.nested_composition->layers,
                layer.nested_composition->width,
                layer.nested_composition->height,
                plan, task, context);
            g_precomp_recursion_depth--;
            
            std::string ck = "precomp_" + layer.id + "_" + std::to_string(task.frame_number);
            context.precomp_cache[ck] = std::move(nested);
        }
    }

    SurfaceRGBA ls(static_cast<std::uint32_t>(tile.width), static_cast<std::uint32_t>(tile.height));
    ls.clear(Color::transparent());
    render_layer_to_surface(ls, layer, width, height, tile, plan, task, context, camera);
    
    composite_surface_at(surface, ls, 0, 0, RectI{0, 0, tile.width, tile.height}, parse_blend_mode(layer.blend_mode));
}

RasterizedFrame2D render_composition_recursive(
    const std::vector<scene::EvaluatedLayerState>& layers,
    std::int64_t width,
    std::int64_t height,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext& context) {

    RasterizedFrame2D frame;
    frame.width = width;
    frame.height = height;
    frame.surface.emplace(static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
    frame.surface->clear(Color::transparent());

    const renderer2d::detail::TransferCurve working_curve = renderer2d::detail::parse_transfer_curve(plan.working_space);
    const int num_samples = plan.motion_blur_enabled ? std::max(1, static_cast<int>(plan.motion_blur_samples)) : 1;
    const double shutter_angle = std::max(0.0, std::min(360.0, plan.motion_blur_shutter_angle));
    const double frame_duration = static_cast<double>(plan.composition.frame_rate.denominator) / static_cast<double>(plan.composition.frame_rate.numerator);
    const double shutter_seconds = frame_duration * (shutter_angle / 360.0);

    AccumulationBuffer accumulation(static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height), working_curve);
    const float sample_weight = 1.0f / static_cast<float>(num_samples);

    for (int sample = 0; sample < num_samples; ++sample) {
        const double time_offset = (num_samples > 1)
            ? (static_cast<double>(sample) / static_cast<double>(num_samples - 1) - 0.5) * shutter_seconds
            : 0.0;
        const double sample_time = static_cast<double>(task.frame_number) * frame_duration + time_offset;

        std::vector<scene::EvaluatedLayerState> sample_layers = layers;
        scene::EvaluatedCameraState sample_camera;

        if (plan.scene_spec != nullptr) {
            const auto sample_state = scene::evaluate_scene_composition_state(*plan.scene_spec, plan.composition.id, sample_time);
            if (sample_state.has_value()) {
                sample_layers = sample_state->layers;
                sample_camera = sample_state->camera;
            }
        }

        SurfaceRGBA sample_surface(static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
        sample_surface.clear(Color::transparent());

        std::stable_sort(sample_layers.begin(), sample_layers.end(), [&](const auto& a, const auto& b) {
            if (a.is_3d && b.is_3d && sample_camera.available) {
                const float dist_a = (a.world_position3 - sample_camera.position).length_squared();
                const float dist_b = (b.world_position3 - sample_camera.position).length_squared();
                return dist_a > dist_b;
            }
            return a.layer_index < b.layer_index;
        });

        for (const auto& layer : sample_layers) {
            if (!layer.visible || layer.type == scene::LayerType::Camera) {
                continue;
            }
            render_layer_recursive(layer, width, height, RectI{0, 0, static_cast<int>(width), static_cast<int>(height)}, plan, task, context, sample_camera, sample_surface);
        }

        accumulation.add(sample_surface, sample_weight);
    }

    accumulation.resolve(*frame.surface);

    return frame;
}

} // namespace

RasterizedFrame2D render_evaluated_composition_2d(
    const scene::EvaluatedCompositionState& state,
    const RenderPlan& plan,
    const FrameRenderTask& task) {
    RenderContext context;
    return render_composition_recursive(state.layers, state.width, state.height, plan, task, context);
}

} // namespace tachyon
