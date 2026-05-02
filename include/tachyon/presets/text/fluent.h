#pragma once

#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include "tachyon/text/animation/text_presets.h"
#include "tachyon/core/types/colors.h"
#include "tachyon/presets/text/text_params.h"
#include <string>
#include <vector>
#include <functional>

namespace tachyon::presets {
LayerSpec build_text(const struct TextParams& p);
}

namespace tachyon::presets::text {

/**
 * @brief Fluent builder for text layers with Remotion-style API.
 * 
 * Usage:
 * @code
 * auto title = headline("Hello World")
 *     .font("Inter")
 *     .font_size(96)
 *     .color({255, 255, 255, 255})
 *     .center()
 *     .animate(fade_up().stagger(0.03).duration(0.6))
 *     .build();
 * @endcode
 */
class TextBuilder {
    TextParams params_;
    std::vector<TextAnimatorSpec> animators_;
    ColorSpec stroke_color_{0, 0, 0, 0};
    double stroke_width_{0.0};
    
public:
    explicit TextBuilder(std::string text) {
        params_.text = std::move(text);
    }
    
    TextBuilder& font(std::string font_id) {
        params_.font_id = std::move(font_id);
        return *this;
    }
    
    TextBuilder& font_size(double font_size) {
        params_.font_size = static_cast<uint32_t>(font_size);
        return *this;
    }

    TextBuilder& fill_color(const ColorSpec& c) {
        params_.color = c;
        return *this;
    }

    TextBuilder& stroke_color(const ColorSpec& c) {
        stroke_color_ = c;
        return *this;
    }

    TextBuilder& stroke_width(double w) {
        stroke_width_ = w;
        return *this;
    }

    TextBuilder& text_box(double w, double h) {
        params_.text_w = static_cast<float>(w);
        params_.text_h = static_cast<float>(h);
        return *this;
    }
    
    TextBuilder& color(const ColorSpec& c) {
        params_.color = c;
        return *this;
    }
    
    TextBuilder& alignment(std::string align) {
        params_.alignment = std::move(align);
        return *this;
    }
    
    TextBuilder& left() {
        params_.alignment = "left";
        return *this;
    }
    
    TextBuilder& center() {
        params_.alignment = "center";
        return *this;
    }
    
    TextBuilder& right() {
        params_.alignment = "right";
        return *this;
    }
    
    TextBuilder& position(double x, double y) {
        params_.x = static_cast<float>(x);
        params_.y = static_cast<float>(y);
        return *this;
    }
    
    TextBuilder& size(double w, double h) {
        params_.text_w = static_cast<float>(w);
        params_.text_h = static_cast<float>(h);
        return *this;
    }
    
    TextBuilder& duration(double d) {
        params_.out_point = static_cast<float>(d);
        return *this;
    }
    
    TextBuilder& animate(const TextAnimatorSpec& animator) {
        animators_.push_back(animator);
        return *this;
    }
    
    TextBuilder& animate(std::vector<TextAnimatorSpec> animators) {
        animators_.insert(animators_.end(), animators.begin(), animators.end());
        return *this;
    }
    
    // Build the LayerSpec
    [[nodiscard]] LayerSpec build() const {
        TextParams p = params_;
        if (!animators_.empty()) {
            p.animations = animators_;
        }
        LayerSpec l = tachyon::presets::build_text(p);
        if (stroke_width_ > 0.0) {
            l.stroke_width = stroke_width_;
            l.stroke_width_property = AnimatedScalarSpec{stroke_width_};
            l.stroke_color = AnimatedColorSpec(stroke_color_);
        }
        return l;
    }
    
    // Implicit conversion to LayerSpec
    [[nodiscard]] operator LayerSpec() const {
        return build();
    }
};

// ---------------------------------------------------------------------------
// Animation builder helpers
// ---------------------------------------------------------------------------

class AnimatorBuilder {
    TextAnimatorSpec spec_;
    
public:
    AnimatorBuilder& name(std::string n) {
        spec_.name = std::move(n);
        return *this;
    }

    AnimatorBuilder& based_on(std::string unit) {
        spec_.selector.based_on = std::move(unit);
        return *this;
    }

    AnimatorBuilder& shape(std::string s) {
        spec_.selector.shape = std::move(s);
        return *this;
    }

    AnimatorBuilder& range(double start, double end) {
        spec_.selector.type = "range";
        spec_.selector.start = start;
        spec_.selector.end = end;
        return *this;
    }

    AnimatorBuilder& stagger(double delay) {
        spec_.selector.stagger_delay = delay;
        return *this;
    }

    AnimatorBuilder& opacity(double value) {
        spec_.properties.opacity_value = value;
        return *this;
    }
    
    AnimatorBuilder& opacity_keyframes(std::vector<ScalarKeyframeSpec> kfs) {
        spec_.properties.opacity_keyframes = std::move(kfs);
        return *this;
    }

    AnimatorBuilder& position_offset(double x, double y) {
        spec_.properties.position_offset_value = math::Vector2{static_cast<float>(x), static_cast<float>(y)};
        return *this;
    }
    
    AnimatorBuilder& position_keyframes(std::vector<Vector2KeyframeSpec> kfs) {
        spec_.properties.position_offset_keyframes = std::move(kfs);
        return *this;
    }

    AnimatorBuilder& scale(double value) {
        spec_.properties.scale_value = value;
        return *this;
    }
    
    AnimatorBuilder& scale_keyframes(std::vector<ScalarKeyframeSpec> kfs) {
        spec_.properties.scale_keyframes = std::move(kfs);
        return *this;
    }

    AnimatorBuilder& rotation(double value) {
        spec_.properties.rotation_value = value;
        return *this;
    }
    
    AnimatorBuilder& rotation_keyframes(std::vector<ScalarKeyframeSpec> kfs) {
        spec_.properties.rotation_keyframes = std::move(kfs);
        return *this;
    }

    AnimatorBuilder& tracking(double value) {
        spec_.properties.tracking_amount_value = value;
        return *this;
    }
    
    AnimatorBuilder& tracking_keyframes(std::vector<ScalarKeyframeSpec> kfs) {
        spec_.properties.tracking_amount_keyframes = std::move(kfs);
        return *this;
    }

    AnimatorBuilder& fill_color(const ColorSpec& c) {
        spec_.properties.fill_color_value = c;
        return *this;
    }
    
    AnimatorBuilder& fill_color_keyframes(std::vector<ColorKeyframeSpec> kfs) {
        spec_.properties.fill_color_keyframes = std::move(kfs);
        return *this;
    }

    AnimatorBuilder& stroke_color(const ColorSpec& c) {
        spec_.properties.stroke_color_value = c;
        return *this;
    }

    AnimatorBuilder& stroke_width(double w) {
        spec_.properties.stroke_width_value = w;
        return *this;
    }

    AnimatorBuilder& blur(double radius) {
        spec_.properties.blur_radius_value = radius;
        return *this;
    }
    
    AnimatorBuilder& blur_keyframes(std::vector<ScalarKeyframeSpec> kfs) {
        spec_.properties.blur_radius_keyframes = std::move(kfs);
        return *this;
    }

    AnimatorBuilder& reveal(double value) {
        spec_.properties.reveal_value = value;
        return *this;
    }
    
    AnimatorBuilder& reveal_keyframes(std::vector<ScalarKeyframeSpec> kfs) {
        spec_.properties.reveal_keyframes = std::move(kfs);
        return *this;
    }

    [[nodiscard]] TextAnimatorSpec build() const {
        return spec_;
    }
    
    [[nodiscard]] operator TextAnimatorSpec() const {
        return spec_;
    }
};

// ---------------------------------------------------------------------------
// Preset animation factories (return TextAnimatorSpec for use with .animate())
// ---------------------------------------------------------------------------

inline TextAnimatorSpec fade_up(double y_offset = 12.0, double duration = 0.45, double /*stagger*/ = 0.03) {
    return ::tachyon::text::make_minimal_fade_up_animator("characters_excluding_spaces", duration, y_offset);
}

inline TextAnimatorSpec slide_in(double distance = 28.0, double stagger = 0.03, double duration = 0.7) {
    return ::tachyon::text::make_slide_in_animator("characters_excluding_spaces", stagger, distance, duration);
}

inline TextAnimatorSpec blur_to_focus(double blur_radius = 8.0, double duration = 0.45) {
    return ::tachyon::text::make_blur_to_focus_animator("characters_excluding_spaces", duration, blur_radius);
}

inline TextAnimatorSpec typewriter(double cps = 20.0, std::string cursor = "|") {
    return ::tachyon::text::make_typewriter_animator(cps, cursor);
}

inline TextAnimatorSpec kinetic_blur(double distance = 200.0, double duration = 0.5) {
    return ::tachyon::text::make_kinetic_blur_animator(distance, duration);
}

inline TextAnimatorSpec tracking_reveal(double initial_tracking = 40.0, double duration = 0.45) {
    return ::tachyon::text::make_tracking_reveal_animator("characters_excluding_spaces", duration, initial_tracking);
}

inline TextAnimatorSpec soft_scale(double duration = 0.55, double scale = 0.96) {
    return ::tachyon::text::make_soft_scale_in_animator("characters_excluding_spaces", duration, scale);
}

inline TextAnimatorSpec pop_in(double distance = 18.0, double stagger = 0.02, double duration = 0.55) {
    return ::tachyon::text::make_pop_in_animator("characters", stagger, distance, duration);
}

// ---------------------------------------------------------------------------
// Text preset factory functions (Remotion-style)
// ---------------------------------------------------------------------------

inline TextBuilder headline(std::string text) {
    TextBuilder builder(std::move(text));
    return builder;
}

inline TextBuilder paragraph(std::string text) {
    TextBuilder builder(std::move(text));
    return builder;
}

inline TextBuilder caption(std::string text) {
    TextBuilder builder(std::move(text));
    builder.font_size(32); // Smaller for captions
    return builder;
}

inline TextBuilder title(std::string text) {
    TextBuilder builder(std::move(text));
    builder.font_size(96);
    return builder;
}

inline TextBuilder subtitle(std::string text) {
    TextBuilder builder(std::move(text));
    builder.font_size(48);
    return builder;
}

} // namespace tachyon::presets::text
