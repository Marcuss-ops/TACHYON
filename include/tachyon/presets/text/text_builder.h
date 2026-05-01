#pragma once

#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include "tachyon/core/types/colors.h"
#include "tachyon/text/animation/text_presets.h"
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
 *     .animate(text::fade_up().stagger(0.03).duration(0.6));
 * @endcode
 */
class TextBuilder {
    LayerSpec spec_;
public:
    explicit TextBuilder(std::string id, std::string content) {
        spec_.id = std::move(id);
        spec_.name = "Text Layer";
        spec_.type = "text";
        spec_.enabled = true;
        spec_.visible = true;
        spec_.text_content = std::move(content);
        spec_.fill_color = AnimatedColorSpec{ColorSpec{238, 242, 248, 255}};
        spec_.font_size = AnimatedScalarSpec{72.0};
        spec_.width = 1920;
        spec_.height = 200;
        spec_.alignment = "center";
        spec_.transform.position_x = 960.0;
        spec_.transform.position_y = 540.0;
    }

    TextBuilder& font(std::string f) {
        spec_.font_id = std::move(f);
        return *this;
    }

    TextBuilder& size(double s) {
        spec_.font_size = AnimatedScalarSpec{s};
        return *this;
    }

    TextBuilder& color(const ColorSpec& c) {
        spec_.fill_color = AnimatedColorSpec{c};
        return *this;
    }

    TextBuilder& position(double x, double y) {
        spec_.transform.position_x = x;
        spec_.transform.position_y = y;
        return *this;
    }

    TextBuilder& center() {
        spec_.alignment = "center";
        spec_.transform.position_x = spec_.width / 2.0;
        spec_.transform.position_y = spec_.height / 2.0;
        return *this;
    }

    TextBuilder& left() {
        spec_.alignment = "left";
        return *this;
    }

    TextBuilder& right() {
        spec_.alignment = "right";
        return *this;
    }

    TextBuilder& width(int w) {
        spec_.width = w;
        return *this;
    }

    TextBuilder& height(int h) {
        spec_.height = h;
        return *this;
    }

    TextBuilder& start_time(double t) {
        spec_.start_time = t;
        spec_.in_point = t;
        return *this;
    }

    TextBuilder& duration(double d) {
        spec_.out_point = spec_.in_point + d;
        return *this;
    }

    TextBuilder& opacity(double o) {
        spec_.opacity = o;
        return *this;
    }

    TextBuilder& animate(const TextAnimatorSpec& animator) {
        spec_.text_animators.push_back(animator);
        return *this;
    }

    TextBuilder& animate(TextAnimatorSpec&& animator) {
        spec_.text_animators.push_back(std::move(animator));
        return *this;
    }

    TextBuilder& highlight(const TextHighlightSpec& hl) {
        spec_.text_highlights.push_back(hl);
        return *this;
    }

    [[nodiscard]] LayerSpec build() const {
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
