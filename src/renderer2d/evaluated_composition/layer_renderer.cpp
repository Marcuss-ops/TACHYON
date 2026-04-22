#include "tachyon/renderer2d/evaluated_composition/layer_renderer.h"
#include "composition_utils.h"
#include "tachyon/media/media_manager.h"
#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"
#include "tachyon/renderer2d/raster/rasterizer.h"
#include "tachyon/renderer2d/color/color_transfer.h"
#include "tachyon/renderer2d/color/blending.h"
#include <algorithm>
#include <cmath>

namespace tachyon::renderer2d {

std::shared_ptr<SurfaceRGBA> render_precomp_surface(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& state,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext2D& context) {

    if (!layer.nested_composition) {
        return make_surface(state.width, state.height, context);
    }

    const std::string cache_key = layer.precomp_id.value_or(layer.id) + ":" + std::to_string(task.frame_number);
    if (context.precomp_cache) {
        if (const auto cached = context.precomp_cache->lookup(cache_key)) {
            auto mutable_cached = std::const_pointer_cast<SurfaceRGBA>(cached);
            return mutable_cached;
        }
    }

    const RasterizedFrame2D nested = render_evaluated_composition_2d(*layer.nested_composition, plan, task, context);
    std::shared_ptr<SurfaceRGBA> surface = nested.surface 
        ? nested.surface 
        : make_surface(layer.nested_composition->width, layer.nested_composition->height, context);

    if (context.precomp_cache) {
        context.precomp_cache->store(cache_key, surface);
    }
    return surface;
}

std::shared_ptr<SurfaceRGBA> render_simple_layer_surface(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& state,
    RenderContext2D& context,
    const std::optional<RectI>& target_rect) {

    const std::int64_t w = target_rect ? target_rect->width : static_cast<std::int64_t>(state.width * context.policy.resolution_scale);
    const std::int64_t h = target_rect ? target_rect->height : static_cast<std::int64_t>(state.height * context.policy.resolution_scale);
    
    std::shared_ptr<SurfaceRGBA> surface = make_surface(w, h, context);

    if (!layer.visible || !layer.enabled || !layer.active) {
        return surface;
    }

    if (layer.type == scene::LayerType::Shape && layer.shape_path.has_value()) {
        const auto& sp = *layer.shape_path;
        PathGeometry geom;
        
        const bool use_camera = layer.is_3d && state.camera.available;
        const float vw = static_cast<float>(state.width) * context.policy.resolution_scale;
        const float vh = static_cast<float>(state.height) * context.policy.resolution_scale;

        auto transform_pt = [&](const math::Vector2& lp) -> math::Vector2 {
            math::Vector3 wp = layer.world_matrix.transform_point({lp.x, lp.y, 0.0f});
            if (use_camera) {
                return state.camera.camera.project_point(wp, vw, vh);
            }
            return {wp.x, wp.y};
        };

        if (!sp.points.empty()) {
            geom.commands.push_back({PathVerb::MoveTo, transform_pt(sp.points[0].position)});
            for (std::size_t i = 1; i < sp.points.size(); ++i) {
                const auto& prev = sp.points[i-1];
                const auto& curr = sp.points[i];
                if (prev.tangent_out.length_squared() > 1e-6f || curr.tangent_in.length_squared() > 1e-6f) {
                    geom.commands.push_back({
                        PathVerb::CubicTo, 
                        transform_pt(prev.position + prev.tangent_out), 
                        transform_pt(curr.position + curr.tangent_in), 
                        transform_pt(curr.position)
                    });
                } else {
                    geom.commands.push_back({PathVerb::LineTo, transform_pt(curr.position)});
                }
            }
            if (sp.closed) {
                geom.commands.push_back({PathVerb::Close});
            }
        }

        if (target_rect) {
            for (auto& cmd : geom.commands) {
                cmd.p0.x -= static_cast<float>(target_rect->x);
                cmd.p0.y -= static_cast<float>(target_rect->y);
                cmd.p1.x -= static_cast<float>(target_rect->x);
                cmd.p1.y -= static_cast<float>(target_rect->y);
                cmd.p2.x -= static_cast<float>(target_rect->x);
                cmd.p2.y -= static_cast<float>(target_rect->y);
            }
        }

        if (layer.trim_start > 0.0f || layer.trim_end < 1.0f || std::abs(layer.trim_offset) > 1e-4f) {
            geom = PathRasterizer::trim(geom, layer.trim_start, layer.trim_end, layer.trim_offset);
        }

        if (layer.fill_color.a > 0 || layer.gradient_fill.has_value()) {
            FillPathStyle style;
            style.fill_color = from_color_spec(layer.fill_color);
            style.gradient = layer.gradient_fill;
            style.opacity = static_cast<float>(layer.opacity);
            PathRasterizer::fill(*surface, geom, style);
        }

        if (layer.stroke_width > 0.0f && (layer.stroke_color.a > 0 || layer.gradient_stroke.has_value())) {
            StrokePathStyle style;
            style.stroke_color = from_color_spec(layer.stroke_color);
            style.gradient = layer.gradient_stroke;
            style.stroke_width = layer.stroke_width;
            style.opacity = static_cast<float>(layer.opacity);
            style.cap = layer.line_cap;
            style.join = layer.line_join;
            style.miter_limit = layer.miter_limit;
            PathRasterizer::stroke(*surface, geom, style);
        }
    } else if (layer.type == scene::LayerType::Image && context.media_manager && layer.asset_path.has_value()) {
        const auto* texture = context.media_manager->get_image(*layer.asset_path);
        if (texture) {
            float image_width = 0.0f;
            float image_height = 0.0f;
            if (layer.width > 0) image_width = (float)layer.width; else image_width = (float)texture->width();
            if (layer.height > 0) image_height = (float)layer.height; else image_height = (float)texture->height();

            const bool use_camera = layer.is_3d && state.camera.available;
            const float vw = static_cast<float>(state.width) * context.policy.resolution_scale;
            const float vh = static_cast<float>(state.height) * context.policy.resolution_scale;

            auto transform_point = [&](float x, float y) -> TexturedVertex2D {
                math::Vector3 wp = layer.world_matrix.transform_point({x, y, 0.0f});
                float inv_w = 1.0f;
                math::Vector2 sp;
                if (use_camera) {
                    sp = state.camera.camera.project_point(wp, vw, vh);
                    math::Matrix4x4 vp = state.camera.camera.get_projection_matrix() * state.camera.camera.get_view_matrix();
                    const float w_clip = vp[3]*wp.x + vp[7]*wp.y + vp[11]*wp.z + vp[15];
                    inv_w = (w_clip > 0.001f) ? 1.0f / w_clip : 0.0f;
                } else {
                    sp = {wp.x * context.policy.resolution_scale, wp.y * context.policy.resolution_scale};
                }
                return TexturedVertex2D{sp.x, sp.y, x / image_width, y / image_height, inv_w};
            };

            const TexturedVertex2D v0 = transform_point(0.0f, 0.0f);
            const TexturedVertex2D v1 = transform_point(image_width, 0.0f);
            const TexturedVertex2D v2 = transform_point(image_width, image_height);
            const TexturedVertex2D v3 = transform_point(0.0f, image_height);

            TexturedQuadPrimitive quad = TexturedQuadPrimitive::custom(v0, v1, v2, v3, texture, from_color_spec(layer.fill_color));
            if (target_rect) {
                for (auto& v : quad.vertices) {
                    v.x -= static_cast<float>(target_rect->x);
                    v.y -= static_cast<float>(target_rect->y);
                }
            }
            CPURasterizer::draw_textured_quad(*surface, quad);
        }
    } else {
        const bool use_camera = layer.is_3d && state.camera.available;
        if (use_camera) {
            const float solid_w = static_cast<float>(layer.width);
            const float solid_h = static_cast<float>(layer.height);
            const float vw = static_cast<float>(state.width) * context.policy.resolution_scale;
            const float vh = static_cast<float>(state.height) * context.policy.resolution_scale;

            auto transform_point = [&](float x, float y) -> TexturedVertex2D {
                math::Vector3 wp = layer.world_matrix.transform_point({x, y, 0.0f});
                math::Vector2 sp = state.camera.camera.project_point(wp, vw, vh);
                math::Matrix4x4 vp = state.camera.camera.get_projection_matrix() * state.camera.camera.get_view_matrix();
                const float w_clip = vp[3]*wp.x + vp[7]*wp.y + vp[11]*wp.z + vp[15];
                const float inv_w = (w_clip > 0.001f) ? 1.0f / w_clip : 0.0f;
                return TexturedVertex2D{sp.x, sp.y, x / solid_w, y / solid_h, inv_w};
            };

            TexturedVertex2D v0 = transform_point(0.0f, 0.0f);
            TexturedVertex2D v1 = transform_point(solid_w, 0.0f);
            TexturedVertex2D v2 = transform_point(solid_w, solid_h);
            TexturedVertex2D v3 = transform_point(0.0f, solid_h);

            static std::shared_ptr<SurfaceRGBA> white_tex = nullptr;
            if (!white_tex) {
                white_tex = std::make_shared<SurfaceRGBA>(1, 1);
                white_tex->clear(Color::white());
            }

            TexturedQuadPrimitive quad = TexturedQuadPrimitive::custom(v0, v1, v2, v3, white_tex.get(), from_color_spec(layer.fill_color));
            quad.tint = apply_opacity(from_color_spec(layer.fill_color), layer.opacity);

            if (target_rect) {
                for (auto& v : quad.vertices) {
                    v.x -= static_cast<float>(target_rect->x);
                    v.y -= static_cast<float>(target_rect->y);
                }
            }
            CPURasterizer::draw_textured_quad(*surface, quad);
        } else {
            RectI rect = layer_rect(layer, state.width, state.height, context.policy.resolution_scale);
            if (target_rect) {
                rect.x -= target_rect->x;
                rect.y -= target_rect->y;
            }
            if (rect.width > 0 && rect.height > 0) {
                Color color = from_color_spec(layer.fill_color);
                color = apply_opacity(color, layer.opacity);
                surface->fill_rect(rect, color, true);
            }
        }
    }

    return surface;
}

std::shared_ptr<SurfaceRGBA> render_layer_surface(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& state,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext2D& context,
    const std::optional<RectI>& target_rect) {

    switch (layer.type) {
    case scene::LayerType::Precomp:
        return render_precomp_surface(layer, state, plan, task, context);
    case scene::LayerType::NullLayer: {
        return make_surface(state.width, state.height, context);
    }
    case scene::LayerType::Mask:
    case scene::LayerType::Solid:
    case scene::LayerType::Shape:
    case scene::LayerType::Image:
    case scene::LayerType::Text:
    default:
        return render_simple_layer_surface(layer, state, context, target_rect);
    }
}

} // namespace tachyon::renderer2d
