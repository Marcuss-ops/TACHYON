#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"
#include <iostream>
#include "tachyon/renderer2d/resource/precomp_cache.h"
#include "tachyon/renderer2d/effects/core/transitions/transition_utils.h"
#include "tachyon/renderer2d/effects/core/transitions/transition_apply.h"
#include "tachyon/core/spec/schema/objects/background_spec.h"
#include "tachyon/runtime/execution/session/render_internal.h"
#include "tachyon/runtime/cache/frame_cache.h"
#include "tachyon/renderer2d/geometry/dirty_region.h"

#ifdef _MSC_VER
#pragma warning(disable:4456)
#endif
#include "tachyon/renderer2d/evaluated_composition/utilities/composition_utils.h"
#include "tachyon/renderer2d/evaluated_composition/layer_renderer.h"
#include "tachyon/renderer2d/evaluated_composition/effect_renderer.h"
#include "tachyon/renderer2d/effects/effect_host.h"
#include "tachyon/renderer2d/evaluated_composition/renderer2d_matte_resolver.h"
#include "tachyon/renderer2d/color/blending.h"
#include "tachyon/renderer2d/color/color_transfer.h"

#include "tachyon/core/render/dof_settings.h"
#include "tachyon/core/render/motion_blur_settings.h"
#include "tachyon/core/render/visibility.h"
#include "tachyon/core/math/utils/noise.h"
#include "tachyon/renderer2d/core/framebuffer.h"

#include <chrono>
#include <algorithm>
#include <map>
#include <string>
#include <variant>
#include <cmath>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace tachyon {

namespace {

inline void hash_combine(std::uint64_t& seed, std::uint64_t value) {
    seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

struct TileState {
    renderer2d::RectI rect;
    int col{0};
    int row{0};
    bool is_dirty{false};
};

struct TiledFrameGrid {
    int tile_size{128};
    int cols{0};
    int rows{0};
    std::vector<TileState> tiles;
};

TiledFrameGrid build_tiled_frame_grid(int width, int height, const renderer2d::DirtyRegion& dirty_region, int tile_size) {
    TiledFrameGrid grid;
    grid.tile_size = tile_size;
    grid.cols = (width + tile_size - 1) / tile_size;
    grid.rows = (height + tile_size - 1) / tile_size;
    grid.tiles.reserve(grid.cols * grid.rows);

    for (int r = 0; r < grid.rows; ++r) {
        for (int c = 0; c < grid.cols; ++c) {
            int tx = c * tile_size;
            int ty = r * tile_size;
            int tw = std::min(tile_size, width - tx);
            int th = std::min(tile_size, height - ty);
            
            TileState tile;
            tile.rect = renderer2d::RectI{tx, ty, tw, th};
            tile.col = c;
            tile.row = r;
            
            renderer2d::IntRect tile_int_rect{tx, ty, tw, th};
            tile.is_dirty = false;
            
            for (const auto& rect : dirty_region.rects()) {
                if (rect.intersects(tile_int_rect)) {
                    tile.is_dirty = true;
                    break;
                }
            }
            grid.tiles.push_back(tile);
        }
    }
    return grid;
}

renderer2d::BlendMode resolve_blend_mode(const std::string& mode) {
    if (mode == "additive") return renderer2d::BlendMode::Additive;
    if (mode == "multiply") return renderer2d::BlendMode::Multiply;
    if (mode == "screen") return renderer2d::BlendMode::Screen;
    if (mode == "overlay") return renderer2d::BlendMode::Overlay;
    if (mode == "soft_light" || mode == "softLight") return renderer2d::BlendMode::SoftLight;
    return renderer2d::BlendMode::Normal;
}

void record_timing(FrameDiagnostics* diagnostics, const std::string& stage, const std::string& layer_id, std::chrono::time_point<std::chrono::high_resolution_clock> start) {
    if (!diagnostics) return;
    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    diagnostics->add_timing(FrameDiagnostics::kCategoryRender, stage + ":" + layer_id, ms);
}

} // namespace

RasterizedFrame2D render_evaluated_composition_2d(
    const scene::EvaluatedCompositionState& state,
    const render::RenderIntent& intent,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext& render_context,
    const renderer2d::EffectRegistry& effect_registry,
    std::shared_ptr<renderer2d::SurfaceRGBA> output_surface) {

    (void)effect_registry;

    const std::uint32_t working_width = state.width;
    const std::uint32_t working_height = state.height;

    auto frame_surface = output_surface ? output_surface : std::make_shared<renderer2d::SurfaceRGBA>(working_width, working_height);
    frame_surface->clear(renderer2d::Color::transparent());

    FrameDiagnostics* diagnostics = render_context.diagnostics;

    // Build map of layer surfaces to resolve effect dependencies
    std::unordered_map<std::string, std::shared_ptr<renderer2d::SurfaceRGBA>> rendered_surfaces;

    // Resolve Matte Dependencies
    std::vector<spec::MatteDependency> matte_dependencies;
    for (std::size_t i = 0; i < state.layers.size(); ++i) {
        const auto& layer = state.layers[i];
        if (layer.track_matte_type != TrackMatteType::None && layer.track_matte_layer_index.has_value()) {
            std::size_t matte_idx = *layer.track_matte_layer_index;
            if (matte_idx < state.layers.size()) {
                spec::MatteDependency dep;
                dep.target_layer_id = layer.identity.id;
                dep.source_layer_id = state.layers[matte_idx].identity.id;
                switch (layer.track_matte_type) {
                    case TrackMatteType::Alpha: dep.mode = spec::MatteMode::Alpha; break;
                    case TrackMatteType::AlphaInverted: dep.mode = spec::MatteMode::AlphaInverted; break;
                    case TrackMatteType::Luma: dep.mode = spec::MatteMode::Luma; break;
                    case TrackMatteType::LumaInverted: dep.mode = spec::MatteMode::LumaInverted; break;
                    default: dep.mode = spec::MatteMode::Alpha; break;
                }
                matte_dependencies.push_back(dep);
            }
        }
    }

    // Determine if we need a first pass
    const bool needs_first_pass = !matte_dependencies.empty() || std::any_of(
        state.layers.begin(), state.layers.end(),
        [](const auto& l) {
            return l.identity.enabled && l.identity.active
                && (!l.effects.empty() || l.identity.is_adjustment_layer);
        });

    if (needs_first_pass) {
        for (std::size_t i = 0; i < state.layers.size(); ++i) {
            const auto& layer = state.layers[i];
            if (!layer.identity.enabled || !layer.identity.active || layer.identity.id.empty()) continue;
            auto surf = renderer2d::render_layer_surface(layer, state, intent, plan, task, render_context, std::nullopt);
            if (surf) {
                rendered_surfaces[layer.identity.id] = surf;
            }
        }
    }

    auto render_pass = [&](renderer2d::SurfaceRGBA& target_surface, RenderContext& context, const std::optional<renderer2d::RectI>& target_rect) {
        for (std::size_t i = 0; i < state.layers.size(); ++i) {
            if (context.cancel_flag && context.cancel_flag->load()) break;
            const auto& layer = state.layers[i];
            if (!layer.identity.enabled || !layer.identity.active) continue;

            // PR3: DirtyRect-aware layer skipping
            if (plan.quality_policy.dirty_rect_enabled && context.diagnostics && context.diagnostics->invalidation) {
                const auto& inv = *context.diagnostics->invalidation;
                if (!inv.full_frame_invalidation) {
                    const auto l_rect = layer_rect(layer, working_width, working_height, 1.0f);
                    const renderer2d::IntRect layer_bounds{l_rect.x, l_rect.y, l_rect.width, l_rect.height};
                    
                    if (!inv.dirty_region.bounds().intersects(layer_bounds)) {
                        // Skip rendering and compositing if layer doesn't touch the dirty area
                        continue;
                    }
                }
            }

            const auto layer_start = std::chrono::high_resolution_clock::now();

            std::shared_ptr<renderer2d::SurfaceRGBA> layer_surface;
            if (needs_first_pass && !layer.identity.id.empty()) {
                auto it = rendered_surfaces.find(layer.identity.id);
                if (it != rendered_surfaces.end()) {
                    layer_surface = it->second;
                }
            }

            if (!layer_surface) {
                layer_surface = renderer2d::render_layer_surface(layer, state, intent, plan, task, context, target_rect);
            }

            if (!layer_surface) continue;

            // Apply Effects
            if (!layer.effects.empty()) {
                renderer2d::EffectHost& host = renderer2d::effect_host_for(context);
                auto res = renderer2d::apply_effect_pipeline(
                    *layer_surface,
                    layer.effects,
                    host,
                    context.working_color_space.profile,
                    rendered_surfaces,
                    layer.identity.id,
                    context.diagnostics);
                
                if (res.ok()) {
                    layer_surface = std::make_shared<renderer2d::SurfaceRGBA>(std::move(*res.value));
                }
            }

            // Apply Transitions
            const bool has_in_trans = !layer.playback.transition_in.transition_id.empty() && layer.playback.transition_in.transition_id != "none";
            const bool has_out_trans = !layer.playback.transition_out.transition_id.empty() && layer.playback.transition_out.transition_id != "none";
            
            if (has_in_trans || has_out_trans) {
                double layer_time = task.time_seconds - layer.playback.in_time;
                double layer_duration = layer.playback.out_time - layer.playback.in_time;
                
                TransitionApplyRequest apply_request;
                std::string trans_id = "none";
                
                if (has_in_trans && layer_time < layer.playback.transition_in.duration) {
                    trans_id = layer.playback.transition_in.transition_id;
                    apply_request.from = nullptr;
                    apply_request.to = layer_surface.get();
                    apply_request.progress = static_cast<float>(layer_time / layer.playback.transition_in.duration);
                } else if (has_out_trans && layer_time > (layer_duration - layer.playback.transition_out.duration)) {
                    trans_id = layer.playback.transition_out.transition_id;
                    apply_request.from = layer_surface.get();
                    apply_request.to = nullptr;
                    apply_request.progress = static_cast<float>((layer_time - (layer_duration - layer.playback.transition_out.duration)) / layer.playback.transition_out.duration);
                }

                if (trans_id != "none") {
                    apply_request.transition_id = trans_id;
                    static TransitionRegistry s_fallback_registry;
                    const TransitionRegistry& registry = context.transition_registry ? *context.transition_registry : s_fallback_registry;
                    auto apply_res = apply_transition(apply_request, registry);
                    if (apply_res.ok) {
                        layer_surface = std::make_shared<renderer2d::SurfaceRGBA>(std::move(apply_res.output));
                    } else {
                        std::cerr << "[RENDER ERROR] Transition failed for layer " << layer.identity.id << ": " << apply_res.error_message << " (id: " << trans_id << ")\n";
                    }
                }
            }

            // Opacity
            multiply_surface_alpha(*layer_surface, static_cast<float>(layer.transform.opacity));

            // Final Composite
            composite_surface(target_surface, *layer_surface, 0, 0, resolve_blend_mode(layer.transform.blend_mode), -1.0f);
            
            record_timing(diagnostics, "layer_total", layer.identity.id, layer_start);
        }
    };

    const bool has_complex_global_effects = std::any_of(
        state.layers.begin(),
        state.layers.end(),
        [](const auto& layer) {
            bool has_transition = (!layer.playback.transition_in.transition_id.empty() && layer.playback.transition_in.transition_id != "none") ||
                                   (!layer.playback.transition_out.transition_id.empty() && layer.playback.transition_out.transition_id != "none");
            return layer.identity.enabled
                && layer.identity.active
                && (layer.identity.is_adjustment_layer || !layer.effects.empty() || has_transition || layer.identity.type == LayerType::Precomp);
        });

    if (render_context.frame_cache && !has_complex_global_effects) {
        int tile_size = render_context.policy.tile_size > 0 ? render_context.policy.tile_size : 128;
        
        // 1. Analyze and subdivide
        renderer2d::DirtyRegion default_dirty;
        const renderer2d::DirtyRegion* dirty_ptr = &default_dirty;
        bool is_full_frame = true;
        
        if (render_context.diagnostics && render_context.diagnostics->invalidation) {
            dirty_ptr = &render_context.diagnostics->invalidation->dirty_region;
            is_full_frame = render_context.diagnostics->invalidation->full_frame_invalidation;
        } else {
            default_dirty.add(renderer2d::IntRect{0, 0, (int)working_width, (int)working_height});
        }
        
        TiledFrameGrid grid = build_tiled_frame_grid(working_width, working_height, *dirty_ptr, tile_size);
        if (is_full_frame) {
            for (auto& tile : grid.tiles) {
                tile.is_dirty = true;
            }
        }
        
        if (render_context.total_tiles_counter) {
            render_context.total_tiles_counter->fetch_add(grid.tiles.size());
        }
        
        // 2. Process tiles in parallel
        #ifdef _OPENMP
        #pragma omp parallel for schedule(dynamic)
        #endif
        for (int i = 0; i < static_cast<int>(grid.tiles.size()); ++i) {
            auto& tile = grid.tiles[i];
            
            // Intersecting layers for this tile
            std::vector<const scene::EvaluatedLayerState*> intersecting_layers;
            for (const auto& layer : state.layers) {
                if (!layer.identity.enabled || !layer.identity.active) continue;
                
                renderer2d::RectI l_rect = layer_rect(layer, working_width, working_height, 1.0f);
                bool intersects = (l_rect.x < tile.rect.x + tile.rect.width) &&
                                  (l_rect.x + l_rect.width > tile.rect.x) &&
                                  (l_rect.y < tile.rect.y + tile.rect.height) &&
                                  (l_rect.y + l_rect.height > tile.rect.y);
                if (intersects) {
                    intersecting_layers.push_back(&layer);
                }
            }
            
            // Compute TileKey
            std::uint64_t tile_key = 0;
            hash_combine(tile_key, tile.col);
            hash_combine(tile_key, tile.row);
            hash_combine(tile_key, tile_size);
            hash_combine(tile_key, working_width);
            hash_combine(tile_key, working_height);
            
            for (const auto* layer_ptr : intersecting_layers) {
                const auto& layer = *layer_ptr;
                hash_combine(tile_key, std::hash<std::string>{}(layer.identity.layer_id));
                hash_combine(tile_key, static_cast<std::uint64_t>(layer.identity.type));
                hash_combine(tile_key, std::hash<std::string>{}(layer.transform.blend_mode));
                hash_combine(tile_key, std::hash<double>{}(layer.transform.opacity));
                hash_combine(tile_key, std::hash<float>{}(layer.transform.local_transform.position.x));
                hash_combine(tile_key, std::hash<float>{}(layer.transform.local_transform.position.y));
                hash_combine(tile_key, std::hash<float>{}(layer.transform.local_transform.scale.x));
                hash_combine(tile_key, std::hash<float>{}(layer.transform.local_transform.scale.y));
                hash_combine(tile_key, std::hash<float>{}(layer.transform.local_transform.rotation_rad));
                hash_combine(tile_key, std::hash<float>{}(layer.transform.local_transform.anchor_point.x));
                hash_combine(tile_key, std::hash<float>{}(layer.transform.local_transform.anchor_point.y));
                
                if (layer.identity.type == LayerType::Solid) {
                    hash_combine(tile_key, std::hash<float>{}(layer.text.fill_color.r));
                    hash_combine(tile_key, std::hash<float>{}(layer.text.fill_color.g));
                    hash_combine(tile_key, std::hash<float>{}(layer.text.fill_color.b));
                    hash_combine(tile_key, std::hash<float>{}(layer.text.fill_color.a));
                }
                if (layer.identity.type == LayerType::Text) {
                    hash_combine(tile_key, std::hash<std::string>{}(layer.text.content));
                    hash_combine(tile_key, std::hash<std::string>{}(layer.text.font_id));
                    hash_combine(tile_key, std::hash<float>{}(layer.text.font_size));
                    hash_combine(tile_key, std::hash<float>{}(layer.text.fill_color.r));
                    hash_combine(tile_key, std::hash<float>{}(layer.text.fill_color.g));
                    hash_combine(tile_key, std::hash<float>{}(layer.text.fill_color.b));
                    hash_combine(tile_key, std::hash<float>{}(layer.text.fill_color.a));
                }
                if (layer.identity.type == LayerType::Procedural) {
                    hash_combine(tile_key, std::hash<double>{}(layer.playback.local_time_seconds));
                }
            }
            
            std::shared_ptr<renderer2d::SurfaceRGBA> cached_tile;
            bool cache_hit = false;
            
            // Check cache
            cached_tile = render_context.frame_cache->lookup_tile(tile_key);
            if (cached_tile) {
                cache_hit = true;
            }
            
            if (cache_hit) {
                // Fast-path: paste directly
                frame_surface->blit(*cached_tile, tile.rect.x, tile.rect.y);
                if (render_context.diagnostics) {
                    #pragma omp critical
                    {
                        render_context.diagnostics->add_timing("tile", "cache_hit", 0.0);
                    }
                }
            } else {
                // Slow-path: render tile
                auto tile_surface = std::make_shared<renderer2d::SurfaceRGBA>(tile.rect.width, tile.rect.height);
                tile_surface->set_profile(render_context.working_color_space.profile);
                tile_surface->clear(renderer2d::Color::transparent());
                
                for (const auto* layer_ptr : intersecting_layers) {
                    const auto& layer = *layer_ptr;
                    auto layer_tile_surf = renderer2d::render_layer_surface(layer, state, intent, plan, task, render_context, tile.rect);
                    if (layer_tile_surf) {
                        // Opacity & composite
                        multiply_surface_alpha(*layer_tile_surf, static_cast<float>(layer.transform.opacity));
                        composite_surface(*tile_surface, *layer_tile_surf, 0, 0, resolve_blend_mode(layer.transform.blend_mode), -1.0f);
                    }
                }
                
                // Store in cache
                render_context.frame_cache->store_tile(tile_key, tile_surface);
                
                // Paste directly
                frame_surface->blit(*tile_surface, tile.rect.x, tile.rect.y);
                if (render_context.diagnostics) {
                    #pragma omp critical
                    {
                        render_context.diagnostics->add_timing("tile", "cache_miss", 0.0);
                    }
                }
            }
        }
    } else {
        render_pass(*frame_surface, render_context, std::nullopt);
    }

    RasterizedFrame2D result;
    result.surface = frame_surface;
    result.width = working_width;
    result.height = working_height;
    result.frame_number = task.frame_number;
    result.layer_count = state.layers.size();
    
    return result;
}

} // namespace tachyon
