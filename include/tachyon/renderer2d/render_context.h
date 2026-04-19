#pragma once

#include "tachyon/renderer2d/rasterizer.h"
#include "tachyon/renderer2d/effect_host.h"

#include <map>
#include <memory>
#include <string>

namespace tachyon {

struct RasterizedFrame2D;

namespace renderer2d {

struct RenderContext {
    std::map<std::string, RasterizedFrame2D> precomp_cache;
    EffectHost effects;

    RenderContext() {
        effects.register_effect("gaussian_blur", std::make_unique<GaussianBlurEffect>());
        effects.register_effect("drop_shadow", std::make_unique<DropShadowEffect>());
        effects.register_effect("glow", std::make_unique<GlowEffect>());
        effects.register_effect("levels", std::make_unique<LevelsEffect>());
        effects.register_effect("curves", std::make_unique<CurvesEffect>());
        effects.register_effect("fill", std::make_unique<FillEffect>());
        effects.register_effect("tint", std::make_unique<TintEffect>());
    }
};

} // namespace renderer2d
} // namespace tachyon
