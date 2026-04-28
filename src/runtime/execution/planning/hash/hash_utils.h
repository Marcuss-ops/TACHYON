#pragma once

#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/cache/cache_key_builder.h"
#include <vector>
#include <cstdint>

namespace tachyon::hash {

template <typename T>
void hash_keyframes(CacheKeyBuilder& builder, const std::vector<T>& keyframes) {
    builder.add_u64(static_cast<std::uint64_t>(keyframes.size()));
    for (const auto& kf : keyframes) {
        builder.add_f64(kf.time);
        builder.add_u64(static_cast<std::uint64_t>(kf.easing));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cx1 * 1000000.0));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cy1 * 1000000.0));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cx2 * 1000000.0));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cy2 * 1000000.0));
        builder.add_f64(kf.speed_in);
        builder.add_f64(kf.influence_in);
        builder.add_f64(kf.speed_out);
        builder.add_f64(kf.influence_out);
    }
}

void hash_animated_scalar(CacheKeyBuilder& builder, const AnimatedScalarSpec& spec);
void hash_animated_vector2(CacheKeyBuilder& builder, const AnimatedVector2Spec& spec);
void hash_animated_vector3(CacheKeyBuilder& builder, const AnimatedVector3Spec& spec);
void hash_animated_color(CacheKeyBuilder& builder, const AnimatedColorSpec& spec);

void hash_effect(CacheKeyBuilder& builder, const EffectSpec& effect);
void hash_text_animator(CacheKeyBuilder& builder, const TextAnimatorSpec& spec);
void hash_text_highlight(CacheKeyBuilder& builder, const TextHighlightSpec& spec);

} // namespace tachyon::hash
