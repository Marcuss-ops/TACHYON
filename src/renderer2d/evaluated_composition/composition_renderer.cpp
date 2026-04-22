#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"
#include "composition_utils.h"
#include "tachyon/renderer2d/evaluated_composition/layer_renderer.h"
#include "tachyon/renderer2d/evaluated_composition/effect_renderer.h"
#include "tachyon/renderer2d/color/blending.h"
#include "tachyon/renderer2d/color/color_transfer.h"
#include "tachyon/renderer3d/ray_tracer.h"
#include "tachyon/runtime/execution/tile_scheduler.h"
#include <algorithm>
#include <cmath>

namespace tachyon {
using namespace renderer2d;

RasterizedFrame2D render_evaluated_composition_2d(
    const scene::EvaluatedCompositionState& state,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext2D& context) {

    RasterizedFrame2D frame;
    frame.frame_number = task.frame_number;
    frame.width = state.width;
    frame.height = state.height;
    frame.layer_count = state.layers.size();
    frame.backend_name = "cpu-evaluated-composition";
    frame.cache_key = task.cache_key.value;
    frame.note = "Frame rasterized from evaluated composition";
    
    const float res_scale = plan.quality_policy.resolution_scale;
    const std::uint32_t working_width = static_cast<std::uint32_t>(std::max<std::int64_t>(1, static_cast<std::int64_t>(std::round(static_cast<float>(state.width) * res_scale))));
    const std::uint32_t working_height = static_cast<std::uint32_t>(std::max<std::int64_t>(1, static_cast<std::int64_t>(std::round(static_cast<float>(state.height) * res_scale))));

    frame.surface = std::make_shared<SurfaceRGBA>(working_width, working_height);
    if (!frame.surface) {
        return frame;
    }

    auto& dst = *frame.surface;
    dst.clear(Color::transparent());

    EffectHost& host = effect_host_for(context);

    // Identify if we have 3D layers and trigger 3D pass if available
    bool has_any_3d = std::any_of(state.layers.begin(), state.layers.end(), [](const auto& l) { return l.is_3d && l.visible; });
    if (has_any_3d && !context.ray_tracer) {
        context.ray_tracer = std::make_shared<renderer3d::RayTracer>();
    }

    auto render_pass = [&](SurfaceRGBA& target_surface, const std::optional<RectI>& tile_rect = std::nullopt) {
        // AE Rendering Order: Bottom to Top
        for (std::size_t i = 0; i < state.layers.size(); ++i) {
            const auto& layer = state.layers[i];
            if (!layer.enabled || !layer.active) {
                continue;
            }

            if (layer.is_3d && layer.visible) {
                if (tile_rect) {
                    continue; // Skip 3D blocks in tiled mode for now
                }
                
                // Found a 3D block. Collect contiguous 3D layers.
                std::vector<std::size_t> block_indices;
                block_indices.push_back(i);
                std::size_t last_block_idx = i;
                for (std::size_t j = i + 1; j < state.layers.size(); ++j) {
                    if (state.layers[j].is_3d && state.layers[j].visible && state.layers[j].enabled && state.layers[j].active) {
                        block_indices.push_back(j);
                        last_block_idx = j;
                    } else if (state.layers[j].enabled && state.layers[j].active) {
                        break;
                    }
                }

                // Render the 3D block
                context.ray_tracer->set_samples_per_pixel(context.policy.ray_tracer_spp);
                context.ray_tracer->build_scene_subset(state, block_indices, context.font);
                
                auto world_3d = make_surface(state.width, state.height, context);
                std::vector<float> hdr_buffer(static_cast<std::size_t>(state.width) * state.height * 4, 0.0f);
                context.ray_tracer->render(state, hdr_buffer.data(), nullptr, static_cast<int>(state.width), static_cast<int>(state.height));
                
                for (std::int64_t px_idx = 0; px_idx < state.width * state.height; ++px_idx) {
                    const float r = hdr_buffer[px_idx * 4 + 0];
                    const float g = hdr_buffer[px_idx * 4 + 1];
                    const float b = hdr_buffer[px_idx * 4 + 2];
                    const float a = hdr_buffer[px_idx * 4 + 3];
                    world_3d->set_pixel(static_cast<std::uint32_t>(px_idx % state.width), static_cast<std::uint32_t>(px_idx / state.width), {r, g, b, a});
                }

                composite_surface(target_surface, *world_3d, 0, 0, BlendMode::Normal);
                i = last_block_idx;
                continue;
            }

            auto layer_surface = render_layer_surface(layer, state, plan, task, context, tile_rect);

            if (layer.is_adjustment_layer) {
                if (context.policy.effects_enabled) {
                    auto adjusted = apply_effect_pipeline(target_surface, layer.effects, host);
                    multiply_surface_alpha(adjusted, static_cast<float>(layer.opacity));
                    composite_surface(target_surface, adjusted, 0, 0, BlendMode::Normal);
                }
                continue;
            }

            // Effects
            if (context.policy.effects_enabled && !layer.effects.empty()) {
                auto effect_surface = apply_effect_pipeline(*layer_surface, layer.effects, host);
                *layer_surface = std::move(effect_surface);
            }

            // Opacity
            multiply_surface_alpha(*layer_surface, static_cast<float>(layer.opacity));

            // Track Matte
            if (layer.track_matte_layer_index.has_value() && layer.track_matte_type != TrackMatteType::None) {
                const auto matte_idx = *layer.track_matte_layer_index;
                if (matte_idx < state.layers.size()) {
                    const auto& matte_layer = state.layers[matte_idx];
                    auto matte_surface = render_layer_surface(matte_layer, state, plan, task, context, tile_rect);
                    apply_mask(*layer_surface, *matte_surface, layer.track_matte_type);
                }
            }

            // Final Composite
            int ox = 0, oy = 0;
            composite_surface(target_surface, *layer_surface, ox, oy, parse_blend_mode(layer.blend_mode));
        }
    };

    if (context.policy.tile_size > 0) {
        TileGrid grid = build_tile_grid({0, 0, static_cast<int>(working_width), static_cast<int>(working_height)}, working_width, working_height, context.policy.tile_size);
        
        #pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < static_cast<int>(grid.tiles.size()); ++i) {
            const auto& tile = grid.tiles[i];
            SurfaceRGBA tile_surface(static_cast<std::uint32_t>(tile.width), static_cast<std::uint32_t>(tile.height));
            tile_surface.clear(Color::transparent());
            
            RenderContext2D thread_context = context;
            render_pass(tile_surface, tile);
            
            #pragma omp critical
            {
                dst.blit(tile_surface, tile.x, tile.y);
            }
        }
    } else {
        render_pass(dst);
    }

    return frame;
}

} // namespace tachyon
