#include "src/runtime/execution/planning/hash/hash_utils.h"

namespace tachyon::hash {

void hash_effect(CacheKeyBuilder& builder, const EffectSpec& effect) {
    builder.add_string(effect.type);
    builder.add_bool(effect.enabled);
    // Hash scalars
    builder.add_u64(static_cast<std::uint64_t>(effect.scalars.size()));
    for (const auto& [key, val] : effect.scalars) {
        builder.add_string(key);
        builder.add_f64(val);
    }
    // Hash colors
    builder.add_u64(static_cast<std::uint64_t>(effect.colors.size()));
    for (const auto& [key, val] : effect.colors) {
        builder.add_string(key);
        builder.add_u64(val.r);
        builder.add_u64(val.g);
        builder.add_u64(val.b);
        builder.add_u64(val.a);
    }
    // Hash strings
    builder.add_u64(static_cast<std::uint64_t>(effect.strings.size()));
    for (const auto& [key, val] : effect.strings) {
        builder.add_string(key);
        builder.add_string(val);
    }
}

void hash_text_animator(CacheKeyBuilder& builder, const TextAnimatorSpec& spec) {
    // Hash selector
    builder.add_string(spec.selector.type);
    builder.add_f64(spec.selector.start);
    builder.add_f64(spec.selector.end);
    builder.add_string(spec.selector.mode);
    builder.add_string(spec.selector.based_on);
    builder.add_bool(spec.selector.random_order);
    builder.add_string(spec.selector.stagger_mode);
    builder.add_f64(spec.selector.stagger_delay);
    builder.add_bool(spec.selector.start_index.has_value());
    if (spec.selector.start_index.has_value()) builder.add_u64(static_cast<std::uint64_t>(*spec.selector.start_index));
    builder.add_bool(spec.selector.end_index.has_value());
    if (spec.selector.end_index.has_value()) builder.add_u64(static_cast<std::uint64_t>(*spec.selector.end_index));
    builder.add_bool(spec.selector.expression.has_value());
    if (spec.selector.expression.has_value()) builder.add_string(*spec.selector.expression);
    builder.add_bool(spec.selector.seed.has_value());
    if (spec.selector.seed.has_value()) builder.add_u64(static_cast<std::uint64_t>(*spec.selector.seed));
    builder.add_bool(spec.selector.amount.has_value());
    if (spec.selector.amount.has_value()) builder.add_f64(*spec.selector.amount);
    builder.add_bool(spec.selector.frequency.has_value());
    if (spec.selector.frequency.has_value()) builder.add_f64(*spec.selector.frequency);

    // Hash properties
    builder.add_bool(spec.properties.tracking_amount_value.has_value());
    if (spec.properties.tracking_amount_value.has_value()) builder.add_f64(*spec.properties.tracking_amount_value);
    hash_keyframes(builder, spec.properties.tracking_amount_keyframes);

    builder.add_bool(spec.properties.fill_color_value.has_value());
    if (spec.properties.fill_color_value.has_value()) {
        builder.add_u64(spec.properties.fill_color_value->r);
        builder.add_u64(spec.properties.fill_color_value->g);
        builder.add_u64(spec.properties.fill_color_value->b);
        builder.add_u64(spec.properties.fill_color_value->a);
    }
    hash_keyframes(builder, spec.properties.fill_color_keyframes);
}

void hash_text_highlight(CacheKeyBuilder& builder, const TextHighlightSpec& spec) {
    builder.add_u64(static_cast<std::uint64_t>(spec.start_glyph));
    builder.add_u64(static_cast<std::uint64_t>(spec.end_glyph));
    builder.add_u64(spec.color.r);
    builder.add_u64(spec.color.g);
    builder.add_u64(spec.color.b);
    builder.add_u64(spec.color.a);
    builder.add_u64(static_cast<std::uint64_t>(spec.padding_x));
    builder.add_u64(static_cast<std::uint64_t>(spec.padding_y));
}

} // namespace tachyon::hash
