#include "tachyon/core/spec/schema/properties/property_spec.h"
#include "tachyon/core/animation/property_sampler.h"
#include "tachyon/core/animation/property_adapter.h"
#include "tachyon/core/animation/property_interpolation.h"

namespace tachyon {

EffectSpec AnimatedEffectSpec::evaluate(double time_seconds) const {
    EffectSpec result;
    result.enabled = enabled;
    result.type = type;
    
    // Start with static values
    result.scalars = static_scalars;
    result.colors = static_colors;
    result.strings = static_strings;
    
    // Override with animated values at the given time using the centralized property sampler
    for (const auto& [key, anim] : animated_scalars) {
        if (!anim.keyframes.empty()) {
            auto generic_prop = animation::to_generic(anim);
            result.scalars[key] = animation::sample_keyframes(
                generic_prop.keyframes, 
                anim.value.value_or(0.0), 
                time_seconds, 
                animation::lerp_scalar
            );
        } else if (anim.value.has_value()) {
            result.scalars[key] = anim.value.value();
        }
    }
    
    for (const auto& [key, anim] : animated_colors) {
        if (!anim.keyframes.empty()) {
            auto generic_prop = animation::to_generic(anim);
            result.colors[key] = animation::sample_keyframes(
                generic_prop.keyframes, 
                anim.value.value_or(ColorSpec{0,0,0,255}), 
                time_seconds, 
                animation::lerp_color
            );
        } else if (anim.value.has_value()) {
            result.colors[key] = anim.value.value();
        }
    }
    
    return result;
}

} // namespace tachyon
