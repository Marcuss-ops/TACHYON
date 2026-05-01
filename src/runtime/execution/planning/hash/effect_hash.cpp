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
    builder.add_string(spec.name);
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
    builder.add_string(spec.selector.shape);
    builder.add_f64(spec.selector.offset);
    builder.add_f64(spec.selector.ease_high);
    builder.add_f64(spec.selector.ease_low);
    builder.add_string(spec.selector.mode);

    // Hash properties - opacity
    builder.add_bool(spec.properties.opacity_value.has_value());
    if (spec.properties.opacity_value.has_value()) builder.add_f64(*spec.properties.opacity_value);
    hash_keyframes(builder, spec.properties.opacity_keyframes);

    // Hash properties - position_offset
    builder.add_bool(spec.properties.position_offset_value.has_value());
    if (spec.properties.position_offset_value.has_value()) {
        builder.add_f64(spec.properties.position_offset_value->x);
        builder.add_f64(spec.properties.position_offset_value->y);
    }
    hash_keyframes(builder, spec.properties.position_offset_keyframes);

    // Hash properties - scale
    builder.add_bool(spec.properties.scale_value.has_value());
    if (spec.properties.scale_value.has_value()) builder.add_f64(*spec.properties.scale_value);
    hash_keyframes(builder, spec.properties.scale_keyframes);

    // Hash properties - rotation
    builder.add_bool(spec.properties.rotation_value.has_value());
    if (spec.properties.rotation_value.has_value()) builder.add_f64(*spec.properties.rotation_value);
    hash_keyframes(builder, spec.properties.rotation_keyframes);

    // Hash properties - tracking_amount
    builder.add_bool(spec.properties.tracking_amount_value.has_value());
    if (spec.properties.tracking_amount_value.has_value()) builder.add_f64(*spec.properties.tracking_amount_value);
    hash_keyframes(builder, spec.properties.tracking_amount_keyframes);

    // Hash properties - fill_color
    builder.add_bool(spec.properties.fill_color_value.has_value());
    if (spec.properties.fill_color_value.has_value()) {
        builder.add_u64(spec.properties.fill_color_value->r);
        builder.add_u64(spec.properties.fill_color_value->g);
        builder.add_u64(spec.properties.fill_color_value->b);
        builder.add_u64(spec.properties.fill_color_value->a);
    }
    hash_keyframes(builder, spec.properties.fill_color_keyframes);

    // Hash properties - stroke_color
    builder.add_bool(spec.properties.stroke_color_value.has_value());
    if (spec.properties.stroke_color_value.has_value()) {
        builder.add_u64(spec.properties.stroke_color_value->r);
        builder.add_u64(spec.properties.stroke_color_value->g);
        builder.add_u64(spec.properties.stroke_color_value->b);
        builder.add_u64(spec.properties.stroke_color_value->a);
    }
    hash_keyframes(builder, spec.properties.stroke_color_keyframes);

    // Hash properties - stroke_width
    builder.add_bool(spec.properties.stroke_width_value.has_value());
    if (spec.properties.stroke_width_value.has_value()) builder.add_f64(*spec.properties.stroke_width_value);
    hash_keyframes(builder, spec.properties.stroke_width_keyframes);

    // Hash properties - blur_radius
    builder.add_bool(spec.properties.blur_radius_value.has_value());
    if (spec.properties.blur_radius_value.has_value()) builder.add_f64(*spec.properties.blur_radius_value);
    hash_keyframes(builder, spec.properties.blur_radius_keyframes);

    // Hash properties - reveal
    builder.add_bool(spec.properties.reveal_value.has_value());
    if (spec.properties.reveal_value.has_value()) builder.add_f64(*spec.properties.reveal_value);
    hash_keyframes(builder, spec.properties.reveal_keyframes);

    // Hash cursor
    builder.add_bool(spec.cursor.enabled);
    builder.add_string(spec.cursor.cursor_char);
    builder.add_f64(spec.cursor.blink_rate);
    builder.add_bool(spec.cursor.color_override.has_value());
    if (spec.cursor.color_override.has_value()) {
        builder.add_u64(spec.cursor.color_override->r);
        builder.add_u64(spec.cursor.color_override->g);
        builder.add_u64(spec.cursor.color_override->b);
        builder.add_u64(spec.cursor.color_override->a);
    }
    builder.add_bool(spec.cursor.follow_last_glyph);
    builder.add_f64(spec.cursor.offset_x);
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
