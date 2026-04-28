#pragma once

#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include "tachyon/text/layout/layout.h"

#include <string>
#include <vector>

namespace tachyon::text {

[[nodiscard]] TextBackgroundBox make_minimal_text_background_box(
    renderer2d::Color fill_color = renderer2d::Color{0.06f, 0.07f, 0.09f, 0.38f},
    renderer2d::Color stroke_color = renderer2d::Color{1.0f, 1.0f, 1.0f, 0.0f},
    float corner_radius = 6.0f,
    float padding_x = 10.0f,
    float padding_y = 6.0f);

[[nodiscard]] TextStyle make_minimal_text_style(
    std::uint32_t pixel_size = 72,
    renderer2d::Color fill_color = renderer2d::Color::white(),
    renderer2d::Color background_fill = renderer2d::Color{0.06f, 0.07f, 0.09f, 0.38f});

[[nodiscard]] ::tachyon::TextAnimatorSpec make_fade_in_animator(
    std::string based_on = "characters_excluding_spaces",
    double stagger_delay_seconds = 0.03,
    double reveal_duration_seconds = 0.7);

[[nodiscard]] ::tachyon::TextAnimatorSpec make_slide_in_animator(
    std::string based_on = "characters_excluding_spaces",
    double stagger_delay_seconds = 0.03,
    double slide_distance_px = 28.0,
    double reveal_duration_seconds = 0.7);

[[nodiscard]] ::tachyon::TextAnimatorSpec make_pop_in_animator(
    std::string based_on = "characters",
    double stagger_delay_seconds = 0.02,
    double slide_distance_px = 18.0,
    double reveal_duration_seconds = 0.55);

[[nodiscard]] ::tachyon::TextAnimatorSpec make_phrase_intro_animator(
    std::string based_on = "characters_excluding_spaces",
    double stagger_delay_seconds = 0.03,
    double slide_distance_px = 28.0,
    double reveal_duration_seconds = 0.7);

[[nodiscard]] ::tachyon::TextAnimatorSpec make_numeric_intro_animator(
    std::string based_on = "characters",
    double stagger_delay_seconds = 0.02,
    double slide_distance_px = 18.0,
    double reveal_duration_seconds = 0.55);

[[nodiscard]] ::tachyon::TextAnimatorSpec make_typewriter_animator(
    double characters_per_second = 20.0,
    std::string cursor_char = "|");

[[nodiscard]] ::tachyon::TextAnimatorSpec make_kinetic_blur_animator(
    double slide_distance_px = 200.0,
    double duration_seconds = 0.5);

[[nodiscard]] ::tachyon::TextAnimatorSpec make_minimal_fade_up_animator(
    std::string based_on = "characters_excluding_spaces",
    double reveal_duration_seconds = 0.45,
    double y_offset_px = 12.0);

[[nodiscard]] ::tachyon::TextAnimatorSpec make_blur_to_focus_animator(
    std::string based_on = "characters_excluding_spaces",
    double reveal_duration_seconds = 0.45,
    double blur_radius_px = 8.0);

[[nodiscard]] ::tachyon::TextAnimatorSpec make_typewriter_minimal_animator(
    double characters_per_second = 20.0,
    bool cursor_enabled = false,
    std::string cursor_char = "|");

[[nodiscard]] ::tachyon::TextAnimatorSpec make_tracking_reveal_animator(
    std::string based_on = "characters_excluding_spaces",
    double reveal_duration_seconds = 0.45,
    double initial_tracking = 40.0);

[[nodiscard]] ::tachyon::TextAnimatorSpec make_slide_mask_left_animator(
    std::string based_on = "characters_excluding_spaces",
    double reveal_duration_seconds = 0.55);

[[nodiscard]] ::tachyon::TextAnimatorSpec make_split_line_stagger_animator(
    double line_stagger_delay_seconds = 0.06,
    double reveal_duration_seconds = 0.45);

[[nodiscard]] ::tachyon::TextAnimatorSpec make_word_by_word_opacity_animator(
    double word_stagger_delay_seconds = 0.04,
    double reveal_duration_seconds = 0.35);

[[nodiscard]] ::tachyon::TextAnimatorSpec make_underline_draw_animator(
    std::string based_on = "words",
    double reveal_duration_seconds = 0.35,
    double underline_width_px = 2.0);

[[nodiscard]] ::tachyon::TextAnimatorSpec make_curtain_box_minimal_animator(
    std::string based_on = "characters_excluding_spaces",
    double reveal_duration_seconds = 0.55);

[[nodiscard]] ::tachyon::TextAnimatorSpec make_morphing_words_animator(
    double reveal_duration_seconds = 0.55);

[[nodiscard]] ::tachyon::TextAnimatorSpec make_soft_scale_in_animator(
    std::string based_on = "characters_excluding_spaces",
    double reveal_duration_seconds = 0.45,
    double initial_scale = 0.95);

[[nodiscard]] ::tachyon::TextAnimatorSpec make_subtle_y_rotate_animator(
    std::string based_on = "characters_excluding_spaces",
    double reveal_duration_seconds = 0.45,
    double rotation_degrees = 5.0);

[[nodiscard]] ::tachyon::TextAnimatorSpec make_fill_wipe_animator(
    std::string based_on = "characters_excluding_spaces",
    double reveal_duration_seconds = 0.45);

[[nodiscard]] ::tachyon::TextAnimatorSpec make_outline_to_solid_animator(
    std::string based_on = "characters_excluding_spaces",
    double reveal_duration_seconds = 0.30,
    double outline_width_px = 1.0);

[[nodiscard]] ::tachyon::TextAnimatorSpec make_number_flip_minimal_animator(
    std::string based_on = "characters",
    double reveal_duration_seconds = 0.35);

} // namespace tachyon::text
