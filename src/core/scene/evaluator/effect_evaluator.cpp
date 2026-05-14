#include "tachyon/core/spec/schema/properties/property_spec.h"
#include "tachyon/core/animation/property_sampler.h"
#include "tachyon/core/animation/property_adapter.h"
#include "tachyon/core/animation/property_interpolation.h"

namespace tachyon {

EvaluatedEffect EffectSpec::evaluate(double time_seconds) const {
    EvaluatedEffect result;
    result.enabled = enabled;
    result.type = type;
    
    for (const auto& [key, value] : params) {
        std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, double> || 
                          std::is_same_v<T, bool> || 
                          std::is_same_v<T, std::string> || 
                          std::is_same_v<T, ColorSpec> || 
                          std::is_same_v<T, math::Vector2>) {
                result.params[key] = arg;
            } else if constexpr (std::is_same_v<T, AnimatedScalarSpec>) {
                if (!arg.keyframes.empty()) {
                    auto generic_prop = animation::to_generic(arg);
                    result.params[key] = animation::sample_keyframes(
                        generic_prop.keyframes, 
                        arg.value.value_or(0.0), 
                        time_seconds, 
                        animation::lerp_scalar
                    );
                } else if (arg.value.has_value()) {
                    result.params[key] = *arg.value;
                }
            } else if constexpr (std::is_same_v<T, AnimatedColorSpec>) {
                if (!arg.keyframes.empty()) {
                    auto generic_prop = animation::to_generic(arg);
                    result.params[key] = animation::sample_keyframes(
                        generic_prop.keyframes, 
                        arg.value.value_or(ColorSpec{0,0,0,255}), 
                        time_seconds, 
                        animation::lerp_color
                    );
                } else if (arg.value.has_value()) {
                    result.params[key] = *arg.value;
                }
            } else if constexpr (std::is_same_v<T, AnimatedVector2Spec>) {
                 // Sample vector2 if needed, otherwise static
                 if (arg.value.has_value()) {
                     result.params[key] = *arg.value;
                 }
            }
        }, value);
    }
    
    return result;
}

} // namespace tachyon
