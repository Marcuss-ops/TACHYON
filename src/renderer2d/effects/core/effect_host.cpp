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
    
    SurfaceRGBA apply(const std::string& name, const SurfaceRGBA& input, const EffectParams& params) const override {
        // 1. Try old-style internal effects
        auto it = m_effects.find(name);
        if (it != m_effects.end()) {
            profiling::ProfileScope scope(params.context ? params.context->profiler : nullptr, profiling::ProfileEventType::Effect, name, -1, std::string(), name);
            return it->second->apply(input, params);
        }

        // 2. Try new-style registry effects
        if (auto* desc = EffectRegistry::instance().find(name)) {
            profiling::ProfileScope scope(params.context ? params.context->profiler : nullptr, profiling::ProfileEventType::Effect, name, -1, std::string(), name);
            SurfaceRGBA output;
            // Note: In this transition phase, we pass empty aux surfaces if they aren't explicitly provided in EffectParams.
            // A more complete integration would resolve aux surfaces from the context/composition.
            std::vector<const SurfaceRGBA*> aux_surfaces;
            desc->factory(EffectSpec{}, input, output, aux_surfaces, params);
            return output;
        }

        // 3. Fallback: Strict reporting
        std::cerr << "[Tachyon] Error: Unknown effect '" << name << "'. No fallback applied." << std::endl;
        // In the future, we should return a ResolutionResult or add to context diagnostics.
        return input;
    }
    
    SurfaceRGBA apply_pipeline(const SurfaceRGBA& input, const std::vector<std::pair<std::string, EffectParams>>& pipeline) const override {
        SurfaceRGBA current = input;
        for (const auto& step : pipeline) {
            current = apply(step.first, current, step.second);
        }
        return current;
    }
};

std::unique_ptr<EffectHost> create_effect_host() {
    return std::make_unique<EffectHostImpl>();
}

void EffectHost::register_builtins(EffectHost& host) {
    // Builtins are now handled by the EffectRegistry.
    // This method is kept for backward compatibility but delegates to the registry.
    EffectRegistry::instance().register_builtins();
}

} // namespace tachyon::renderer2d
