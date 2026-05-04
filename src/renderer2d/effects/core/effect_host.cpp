#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/utility/number_counter_effect.h"
#include "tachyon/renderer2d/resource/render_context.h"
#include "tachyon/runtime/profiling/render_profiler.h"
#include <stdexcept>
#include <utility>
#include <iostream>

namespace tachyon::renderer2d {

// Concrete implementation of EffectHost
class EffectHostImpl : public EffectHost {
public:
    void register_effect(std::string name, std::unique_ptr<Effect> effect) override {
        if (!effect) throw std::invalid_argument("effect must not be null");
        m_effects[std::move(name)] = std::move(effect);
    }
    
    bool has_effect(const std::string& name) const override {
        if (m_effects.find(name) != m_effects.end()) return true;
        return EffectRegistry::instance().find(name) != nullptr;
    }
    
    ResolutionResult<SurfaceRGBA> apply(const std::string& name, const SurfaceRGBA& input, const EffectParams& params) const override {
        ResolutionResult<SurfaceRGBA> result;
        
        // 1. Try old-style internal effects (legacy path)
        auto it = m_effects.find(name);
        if (it != m_effects.end()) {
            profiling::ProfileScope scope(params.context ? params.context->profiler : nullptr, profiling::ProfileEventType::Effect, name, -1, std::string(), name);
            result.value = it->second->apply(input, params);
            return result;
        }

        // 2. Try new-style registry effects
        if (auto* desc = EffectRegistry::instance().find(name)) {
            profiling::ProfileScope scope(params.context ? params.context->profiler : nullptr, profiling::ProfileEventType::Effect, name, -1, std::string(), name);
            
            // Resolve auxiliary surfaces
            std::vector<const SurfaceRGBA*> aux_surfaces;
            for (const auto& req : desc->aux_requirements) {
                auto aux_it = params.aux_surfaces.find(req.param_name);
                if (aux_it != params.aux_surfaces.end()) {
                    aux_surfaces.push_back(aux_it->second);
                } else if (req.is_required) {
                    std::string msg = "Effect '" + name + "' requires auxiliary surface '" + req.param_name + "' (" + req.semantic + ") which was not provided.";
                    result.diagnostics.add_error("EFFECT_AUX_MISSING", msg);
                    return result; // Strict failure: missing required aux surface
                } else {
                    aux_surfaces.push_back(nullptr);
                }
            }

            SurfaceRGBA output;
            desc->factory(EffectSpec{}, input, output, aux_surfaces, params);
            result.value = std::move(output);
            return result;
        }

        // 3. Strict failure: Unknown effect
        std::string msg = "Unknown effect '" + name + "'. No fallback allowed in strict mode.";
        result.diagnostics.add_error("EFFECT_NOT_FOUND", msg);
        
        // If we are in permissive mode (which we don't have yet in EffectParams, 
        // but let's assume if we had it, we'd check it here), we might return input.
        // For now, we return nullopt to signal failure.
        return result;
    }
    
    ResolutionResult<SurfaceRGBA> apply_pipeline(const SurfaceRGBA& input, const std::vector<std::pair<std::string, EffectParams>>& pipeline) const override {
        ResolutionResult<SurfaceRGBA> result;
        result.value = input;
        
        for (const auto& step : pipeline) {
            auto step_res = apply(step.first, *result.value, step.second);
            if (!step_res.ok()) {
                result.diagnostics.append(step_res.diagnostics);
                result.value = std::nullopt;
                return result; // Stop the pipeline on first error
            }
            result.value = std::move(step_res.value);
        }
        
        return result;
    }
};

std::unique_ptr<EffectHost> create_effect_host() {
    return std::make_unique<EffectHostImpl>();
}

void EffectHost::register_builtins(EffectHost& host) {
    EffectRegistry::instance().register_builtins();
}

} // namespace tachyon::renderer2d
