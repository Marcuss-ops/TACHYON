#pragma once

#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/text/animation/text_presets.h"
#include "tachyon/core/types/colors.h"
#include "tachyon/presets/preset_base.h"

namespace tachyon::presets::text {

using ::tachyon::text::make_minimal_fade_up_animator;
using ::tachyon::text::make_blur_to_focus_animator;
using ::tachyon::text::make_soft_scale_in_animator;
using ::tachyon::text::make_fade_in_animator;
using ::tachyon::text::make_slide_in_animator;
using ::tachyon::text::make_typewriter_animator;
using ::tachyon::text::make_kinetic_blur_animator;
using ::tachyon::text::make_tracking_reveal_animator;
using ::tachyon::text::make_phrase_intro_animator;

// Text layer preset factory functions
// These create reusable text layer specs that can be used in any scene

struct TextParams : LayerParams {
    std::string id = "headline";
    std::string text = "Hello World";
    std::string font_id;
    double font_size = 72.0;
    int width = 1920;
    int height = 200;
    int position_x = 960;
    int position_y = 540;
    std::string alignment = "center";
    ColorSpec fill_color{238, 242, 248, 245};
    double start_time = 0.0;
    double duration = 5.0;
    std::vector<TextAnimatorSpec> animators;
};

[[nodiscard]] inline LayerSpec build_minimal(
    const TextParams& opts = {}) {
    LayerSpec text;
    text.id = opts.id;
    text.name = "Minimal Text";
    text.type = "text";
    text.enabled = true;
    text.visible = true;
    text.start_time = opts.start_time;
    text.in_point = opts.start_time;
    text.out_point = opts.duration;
    text.width = opts.width;
    text.height = opts.height;
    text.alignment = opts.alignment;
    text.text_content = opts.text;
    text.font_id = opts.font_id;
    text.font_size.value = opts.font_size;
    text.fill_color = AnimatedColorSpec(opts.fill_color);
    text.transform.position_x = static_cast<double>(opts.position_x);
    text.transform.position_y = static_cast<double>(opts.position_y);

    if (!opts.animators.empty()) {
        text.text_animators = opts.animators;
    } else {
        text.text_animators.push_back(
            text::make_minimal_fade_up_animator("characters_excluding_spaces", 0.45, 12.0));
        text.text_animators.push_back(
            text::make_blur_to_focus_animator("characters_excluding_spaces", 0.45, 8.0));
        text.text_animators.push_back(
            text::make_soft_scale_in_animator("characters_excluding_spaces", 0.45, 0.95));
    }

    return text;
}

[[nodiscard]] inline LayerSpec build_enhanced(
    const TextParams& opts = {}) {
    LayerSpec text;
    text.id = opts.id;
    text.name = "Enhanced Text";
    text.type = "text";
    text.enabled = true;
    text.visible = true;
    text.start_time = opts.start_time;
    text.in_point = opts.start_time;
    text.out_point = opts.duration;
    text.width = opts.width;
    text.height = opts.height;
    text.alignment = opts.alignment;
    text.text_content = opts.text;
    text.font_id = opts.font_id;
    text.font_size.value = opts.font_size;
    text.fill_color = AnimatedColorSpec(opts.fill_color);
    text.transform.position_x = static_cast<double>(opts.position_x);
    text.transform.position_y = static_cast<double>(opts.position_y);

    if (!opts.animators.empty()) {
        text.text_animators = opts.animators;
    } else {
        text.text_animators.push_back(
            text::make_fade_in_animator("characters_excluding_spaces", 0.03, 0.7));
        text.text_animators.push_back(
            text::make_slide_in_animator("characters_excluding_spaces", 0.03, 28.0, 0.7));
        text.text_animators.push_back(
            text::make_soft_scale_in_animator("characters_excluding_spaces", 0.55, 0.96));
    }

    return text;
}

[[nodiscard]] inline LayerSpec build_typewriter(
    const TextParams& opts = {}) {
    LayerSpec text;
    text.id = opts.id;
    text.name = "Typewriter Text";
    text.type = "text";
    text.enabled = true;
    text.visible = true;
    text.start_time = opts.start_time;
    text.in_point = opts.start_time;
    text.out_point = opts.duration;
    text.width = opts.width;
    text.height = opts.height;
    text.alignment = opts.alignment;
    text.text_content = opts.text;
    text.font_id = opts.font_id;
    text.font_size.value = opts.font_size;
    text.fill_color = AnimatedColorSpec(opts.fill_color);
    text.transform.position_x = static_cast<double>(opts.position_x);
    text.transform.position_y = static_cast<double>(opts.position_y);
    text.text_animators.push_back(text::make_typewriter_animator(20.0, "|"));
    return text;
}

[[nodiscard]] inline LayerSpec build_kinetic(
    const TextParams& opts = {}) {
    LayerSpec text;
    text.id = opts.id;
    text.name = "Kinetic Text";
    text.type = "text";
    text.enabled = true;
    text.visible = true;
    text.start_time = opts.start_time;
    text.in_point = opts.start_time;
    text.out_point = opts.duration;
    text.width = opts.width;
    text.height = opts.height;
    text.alignment = opts.alignment;
    text.text_content = opts.text;
    text.font_id = opts.font_id;
    text.font_size.value = opts.font_size;
    text.fill_color = AnimatedColorSpec(opts.fill_color);
    text.transform.position_x = static_cast<double>(opts.position_x);
    text.transform.position_y = static_cast<double>(opts.position_y);
    text.text_animators.push_back(text::make_kinetic_blur_animator(200.0, 0.5));
    text.text_animators.push_back(
        text::make_tracking_reveal_animator("characters_excluding_spaces", 0.45, 40.0));
    return text;
}

[[nodiscard]] inline LayerSpec build_phrase_intro(
    const TextParams& opts = {}) {
    LayerSpec text;
    text.id = opts.id;
    text.name = "Phrase Intro Text";
    text.type = "text";
    text.enabled = true;
    text.visible = true;
    text.start_time = opts.start_time;
    text.in_point = opts.start_time;
    text.out_point = opts.duration;
    text.width = opts.width;
    text.height = opts.height;
    text.alignment = opts.alignment;
    text.text_content = opts.text;
    text.font_id = opts.font_id;
    text.font_size.value = opts.font_size;
    text.fill_color = AnimatedColorSpec(opts.fill_color);
    text.transform.position_x = static_cast<double>(opts.position_x);
    text.transform.position_y = static_cast<double>(opts.position_y);
    text.text_animators.push_back(
        text::make_phrase_intro_animator("characters_excluding_spaces", 0.03, 28.0, 0.7));
    return text;
}

} // namespace tachyon::presets::text
