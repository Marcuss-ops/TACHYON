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

void add_color_ramp(
    std::vector<ColorKeyframeSpec>& keyframes,
    double start_time,
    ColorSpec start_value,
    double end_time,
    ColorSpec end_value) {

    ColorKeyframeSpec start;
    start.time = start_time;
    start.value = start_value;
    ColorKeyframeSpec end;
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

::tachyon::TextAnimatorSpec make_minimal_fade_animator(
    std::string based_on,
    double reveal_duration_seconds) {

    return make_common_intro_animator(
        std::move(based_on),
        0.0,
        reveal_duration_seconds);
}

} // namespace

TextBackgroundBox make_minimal_text_background_box(
    renderer2d::Color fill_color,
    renderer2d::Color stroke_color,
    float corner_radius,
    float padding_x,
    float padding_y) {

    TextBackgroundBox box;
    box.enabled = true;
    box.fill_color = fill_color;
    box.stroke_color = stroke_color;
    box.corner_radius = corner_radius;
    box.padding_left = padding_x;
    box.padding_right = padding_x;
    box.padding_top = padding_y;
    box.padding_bottom = padding_y;
    return box;
}

TextStyle make_minimal_text_style(
    std::uint32_t pixel_size,
    renderer2d::Color fill_color,
    renderer2d::Color background_fill) {

    TextStyle style;
    style.pixel_size = pixel_size;
    style.fill_color = fill_color;
    style.background_box = make_minimal_text_background_box(background_fill);
    return style;
}

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

::tachyon::TextAnimatorSpec make_typewriter_word_animator(
    double words_per_second,
    bool cursor_enabled,
    std::string cursor_char) {

    const double word_stagger_delay = words_per_second > 0.0 ? 1.0 / words_per_second : 0.25;
    ::tachyon::TextAnimatorSpec animator = make_word_by_word_opacity_animator(word_stagger_delay, 0.45);
    animator.name = "TypewriterWord";
    animator.cursor.enabled = cursor_enabled;
    animator.cursor.cursor_char = std::move(cursor_char);
    animator.cursor.blink_rate = cursor_enabled ? 3.0 : 0.0;
    return animator;
}

::tachyon::TextAnimatorSpec make_typewriter_line_animator(
    double lines_per_second,
    bool cursor_enabled,
    std::string cursor_char) {

    const double line_stagger_delay = lines_per_second > 0.0 ? 1.0 / lines_per_second : 0.5;
    ::tachyon::TextAnimatorSpec animator = make_split_line_stagger_animator(line_stagger_delay, 0.45);
    animator.name = "TypewriterLine";
    animator.cursor.enabled = cursor_enabled;
    animator.cursor.cursor_char = std::move(cursor_char);
    animator.cursor.blink_rate = cursor_enabled ? 3.0 : 0.0;
    return animator;
}

::tachyon::TextAnimatorSpec make_typewriter_terminal_animator(
    double characters_per_second,
    std::string cursor_char) {

    ::tachyon::TextAnimatorSpec animator = make_typewriter_minimal_animator(characters_per_second, true, std::move(cursor_char));
    animator.name = "TypewriterTerminal";
    animator.properties.fill_color_value = ColorSpec{0, 255, 128, 255};
    animator.properties.stroke_width_value = 0.0;
    return animator;
}

::tachyon::TextAnimatorSpec make_typewriter_archive_animator(
    double characters_per_second,
    std::string cursor_char) {

    ::tachyon::TextAnimatorSpec animator = make_typewriter_minimal_animator(characters_per_second, false, std::move(cursor_char));
    animator.name = "TypewriterArchive";
    animator.properties.fill_color_value = ColorSpec{220, 224, 232, 255};
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

::tachyon::TextAnimatorSpec make_minimal_fade_up_animator(
    std::string based_on,
    double reveal_duration_seconds,
    double y_offset_px) {

    ::tachyon::TextAnimatorSpec animator = make_minimal_fade_animator(
        std::move(based_on),
        reveal_duration_seconds);
    animator.name = "MinimalFadeUp";
    add_vector2_ramp(
        animator.properties.position_offset_keyframes,
        0.0,
        math::Vector2{0.0f, static_cast<float>(y_offset_px)},
        reveal_duration_seconds,
        math::Vector2{0.0f, 0.0f});
    return animator;
}

::tachyon::TextAnimatorSpec make_blur_to_focus_animator(
    std::string based_on,
    double reveal_duration_seconds,
    double blur_radius_px) {

    ::tachyon::TextAnimatorSpec animator = make_minimal_fade_animator(
        std::move(based_on),
        reveal_duration_seconds);
    animator.name = "BlurToFocus";
    add_scalar_ramp(
        animator.properties.blur_radius_keyframes,
        0.0,
        blur_radius_px,
        reveal_duration_seconds,
        0.0);
    return animator;
}

::tachyon::TextAnimatorSpec make_typewriter_minimal_animator(
    double characters_per_second,
    bool cursor_enabled,
    std::string cursor_char) {

    ::tachyon::TextAnimatorSpec animator = make_typewriter_animator(characters_per_second, std::move(cursor_char));
    animator.name = "TypewriterMinimal";
    animator.cursor.enabled = cursor_enabled;
    animator.cursor.blink_rate = cursor_enabled ? 3.0 : 0.0;
    return animator;
}

::tachyon::TextAnimatorSpec make_tracking_reveal_animator(
    std::string based_on,
    double reveal_duration_seconds,
    double initial_tracking) {

    ::tachyon::TextAnimatorSpec animator = make_minimal_fade_animator(
        std::move(based_on),
        reveal_duration_seconds);
    animator.name = "TrackingReveal";
    add_scalar_ramp(
        animator.properties.tracking_amount_keyframes,
        0.0,
        initial_tracking,
        reveal_duration_seconds,
        0.0);
    return animator;
}

::tachyon::TextAnimatorSpec make_slide_mask_left_animator(
    std::string based_on,
    double reveal_duration_seconds) {

    ::tachyon::TextAnimatorSpec animator = make_minimal_fade_animator(
        std::move(based_on),
        reveal_duration_seconds);
    animator.name = "SlideMaskLeft";
    animator.selector.shape = "ramp_up";
    animator.selector.start = 0.0;
    animator.selector.end = 100.0;
    animator.selector.stagger_mode = "character";
    animator.selector.stagger_delay = 0.02;
    return animator;
}

::tachyon::TextAnimatorSpec make_split_line_stagger_animator(
    double line_stagger_delay_seconds,
    double reveal_duration_seconds) {

    ::tachyon::TextAnimatorSpec animator = make_minimal_fade_animator("lines", reveal_duration_seconds);
    animator.name = "SplitLineStagger";
    animator.selector.based_on = "lines";
    animator.selector.stagger_mode = "line";
    animator.selector.stagger_delay = line_stagger_delay_seconds;
    return animator;
}

::tachyon::TextAnimatorSpec make_word_by_word_opacity_animator(
    double word_stagger_delay_seconds,
    double reveal_duration_seconds) {

    ::tachyon::TextAnimatorSpec animator = make_minimal_fade_animator("words", reveal_duration_seconds);
    animator.name = "WordByWordOpacity";
    animator.selector.based_on = "words";
    animator.selector.stagger_mode = "word";
    animator.selector.stagger_delay = word_stagger_delay_seconds;
    return animator;
}

::tachyon::TextAnimatorSpec make_underline_draw_animator(
    std::string based_on,
    double reveal_duration_seconds,
    double underline_width_px) {

    ::tachyon::TextAnimatorSpec animator = make_common_intro_animator(
        std::move(based_on),
        0.0,
        reveal_duration_seconds);
    animator.name = "UnderlineDraw";
    animator.properties.stroke_width_value = underline_width_px;
    animator.properties.stroke_color_value = ColorSpec{255, 255, 255, 255};
    add_color_ramp(
        animator.properties.fill_color_keyframes,
        0.0,
        ColorSpec{255, 255, 255, 0},
        reveal_duration_seconds,
        ColorSpec{255, 255, 255, 255});
    return animator;
}

::tachyon::TextAnimatorSpec make_curtain_box_minimal_animator(
    std::string based_on,
    double reveal_duration_seconds) {

    ::tachyon::TextAnimatorSpec animator = make_slide_mask_left_animator(
        std::move(based_on),
        reveal_duration_seconds);
    animator.name = "CurtainBoxMinimal";
    animator.selector.stagger_mode = "line";
    animator.selector.stagger_delay = 0.0;
    return animator;
}

::tachyon::TextAnimatorSpec make_morphing_words_animator(
    double reveal_duration_seconds) {

    ::tachyon::TextAnimatorSpec animator = make_common_intro_animator(
        "words",
        0.0,
        reveal_duration_seconds);
    animator.name = "MorphingWords";
    animator.selector.based_on = "words";
    animator.properties.scale_keyframes.clear();
    add_scalar_ramp(
        animator.properties.scale_keyframes,
        0.0,
        0.96,
        reveal_duration_seconds,
        1.0);
    add_scalar_ramp(
        animator.properties.blur_radius_keyframes,
        0.0,
        3.0,
        reveal_duration_seconds,
        0.0);
    return animator;
}

::tachyon::TextAnimatorSpec make_soft_scale_in_animator(
    std::string based_on,
    double reveal_duration_seconds,
    double initial_scale) {

    ::tachyon::TextAnimatorSpec animator = make_minimal_fade_animator(
        std::move(based_on),
        reveal_duration_seconds);
    animator.name = "SoftScaleIn";
    add_scalar_ramp(
        animator.properties.scale_keyframes,
        0.0,
        initial_scale,
        reveal_duration_seconds,
        1.0);
    return animator;
}

::tachyon::TextAnimatorSpec make_subtle_y_rotate_animator(
    std::string based_on,
    double reveal_duration_seconds,
    double rotation_degrees) {

    ::tachyon::TextAnimatorSpec animator = make_minimal_fade_animator(
        std::move(based_on),
        reveal_duration_seconds);
    animator.name = "SubtleYRotate";
    add_scalar_ramp(
        animator.properties.rotation_keyframes,
        0.0,
        rotation_degrees,
        reveal_duration_seconds,
        0.0);
    return animator;
}

::tachyon::TextAnimatorSpec make_fill_wipe_animator(
    std::string based_on,
    double reveal_duration_seconds) {

    ::tachyon::TextAnimatorSpec animator = make_common_intro_animator(
        std::move(based_on),
        0.0,
        reveal_duration_seconds);
    animator.name = "FillWipe";
    add_color_ramp(
        animator.properties.fill_color_keyframes,
        0.0,
        ColorSpec{255, 255, 255, 0},
        reveal_duration_seconds,
        ColorSpec{255, 255, 255, 255});
    return animator;
}

::tachyon::TextAnimatorSpec make_outline_to_solid_animator(
    std::string based_on,
    double reveal_duration_seconds,
    double outline_width_px) {

    ::tachyon::TextAnimatorSpec animator = make_common_intro_animator(
        std::move(based_on),
        0.0,
        reveal_duration_seconds);
    animator.name = "OutlineToSolid";
    animator.properties.stroke_width_value = outline_width_px;
    add_scalar_ramp(
        animator.properties.stroke_width_keyframes,
        0.0,
        outline_width_px,
        reveal_duration_seconds,
        0.0);
    add_color_ramp(
        animator.properties.stroke_color_keyframes,
        0.0,
        ColorSpec{255, 255, 255, 255},
        reveal_duration_seconds,
        ColorSpec{255, 255, 255, 0});
    return animator;
}

::tachyon::TextAnimatorSpec make_number_flip_minimal_animator(
    std::string based_on,
    double reveal_duration_seconds) {

    ::tachyon::TextAnimatorSpec animator = make_common_intro_animator(
        std::move(based_on),
        0.0,
        reveal_duration_seconds);
    animator.name = "NumberFlipMinimal";
    add_scalar_ramp(
        animator.properties.rotation_keyframes,
        0.0,
        5.0,
        reveal_duration_seconds * 0.5,
        0.0);
    add_scalar_ramp(
        animator.properties.blur_radius_keyframes,
        0.0,
        2.0,
        reveal_duration_seconds,
        0.0);
    return animator;
}

::tachyon::TextAnimatorSpec make_bounce_in_animator(
    std::string based_on,
    double stagger_delay_seconds,
    double reveal_duration_seconds,
    double y_offset_px) {

    ::tachyon::TextAnimatorSpec animator;
    animator.name = "BounceIn";
    animator.selector.type = "all";
    animator.selector.based_on = std::move(based_on);
    animator.selector.stagger_mode = "character";
    animator.selector.stagger_delay = stagger_delay_seconds;

    add_scalar_ramp(
        animator.properties.opacity_keyframes,
        0.0,
        0.0,
        reveal_duration_seconds * 0.55,
        1.0);

    add_vector2_ramp(
        animator.properties.position_offset_keyframes,
        0.0,
        math::Vector2{0.0f, static_cast<float>(y_offset_px)},
        reveal_duration_seconds,
        math::Vector2{0.0f, 0.0f});

    add_scalar_ramp(
        animator.properties.scale_keyframes,
        0.0,
        0.85,
        reveal_duration_seconds * 0.65,
        1.08);

    add_scalar_ramp(
        animator.properties.scale_keyframes,
        reveal_duration_seconds * 0.65,
        1.08,
        reveal_duration_seconds,
        1.0);

    return animator;
}

::tachyon::TextAnimatorSpec make_word_punch_animator(
    double word_stagger_delay_seconds,
    double reveal_duration_seconds,
    double punch_scale) {

    ::tachyon::TextAnimatorSpec animator;
    animator.name = "WordPunch";
    animator.selector.type = "all";
    animator.selector.based_on = "words";
    animator.selector.stagger_mode = "word";
    animator.selector.stagger_delay = word_stagger_delay_seconds;

    add_scalar_ramp(
        animator.properties.opacity_keyframes,
        0.0,
        0.0,
        reveal_duration_seconds * 0.35,
        1.0);

    add_scalar_ramp(
        animator.properties.scale_keyframes,
        0.0,
        0.92,
        reveal_duration_seconds * 0.55,
        punch_scale);

    add_scalar_ramp(
        animator.properties.scale_keyframes,
        reveal_duration_seconds * 0.55,
        punch_scale,
        reveal_duration_seconds,
        1.0);

    add_scalar_ramp(
        animator.properties.blur_radius_keyframes,
        0.0,
        3.0,
        reveal_duration_seconds,
        0.0);

    return animator;
}

} // namespace tachyon::text
