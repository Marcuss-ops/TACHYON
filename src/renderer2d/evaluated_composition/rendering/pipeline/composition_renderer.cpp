#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"
#include <iostream>
#include "tachyon/renderer2d/resource/precomp_cache.h"
#include "tachyon/renderer2d/effects/core/transitions/transition_utils.h"
#include "tachyon/renderer2d/effects/core/transitions/transition_apply.h"
#include "tachyon/core/spec/schema/objects/background_spec.h"
#include "tachyon/runtime/execution/session/render_internal.h"

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

    const bool has_effectful_layers = std::any_of(
        state.layers.begin(),
        state.layers.end(),
        [](const auto& layer) {
            bool has_transition = (!layer.playback.transition_in.transition_id.empty() && layer.playback.transition_in.transition_id != "none") ||
                                   (!layer.playback.transition_out.transition_id.empty() && layer.playback.transition_out.transition_id != "none");
            return layer.identity.enabled
                && layer.identity.active
                && (!layer.effects.empty() || has_transition);
        });

    if (render_context.policy.tile_size > 0 && !has_effectful_layers) {
        render_pass(*frame_surface, render_context, std::nullopt);
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
