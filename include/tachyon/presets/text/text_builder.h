#pragma once

#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include "tachyon/core/types/colors.h"
#include "tachyon/text/animation/text_presets.h"
#include "tachyon/text/animation/text_animator_utils.h"
#include "tachyon/presets/text/text_anchor.h"
#include <string>
#include <vector>
#include <optional>

namespace tachyon::presets::text {

/**
 * @brief Fluent builder for text layers.
 * 
 * Provides a clean API for building text layers:
 * 
 * @code
 * auto title = text::headline("Breaking News")
 *     .font("Inter")
 *     .size(96)
 *     .color({255, 255, 255, 255})
 *     .center()
 *     .animate(text::kind_fade_up().stagger(0.03).duration(0.6));
 * @endcode
 */
class TextBuilder {
    LayerSpec spec_;
    std::optional<TextAnchor> anchor_;

    void apply_anchor() {
        if (anchor_.has_value()) {
            apply_text_anchor(spec_, *anchor_);
        }
    }
public:
    explicit TextBuilder(std::string id, std::string content) {
        spec_.identity.id = std::move(id);
        spec_.identity.name = "Text Layer";
        spec_.identity.type = ::tachyon::LayerType::Text;
        spec_.identity.enabled = true;
        spec_.identity.visible = true;
        spec_.text.content = std::move(content);
        spec_.text.fill_color = AnimatedColorSpec{ColorSpec{238, 242, 248, 255}};
        spec_.text.font_size = AnimatedScalarSpec{72.0};
        spec_.transform.width = 1920;
        spec_.transform.height = 200;
        spec_.text.box.horizontal_align = HorizontalAlign::Center;
        spec_.transform.transform.position_x = 960.0;
        spec_.transform.transform.position_y = 540.0;
    }

    TextBuilder& font(std::string f) {
        spec_.text.font_id = std::move(f);
        return *this;
    }

    TextBuilder& size(double s) {
        spec_.text.font_size = AnimatedScalarSpec{s};
        apply_anchor();
        return *this;
    }

    TextBuilder& color(const ColorSpec& c) {
        spec_.text.fill_color = AnimatedColorSpec{c};
        return *this;
    }

    TextBuilder& position(double x, double y) {
        spec_.transform.transform.position_x = x;
        spec_.transform.transform.position_y = y;
        anchor_.reset();
        return *this;
    }

    TextBuilder& center() {
        anchor_ = TextAnchor::MiddleCenter;
        apply_anchor();
        return *this;
    }

    TextBuilder& left() {
        spec_.text.box.horizontal_align = HorizontalAlign::Left;
        anchor_.reset();
        return *this;
    }

    TextBuilder& right() {
        spec_.text.box.horizontal_align = HorizontalAlign::Right;
        anchor_.reset();
        return *this;
    }

    TextBuilder& anchor(TextAnchor anchor) {
        anchor_ = anchor;
        apply_anchor();
        return *this;
    }

    TextBuilder& width(int w) {
        spec_.transform.width = w;
        apply_anchor();
        return *this;
    }

    TextBuilder& height(int h) {
        spec_.transform.height = h;
        apply_anchor();
        return *this;
    }

    TextBuilder& fixed_pitch(bool enabled = true) {
        spec_.text.box.fixed_pitch = enabled;
        return *this;
    }

    TextBuilder& start_time(double t) {
        spec_.playback.timing.start = t;
        spec_.playback.timing.source_in = t;
        return *this;
    }

    TextBuilder& duration(double d) {
        spec_.playback.timing.duration = d;
        spec_.playback.timing.source_out = spec_.playback.timing.source_in + d;
        return *this;
    }

    TextBuilder& opacity(double o) {
        spec_.transform.opacity = o;
        return *this;
    }

    TextBuilder& animate(const TextAnimatorSpec& animator) {
        spec_.text_animators.push_back(animator);
        if (::tachyon::text::uses_character_stagger_layout(animator)) {
            spec_.text.box.fixed_pitch = true;
        }
        return *this;
    }

    TextBuilder& animate(TextAnimatorSpec&& animator) {
        if (::tachyon::text::uses_character_stagger_layout(animator)) {
            spec_.text.box.fixed_pitch = true;
        }
        spec_.text_animators.push_back(std::move(animator));
        return *this;
    }

    TextBuilder& highlight(const TextHighlightSpec& hl) {
        spec_.text_highlights.push_back(hl);
        return *this;
    }

    [[nodiscard]] LayerSpec build() const & {
        return spec_;
    }

    [[nodiscard]] LayerSpec build() && {
        return std::move(spec_);
    }
};

/**
 * @brief Create a headline text layer.
 */
inline TextBuilder headline(std::string text) {
    return TextBuilder("headline", std::move(text));
}

/**
 * @brief Create a paragraph text layer.
 */
inline TextBuilder paragraph(std::string text) {
    TextBuilder builder("paragraph", std::move(text));
    builder.size(24);
    builder.width(1200);
    return builder;
}

/**
 * @brief Create a caption text layer.
 */
inline TextBuilder caption(std::string text) {
    TextBuilder builder("caption", std::move(text));
    builder.size(32);
    builder.width(1600);
    return builder;
}

/**
 * @brief Create a typewriter text layer preset.
 */
inline TextBuilder typewriter(std::string text) {
    TextBuilder builder("typewriter", std::move(text));
    builder.fixed_pitch();
    builder.animate(::tachyon::text::make_typewriter_animator(20.0, "|"));
    return builder;
}

/**
 * @brief Create a kinetic text layer preset.
 */
inline TextBuilder kinetic(std::string text) {
    TextBuilder builder("kinetic", std::move(text));
    builder.animate(::tachyon::text::make_kinetic_blur_animator(200.0, 0.5));
    builder.animate(::tachyon::text::make_tracking_reveal_animator("characters_excluding_spaces", 0.45, 40.0));
    return builder;
}

} // namespace tachyon::presets::text
