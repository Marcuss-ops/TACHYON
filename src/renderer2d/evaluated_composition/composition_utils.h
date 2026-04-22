#pragma once

#include "tachyon/renderer2d/raster/rasterizer.h"
#include "tachyon/renderer2d/resource/render_context.h"
#include "tachyon/core/spec/scene_spec.h"

namespace tachyon {

renderer2d::Color from_color_spec(const ColorSpec& spec);
renderer2d::Color apply_opacity(renderer2d::Color color, double opacity);

renderer2d::RectI layer_rect(const scene::EvaluatedLayerState& layer, std::int64_t comp_width, std::int64_t comp_height, float resolution_scale);
renderer2d::RectI shape_bounds_from_path(const scene::EvaluatedShapePath& path);

std::shared_ptr<renderer2d::SurfaceRGBA> make_surface(std::int64_t width, std::int64_t height, renderer2d::RenderContext2D& context);
void multiply_surface_alpha(renderer2d::SurfaceRGBA& surface, float factor);
void apply_mask(renderer2d::SurfaceRGBA& surface, const renderer2d::SurfaceRGBA& mask, TrackMatteType type);

void composite_surface(
    renderer2d::SurfaceRGBA& dst,
    const renderer2d::SurfaceRGBA& src,
    int offset_x,
    int offset_y,
    renderer2d::BlendMode blend_mode = renderer2d::BlendMode::Normal);

} // namespace tachyon
