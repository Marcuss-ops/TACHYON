#include "tachyon/renderer2d/evaluated_composition/effect_renderer.h"
#include "tachyon/renderer2d/evaluated_composition/utilities/composition_utils.h"
#include "tachyon/renderer2d/color/color_transfer.h"
#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/effect_resolver.h"
#include "tachyon/transition_registry.h"

#include <chrono>

namespace tachyon::renderer2d {

EffectHost& effect_host_for(RenderContext2D& context) {
    if (context.effects) {
        return *context.effects;
    }
    
    // Fallback path for code that doesn't use the session manager.
    // In production, context.effects should always be set by RenderSession.
    static std::unique_ptr<EffectHost> s_fallback_host = [] {
        static EffectRegistry registry;
        static TransitionRegistry transition_registry;
        register_builtin_transitions(transition_registry);
        register_builtin_effects(registry, transition_registry);
        return create_effect_host(registry);
    }();
    
    return *s_fallback_host;
}

EffectParams effect_params_from_spec(const EffectSpec& spec, const ColorProfile& working_profile) {
    EffectParams params;
    for (const auto& [key, value] : spec.scalars) {
        params.scalars.emplace(key, static_cast<float>(value));
    }
    for (const auto& [key, value] : spec.colors) {
        params.colors.emplace(key, from_color_spec(value, working_profile));
    }
    for (const auto& [key, value] : spec.strings) {
        params.strings.emplace(key, value);
    }
    return params;
}

EffectParams effect_params_from_spec(const EffectSpec& spec) {
    return effect_params_from_spec(spec, ColorProfile::Rec709());
}

ResolutionResult<SurfaceRGBA> apply_effect_pipeline(
    const SurfaceRGBA& input,
    const std::vector<EffectSpec>& effects,
    EffectHost& host,
    const ColorProfile& working_profile,
    FrameDiagnostics* diagnostics,
    const std::string& current_layer_id) {
    return apply_effect_pipeline(input, effects, host, working_profile, {}, current_layer_id, diagnostics);
}

ResolutionResult<SurfaceRGBA> apply_effect_pipeline(
    const SurfaceRGBA& input,
    const std::vector<EffectSpec>& effects,
    EffectHost& host,
    const ColorProfile& working_profile,
    const std::unordered_map<std::string, std::shared_ptr<SurfaceRGBA>>& surfaces,
    const std::string& current_layer_id,
    FrameDiagnostics* diagnostics) {

    ResolutionResult<SurfaceRGBA> result;
    result.value = input;
    
    for (const auto& effect : effects) {
        if (!effect.enabled || effect.type.empty()) {
            continue;
        }
        
        EffectParams params = effect_params_from_spec(effect, working_profile);
        params.strings.emplace("layer_id", current_layer_id);
        
        // Use centralized effect resolver with injected registry
        auto resolved = resolve_effect(effect, host.registry());
        
        if (!resolved.valid) {
            result.diagnostics.add_error("EFFECT_RESOLUTION_FAILED", resolved.error_message);
            result.value = std::nullopt;
            return result;
        }
        
        // Resolve auxiliary surfaces from descriptor requirements
        if (resolved.descriptor) {
            for (const auto& req : resolved.descriptor->aux_requirements) {
                std::string target_layer_id;
                
                // 1. Try to get ID from defined source_key in spec
                if (!req.source_key.empty()) {
                    if (const auto it = effect.strings.find(req.source_key); it != effect.strings.end()) {
                        target_layer_id = it->second;
                    }
                }
                
                // 2. Fallback to default_id
                if (target_layer_id.empty()) {
                    target_layer_id = req.default_id;
                }
                
                // 3. Resolve surface pointer
                if (!target_layer_id.empty()) {
                    const auto surface_it = surfaces.find(target_layer_id);
                    if (surface_it != surfaces.end() && surface_it->second) {
                        params.aux_surfaces.emplace(req.param_name, surface_it->second.get());
                    }
                }
            }
        }

        const auto start = std::chrono::high_resolution_clock::now();
        auto step_res = host.apply(effect.type, *result.value, params);
        
        if (diagnostics) {
            const auto end = std::chrono::high_resolution_clock::now();
            diagnostics->timings.push_back(TimingSample{
                "effect",
                current_layer_id.empty() ? effect.type : (current_layer_id + ":" + effect.type),
                std::chrono::duration<double, std::milli>(end - start).count()
            });
            diagnostics->diagnostics.append(step_res.diagnostics);
        }

        if (!step_res.ok()) {
            result.diagnostics.append(step_res.diagnostics);
            result.value = std::nullopt;
            return result; // Strict failure
        }
        
        result.value = std::move(step_res.value);
    }
    
    return result;
}

} // namespace tachyon::renderer2d
