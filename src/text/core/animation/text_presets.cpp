#include "tachyon/text/animation/text_presets.h"

#include "tachyon/core/math/algebra/vector2.h"

#include <utility>

namespace tachyon::text {

namespace {

void add_scalar_ramp(
    std::vector<ScalarKeyframeSpec>& keyframes,
    double start_time,
    double start_value,
    double end_time,
    double end_value) {

    ScalarKeyframeSpec start;
    start.time = start_time;
    start.value = start_value;
    ScalarKeyframeSpec end;
    end.time = end_time;
    end.value = end_value;
    keyframes.push_back(start);
    keyframes.push_back(end);
}

void add_vector2_ramp(
    std::vector<Vector2KeyframeSpec>& keyframes,
    double start_time,
    math::Vector2 start_value,
    double end_time,
    math::Vector2 end_value) {

    Vector2KeyframeSpec start;
    start.time = start_time;
    start.value = start_value;
    Vector2KeyframeSpec end;
    end.time = end_time;
    end.value = end_value;
    keyframes.push_back(start);
    keyframes.push_back(end);
}

::tachyon::TextAnimatorSpec make_common_intro_animator(
    std::string based_on,
    double stagger_delay_seconds,
    double reveal_duration_seconds) {

    ::tachyon::TextAnimatorSpec animator;
    animator.selector.type = "all";
    animator.selector.based_on = std::move(based_on);
    animator.selector.stagger_mode = "character";
    animator.selector.stagger_delay = stagger_delay_seconds;
    add_scalar_ramp(animator.properties.opacity_keyframes, 0.0, 0.0, reveal_duration_seconds, 1.0);
    add_scalar_ramp(animator.properties.reveal_keyframes, 0.0, 0.0, reveal_duration_seconds, 1.0);
    return animator;
}

} // namespace

::tachyon::TextAnimatorSpec make_fade_in_animator(
    std::string based_on,
    double stagger_delay_seconds,
    double reveal_duration_seconds) {

    return make_common_intro_animator(
        std::move(based_on),
        stagger_delay_seconds,
        reveal_duration_seconds);
}

::tachyon::TextAnimatorSpec make_slide_in_animator(
    std::string based_on,
    double stagger_delay_seconds,
    double slide_distance_px,
    double reveal_duration_seconds) {

    ::tachyon::TextAnimatorSpec animator = make_common_intro_animator(
        std::move(based_on),
        stagger_delay_seconds,
        reveal_duration_seconds);

    add_vector2_ramp(
        animator.properties.position_offset_keyframes,
        0.0,
        math::Vector2{0.0f, static_cast<float>(slide_distance_px)},
        reveal_duration_seconds,
        math::Vector2{0.0f, 0.0f});

    return animator;
}

::tachyon::TextAnimatorSpec make_pop_in_animator(
    std::string based_on,
    double stagger_delay_seconds,
    double slide_distance_px,
    double reveal_duration_seconds) {

    ::tachyon::TextAnimatorSpec animator = make_slide_in_animator(
        std::move(based_on),
        stagger_delay_seconds,
        slide_distance_px,
        reveal_duration_seconds);

    add_scalar_ramp(
        animator.properties.scale_keyframes,
        0.0,
        0.96,
        reveal_duration_seconds,
        1.0);

    add_scalar_ramp(
        animator.properties.tracking_amount_keyframes,
        0.0,
        18.0,
        reveal_duration_seconds,
        0.0);

    return animator;
}

::tachyon::TextAnimatorSpec make_phrase_intro_animator(
    std::string based_on,
    double stagger_delay_seconds,
    double slide_distance_px,
    double reveal_duration_seconds) {

    return make_slide_in_animator(
        std::move(based_on),
        stagger_delay_seconds,
        slide_distance_px,
        reveal_duration_seconds);
}

::tachyon::TextAnimatorSpec make_numeric_intro_animator(
    std::string based_on,
    double stagger_delay_seconds,
    double slide_distance_px,
    double reveal_duration_seconds) {

    return make_pop_in_animator(
        std::move(based_on),
        stagger_delay_seconds,
        slide_distance_px,
        reveal_duration_seconds);
}

::tachyon::TextAnimatorSpec make_typewriter_animator(
    double characters_per_second,
    std::string cursor_char) {

    ::tachyon::TextAnimatorSpec animator;
    animator.name = "Typewriter";
    animator.selector.type = "range";
    animator.selector.based_on = "characters";
    animator.selector.stagger_mode = "character";
    animator.selector.stagger_delay = 1.0 / characters_per_second;
    
    // Opacity ramp (instant appear after stagger)
    add_scalar_ramp(animator.properties.opacity_keyframes, 0.0, 0.0, 0.01, 1.0);
    
    // Cursor settings
    animator.cursor.enabled = true;
    animator.cursor.cursor_char = std::move(cursor_char);
    animator.cursor.blink_rate = 4.0;
    
    return animator;
}

::tachyon::TextAnimatorSpec make_kinetic_blur_animator(
    double slide_distance_px,
    double duration_seconds) {

    ::tachyon::TextAnimatorSpec animator;
    animator.name = "KineticBlur";
    animator.selector.type = "all";
    animator.selector.stagger_mode = "character";
    animator.selector.stagger_delay = 0.05;
    
    // High speed movement for motion blur demonstration
    add_vector2_ramp(
        animator.properties.position_offset_keyframes,
        0.0,
        math::Vector2{static_cast<float>(slide_distance_px), 0.0f},
        duration_seconds,
        math::Vector2{0.0f, 0.0f});
        
    // Opacity fade in
    add_scalar_ramp(animator.properties.opacity_keyframes, 0.0, 0.0, duration_seconds * 0.5, 1.0);

    return animator;
}

} // namespace tachyon::text

