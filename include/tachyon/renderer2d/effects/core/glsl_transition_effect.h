#pragma once

#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/transition_registry.h"

namespace tachyon::renderer2d {

class GlslTransitionEffect : public Effect {
public:
    explicit GlslTransitionEffect(const tachyon::TransitionRegistry& registry) : m_registry(registry) {}
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;

private:
    const tachyon::TransitionRegistry& m_registry;
};

}  // namespace tachyon::renderer2d


