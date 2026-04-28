#include "src/runtime/execution/planning/hash/hash_utils.h"

namespace tachyon::hash {

void hash_animated_scalar(CacheKeyBuilder& builder, const AnimatedScalarSpec& spec) {
    builder.add_bool(spec.value.has_value());
    if (spec.value.has_value()) {
        builder.add_f64(*spec.value);
    }
    builder.add_u64(static_cast<std::uint64_t>(spec.keyframes.size()));
    for (const auto& kf : spec.keyframes) {
        builder.add_f64(kf.time);
        builder.add_f64(kf.value);
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
    builder.add_bool(spec.audio_band.has_value());
    if (spec.audio_band.has_value()) {
        builder.add_u32(static_cast<std::uint32_t>(*spec.audio_band));
    }
    builder.add_f64(spec.audio_min);
    builder.add_f64(spec.audio_max);
    builder.add_bool(spec.expression.has_value());
    if (spec.expression.has_value()) {
        builder.add_string(*spec.expression);
    }
}

void hash_animated_vector2(CacheKeyBuilder& builder, const AnimatedVector2Spec& spec) {
    builder.add_bool(spec.value.has_value());
    if (spec.value.has_value()) {
        builder.add_f64(spec.value->x);
        builder.add_f64(spec.value->y);
    }
    builder.add_u64(static_cast<std::uint64_t>(spec.keyframes.size()));
    for (const auto& kf : spec.keyframes) {
        builder.add_f64(kf.time);
        builder.add_f64(kf.value.x);
        builder.add_f64(kf.value.y);
        builder.add_f64(kf.tangent_in.x);
        builder.add_f64(kf.tangent_in.y);
        builder.add_f64(kf.tangent_out.x);
        builder.add_f64(kf.tangent_out.y);
        builder.add_u32(static_cast<std::uint32_t>(kf.easing));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cx1 * 1000000.0));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cy1 * 1000000.0));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cx2 * 1000000.0));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cy2 * 1000000.0));
        builder.add_f64(kf.speed_in);
        builder.add_f64(kf.influence_in);
        builder.add_f64(kf.speed_out);
        builder.add_f64(kf.influence_out);
    }
    builder.add_bool(spec.expression.has_value());
    if (spec.expression.has_value()) {
        builder.add_string(*spec.expression);
    }
}

void hash_animated_vector3(CacheKeyBuilder& builder, const AnimatedVector3Spec& spec) {
    builder.add_bool(spec.value.has_value());
    if (spec.value.has_value()) {
        builder.add_f64(spec.value->x);
        builder.add_f64(spec.value->y);
        builder.add_f64(spec.value->z);
    }
    builder.add_u64(static_cast<std::uint64_t>(spec.keyframes.size()));
    for (const auto& kf : spec.keyframes) {
        builder.add_f64(kf.time);
        builder.add_f64(kf.value.x);
        builder.add_f64(kf.value.y);
        builder.add_f64(kf.value.z);
        builder.add_f64(kf.tangent_in.x);
        builder.add_f64(kf.tangent_in.y);
        builder.add_f64(kf.tangent_in.z);
        builder.add_f64(kf.tangent_out.x);
        builder.add_f64(kf.tangent_out.y);
        builder.add_f64(kf.tangent_out.z);
        builder.add_u32(static_cast<std::uint32_t>(kf.easing));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cx1 * 1000000.0));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cy1 * 1000000.0));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cx2 * 1000000.0));
        builder.add_u64(static_cast<std::uint64_t>(kf.bezier.cy2 * 1000000.0));
        builder.add_f64(kf.speed_in);
        builder.add_f64(kf.influence_in);
        builder.add_f64(kf.speed_out);
        builder.add_f64(kf.influence_out);
    }
    builder.add_bool(spec.expression.has_value());
    if (spec.expression.has_value()) {
        builder.add_string(*spec.expression);
    }
}

void hash_animated_color(CacheKeyBuilder& builder, const AnimatedColorSpec& spec) {
    builder.add_bool(spec.value.has_value());
    if (spec.value.has_value()) {
        builder.add_u32(spec.value->r);
        builder.add_u32(spec.value->g);
        builder.add_u32(spec.value->b);
        builder.add_u32(spec.value->a);
    }
    builder.add_u64(static_cast<std::uint64_t>(spec.keyframes.size()));
    for (const auto& kf : spec.keyframes) {
        builder.add_f64(kf.time);
        builder.add_u32(kf.value.r);
        builder.add_u32(kf.value.g);
        builder.add_u32(kf.value.b);
        builder.add_u32(kf.value.a);
        builder.add_u32(static_cast<std::uint32_t>(kf.easing));
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

} // namespace tachyon::hash
