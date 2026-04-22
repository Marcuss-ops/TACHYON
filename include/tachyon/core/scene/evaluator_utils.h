#pragma once

#include "tachyon/core/scene/evaluator/hashing.h"
#include "tachyon/core/scene/evaluator/templates.h"
#include "tachyon/core/scene/evaluator/property_sampler.h"

namespace tachyon::scene {
    math::Vector2 fallback_position(const LayerSpec& layer);
    math::Vector2 fallback_scale(const LayerSpec& layer);
    double fallback_rotation(const LayerSpec& layer);
}
