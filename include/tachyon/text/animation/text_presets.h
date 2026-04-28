#pragma once

#include "tachyon/core/spec/schema/animation/text_animator_spec.h"

#include <string>

namespace tachyon::text {

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

} // namespace tachyon::text
