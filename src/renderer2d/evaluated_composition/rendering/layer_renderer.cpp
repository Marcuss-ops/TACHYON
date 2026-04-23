#include "tachyon/renderer2d/evaluated_composition/layer_renderer.h"
#include "tachyon/renderer2d/evaluated_composition/utilities/composition_utils.h"
#include "tachyon/media/management/media_manager.h"
#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"
#include "tachyon/renderer2d/raster/rasterizer.h"
#include "tachyon/renderer2d/color/color_transfer.h"
#include "tachyon/renderer2d/color/blending.h"
#include "tachyon/renderer2d/effects/effect_common.h"
#include "tachyon/text/layout/layout.h"
#include "tachyon/text/animation/text_animator_utils.h"
#include "tachyon/text/animation/text_animator_pipeline.h"
#include "tachyon/text/animation/text_on_path.h"
#include "tachyon/text/fonts/font_registry.h"
#include "tachyon/core/shapes/shape_modifiers.h"
#include <algorithm>
#include <cmath>

namespace tachyon::renderer2d {

namespace {

::tachyon::shapes::ShapePath to_shape_path(const scene::EvaluatedShapePath& path) {
    ::tachyon::shapes::ShapePath out;
    ::tachyon::shapes::ShapeSubpath subpath;
    subpath.closed = path.closed;
    subpath.vertices.reserve(path.points.size());
    for (const auto& point : path.points) {
        subpath.vertices.push_back(::tachyon::shapes::PathVertex{
            point.position,
            point.tangent_in,
            point.tangent_out
        });
    }
    out.subpaths.push_back(std::move(subpath));
    return out;
}

float sample_glyph_alpha(const ::tachyon::text::GlyphBitmap& glyph, float src_x, float src_y) {
    if (glyph.width == 0U || glyph.height == 0U || glyph.alpha_mask.empty()) {
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
        const std::size_t index = static_cast<std::size_t>(y) * glyph.width + x;
        if (index >= glyph.alpha_mask.size()) {
            return 0.0f;
        }
        return static_cast<float>(glyph.alpha_mask[index]) / 255.0f;
    };

    const float a00 = alpha_at(x0, y0);
    const float a10 = alpha_at(x1, y0);
    const float a01 = alpha_at(x0, y1);
    const float a11 = alpha_at(x1, y1);

    const float ax0 = a00 + (a10 - a00) * fx;
    const float ax1 = a01 + (a11 - a01) * fx;
    return ax0 + (ax1 - ax0) * fy;
}

PathGeometry build_shape_geometry(
    const scene::EvaluatedLayerState& layer,
    const ::tachyon::shapes::ShapePath& sp,
    const scene::EvaluatedCompositionState& state,
    const RenderContext2D& context,
    const std::optional<RectI>& target_rect) {

    PathGeometry geom;
    if (sp.empty()) {
        return geom;
    }

    const bool use_camera = layer.is_3d && state.camera.available;
    const float vw = static_cast<float>(state.width) * context.policy.resolution_scale;
    const float vh = static_cast<float>(state.height) * context.policy.resolution_scale;

    auto transform_pt = [&](const math::Vector2& lp) -> math::Vector2 {
        math::Vector3 wp = layer.world_matrix.transform_point({lp.x, lp.y, 0.0f});
        if (use_camera) {
            return state.camera.camera.project_point(wp, vw, vh);
        }
        return {wp.x * context.policy.resolution_scale, wp.y * context.policy.resolution_scale};
    };

    // Check if we have mask path with per-vertex feather
    const bool has_mask_feather = layer.mask_path.has_value();
    const auto& mask_vertices = has_mask_feather ? layer.mask_path->vertices : std::vector<MaskVertex>{};

    for (const auto& subpath : sp.subpaths) {
        if (subpath.vertices.empty()) continue;

        // Get feather values for this vertex if available
        float feather_inner = 0.0f;
        float feather_outer = 0.0f;
        if (has_mask_feather && !mask_vertices.empty()) {
            // Match by index - assuming subpath vertices correspond to mask vertices
            std::size_t idx = static_cast<std::size_t>(&subpath - &sp.subpaths[0]);
            if (idx < mask_vertices.size()) {
                feather_inner = mask_vertices[idx].feather_inner;
                feather_outer = mask_vertices[idx].feather_outer;
            }
        }

        geom.commands.push_back({PathVerb::MoveTo, transform_pt(math::Vector2(subpath.vertices[0].point.x, subpath.vertices[0].point.y)), {}, {}, feather_inner, feather_outer});
        for (std::size_t i = 1; i < subpath.vertices.size(); ++i) {
            const auto& prev = subpath.vertices[i - 1];
            const auto& curr = subpath.vertices[i];
            
            math::Vector2 prev_p(prev.point.x, prev.point.y);
            math::Vector2 prev_out(prev.out_tangent.x, prev.out_tangent.y);
            math::Vector2 curr_p(curr.point.x, curr.point.y);
            math::Vector2 curr_in(curr.in_tangent.x, curr.in_tangent.y);

            // Update feather for this vertex
            if (has_mask_feather && i < mask_vertices.size()) {
                feather_inner = mask_vertices[i].feather_inner;
                feather_outer = mask_vertices[i].feather_outer;
            }

            if (prev_out.length_squared() > 1e-6f || curr_in.length_squared() > 1e-6f) {
                geom.commands.push_back({
                    PathVerb::CubicTo,
                    transform_pt(prev_p + prev_out),
                    transform_pt(curr_p + curr_in),
                    transform_pt(curr_p),
                    feather_inner,
                    feather_outer
                });
            } else {
                geom.commands.push_back({PathVerb::LineTo, transform_pt(curr_p), {}, {}, feather_inner, feather_outer});
            }
        }
        if (subpath.closed) {
            geom.commands.push_back({PathVerb::Close, {}, {}, {}, feather_inner, feather_outer});
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

    return geom;
}

std::shared_ptr<SurfaceRGBA> render_mask_layer_surface(
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

    const Color mask_color{0.0f, 0.0f, 0.0f, static_cast<float>(std::clamp(static_cast<double>(layer.opacity), 0.0, 1.0))};

    if (layer.shape_path.has_value()) {
        const PathGeometry geom = build_shape_geometry(layer, *layer.shape_path, state, context, target_rect);
        if (!geom.commands.empty()) {
            FillPathStyle style;
            style.fill_color = mask_color;
            style.opacity = 1.0f;
            style.winding = WindingRule::NonZero;
            PathRasterizer::fill(*surface, geom, style);
        }
    } else {
        RectI rect = layer_rect(layer, state.width, state.height, context.policy.resolution_scale);
        if (target_rect) {
            rect.x -= target_rect->x;
            rect.y -= target_rect->y;
        }
        if (rect.width > 0 && rect.height > 0) {
            surface->fill_rect(rect, mask_color, true);
        }
    }

    // Note: Feather is now handled in the rasterizer via per-vertex feather values
    // The old post-rasterization blur has been removed

    return surface;
}

} // namespace

std::shared_ptr<SurfaceRGBA> render_text_layer_surface(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& state,
    RenderContext2D& context,
    const std::optional<RectI>& target_rect) {

    const std::int64_t w = target_rect ? target_rect->width : static_cast<std::int64_t>(state.width * context.policy.resolution_scale);
    const std::int64_t h = target_rect ? target_rect->height : static_cast<std::int64_t>(state.height * context.policy.resolution_scale);
    
    std::shared_ptr<SurfaceRGBA> surface = make_surface(w, h, context);

    if (layer.text_content.empty() || !context.font_registry) {
        return surface;
    }

    namespace txt = ::tachyon::text;

    txt::TextStyle style;
    style.pixel_size = static_cast<std::uint32_t>(layer.font_size * context.policy.resolution_scale);
    style.fill_color = from_color_spec(layer.fill_color, context.working_color_space.profile);

    txt::TextBox box;
    box.width = static_cast<std::uint32_t>(layer.width * context.policy.resolution_scale);
    box.height = static_cast<std::uint32_t>(layer.height * context.policy.resolution_scale);
    box.multiline = true;

    txt::TextAlignment align = txt::TextAlignment::Left;
    if (layer.text_alignment == 1) align = txt::TextAlignment::Center;
    else if (layer.text_alignment == 2) align = txt::TextAlignment::Right;
    else if (layer.text_alignment == 3) align = txt::TextAlignment::Justify;

    txt::TextLayoutResult layout = txt::layout_text(*context.font_registry, layer.font_id, layer.text_content, style, box, align);

    const bool use_camera = layer.is_3d && state.camera.available;
    const float vw = static_cast<float>(state.width) * context.policy.resolution_scale;
    const float vh = static_cast<float>(state.height) * context.policy.resolution_scale;

    Color fill_color = from_color_spec(layer.fill_color, context.working_color_space.profile);
    fill_color.a *= static_cast<float>(layer.opacity);

    // Apply new TextAnimatorPipeline
    txt::TextAnimatorContext animator_ctx;
    animator_ctx.time = static_cast<float>(layer.local_time_seconds);
    txt::TextAnimatorPipeline::apply_animators(layout, layer.text_animators, animator_ctx);

    // Apply Text-On-Path if configured
    if (layer.shape_path.has_value() && layer.text_on_path_enabled) {
        txt::TextOnPathModifier::apply(layout, *layer.shape_path, 0.0, true);
    }

    for (const auto& pg : layout.ResolvedTextLayout::glyphs) {
        const txt::Font* font = context.font_registry->find_by_id(pg.font_id);
        if (!font) continue;

        const txt::GlyphBitmap* glyph = font->find_glyph_by_index(pg.font_glyph_index);
        if (!glyph) continue;

        // The pipeline has already modified pg.position, pg.scale, pg.rotation, pg.opacity, pg.fill_color
        auto transform_pos = [&](float lx, float ly) -> math::Vector2 {
            math::Vector3 wp = layer.world_matrix.transform_point({lx, ly, 0.0f});
            if (use_camera) {
                return state.camera.camera.project_point(wp, vw, vh);
            }
            return {wp.x * context.policy.resolution_scale, wp.y * context.policy.resolution_scale};
        };

        math::Vector2 sp = transform_pos(static_cast<float>(pg.position.x), static_cast<float>(pg.position.y));
        
        int tx = static_cast<int>(std::round(sp.x));
        int ty = static_cast<int>(std::round(sp.y));
        const int tw = (std::max)(1, static_cast<int>(std::lround(static_cast<float>(glyph->width) * layout.scale * pg.scale.x)));
        const int th = (std::max)(1, static_cast<int>(std::lround(static_cast<float>(glyph->height) * layout.scale * pg.scale.y)));

        if (target_rect) {
            tx -= target_rect->x;
            ty -= target_rect->y;
        }

        Color final_fill = Color(
            static_cast<float>(pg.fill_color.r) / 255.0f,
            static_cast<float>(pg.fill_color.g) / 255.0f,
            static_cast<float>(pg.fill_color.b) / 255.0f,
            (static_cast<float>(pg.fill_color.a) / 255.0f) * static_cast<float>(layer.opacity)
        );

        for (int y = 0; y < th; ++y) {
            const float src_y = ((static_cast<float>(y) + 0.5f) * static_cast<float>(glyph->height) / static_cast<float>(th)) - 0.5f;
            for (int x = 0; x < tw; ++x) {
                const float src_x = ((static_cast<float>(x) + 0.5f) * static_cast<float>(glyph->width) / static_cast<float>(tw)) - 0.5f;
                const float alpha = sample_glyph_alpha(*glyph, src_x, src_y);
                if (alpha > 0.0f) {
                    surface->blend_pixel(tx + x, ty + y, final_fill, alpha * pg.opacity);
                }
            }
        }
    }

    return surface;
}

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

    if (layer.type == scene::LayerType::Mask) {
        return render_mask_layer_surface(layer, state, context, target_rect);
    }

    if (layer.type == scene::LayerType::Shape && layer.shape_path.has_value()) {
        ::tachyon::shapes::ShapePath modified_path = *layer.shape_path;
        
        // 1. Trim Paths
        if (layer.trim_start > 0.0f || layer.trim_end < 1.0f || std::abs(layer.trim_offset) > 1e-4f) {
            modified_path = shapes::ShapeModifiers::trim_paths(modified_path, layer.trim_start, layer.trim_end, layer.trim_offset);
        }

        PathGeometry geom = build_shape_geometry(layer, modified_path, state, context, target_rect);

        if (layer.fill_color.a > 0 || layer.gradient_fill.has_value()) {
            FillPathStyle style;
            style.fill_color = from_color_spec(layer.fill_color, context.working_color_space.profile);
            style.gradient = layer.gradient_fill;
            style.opacity = static_cast<float>(layer.opacity);
            PathRasterizer::fill(*surface, geom, style);
        }

        if (layer.stroke_width > 0.0f && (layer.stroke_color.a > 0 || layer.gradient_stroke.has_value())) {
            StrokePathStyle style;
            style.stroke_color = from_color_spec(layer.stroke_color, context.working_color_space.profile);
            style.gradient = layer.gradient_stroke;
            style.stroke_width = layer.stroke_width;
            style.opacity = static_cast<float>(layer.opacity);
            style.cap = layer.line_cap;
            style.join = layer.line_join;
            style.miter_limit = layer.miter_limit;
            PathRasterizer::stroke(*surface, geom, style);
        }
    } else if (layer.type == scene::LayerType::Video && context.media_manager && layer.asset_path.has_value()) {
        const auto* texture = context.media_manager->get_video_frame(*layer.asset_path, layer.local_time_seconds);
        if (texture) {
            float image_width = (layer.width > 0) ? (float)layer.width : (float)texture->width();
            float image_height = (layer.height > 0) ? (float)layer.height : (float)texture->height();
            
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

                TexturedQuadPrimitive quad = TexturedQuadPrimitive::custom(v0, v1, v2, v3, texture, from_color_spec(layer.fill_color, context.working_color_space.profile));
                if (target_rect) {
                    for (auto& v : quad.vertices) {
                        v.x -= static_cast<float>(target_rect->x);
                        v.y -= static_cast<float>(target_rect->y);
                    }
                }
                CPURasterizer::draw_textured_quad(*surface, quad);
        }
    } else if (layer.type == scene::LayerType::Image && context.media_manager && layer.asset_path.has_value()) {
        std::filesystem::path resolved = context.media_manager->resolve_media_path(*layer.asset_path);
        const auto* texture = context.media_manager->get_image(resolved);
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

            TexturedQuadPrimitive quad = TexturedQuadPrimitive::custom(v0, v1, v2, v3, texture, from_color_spec(layer.fill_color, context.working_color_space.profile));
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

            TexturedQuadPrimitive quad = TexturedQuadPrimitive::custom(v0, v1, v2, v3, white_tex.get(), from_color_spec(layer.fill_color, context.working_color_space.profile));
            quad.tint = apply_opacity(from_color_spec(layer.fill_color, context.working_color_space.profile), layer.opacity);

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
                Color color = from_color_spec(layer.fill_color, context.working_color_space.profile);
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

    if (layer.type == scene::LayerType::Precomp) {
        return render_precomp_surface(layer, state, plan, task, context);
    } else {
        return render_simple_layer_surface(layer, state, context, target_rect);
    }
}

} // namespace tachyon::renderer2d
