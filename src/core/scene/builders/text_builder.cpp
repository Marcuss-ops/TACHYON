#include "tachyon/scene/text_builder.h"
#include "tachyon/scene/builder.h"
#include "tachyon/text/animation/text_animator_utils.h"

namespace tachyon::scene {

namespace {

void mark_fixed_pitch_if_needed(tachyon::LayerSpec& spec, const TextAnimatorSpec& anim) {
    if (::tachyon::text::uses_character_stagger_layout(anim)) {
        spec.text.box.fixed_pitch = true;
    }
}

} // namespace

TextBuilder& TextBuilder::content(std::string t) {
    parent_.spec_.type = LayerType::Text;
    parent_.spec_.text.content = std::move(t);
    return *this;
}

TextBuilder& TextBuilder::font(std::string f) {
    parent_.spec_.text.font_id = std::move(f);
    return *this;
}

TextBuilder& TextBuilder::font(std::string f, double sz) {
    parent_.spec_.text.font_id = std::move(f);
    parent_.spec_.text.font_size = sz;
    return *this;
}

TextBuilder& TextBuilder::font_size(double sz) {
    parent_.spec_.text.font_size = sz;
    return *this;
}

TextBuilder& TextBuilder::box(float w, float h, TextBoxMode mode) {
    parent_.spec_.text.box.width = w;
    parent_.spec_.text.box.height = h;
    parent_.spec_.text.box.mode = mode;
    return *this;
}

TextBuilder& TextBuilder::align(HorizontalAlign h) {
    parent_.spec_.text.box.horizontal_align = h;
    return *this;
}

TextBuilder& TextBuilder::valign(VerticalAlign v) {
    parent_.spec_.text.box.vertical_align = v;
    return *this;
}

TextBuilder& TextBuilder::line_height(float factor) {
    parent_.spec_.text.box.line_height_factor = factor;
    return *this;
}

TextBuilder& TextBuilder::tracking(float amount) {
    parent_.spec_.text.box.tracking_amount = amount;
    return *this;
}

TextBuilder& TextBuilder::centerText() {
    parent_.spec_.text.box.mode = TextBoxMode::Fixed;
    parent_.spec_.text.box.width = static_cast<float>(parent_.spec_.width);
    parent_.spec_.text.box.height = static_cast<float>(parent_.spec_.height);
    parent_.spec_.text.box.horizontal_align = HorizontalAlign::Center;
    parent_.spec_.text.box.vertical_align = VerticalAlign::Middle;
    parent_.spec_.transform.position_x = static_cast<double>(parent_.spec_.width) * 0.5;
    parent_.spec_.transform.position_y = static_cast<double>(parent_.spec_.height) * 0.5;
    return *this;
}

TextBuilder& TextBuilder::fixed_pitch(bool enabled) {
    parent_.spec_.text.box.fixed_pitch = enabled;
    return *this;
}

TextBuilder& TextBuilder::fill(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    parent_.spec_.text.fill_color.value = {r, g, b, a};
    return *this;
}

TextBuilder& TextBuilder::color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return fill(r, g, b, a);
}

TextBuilder& TextBuilder::color(const ColorSpec& c) {
    parent_.spec_.text.fill_color.value = c;
    return *this;
}

TextBuilder& TextBuilder::stroke(uint8_t r, uint8_t g, uint8_t b, uint8_t a, float width) {
    parent_.spec_.text.stroke_color.value = {r, g, b, a};
    parent_.spec_.text.stroke_width = width;
    return *this;
}

TextBuilder& TextBuilder::subtitle_path(std::string path) {
    parent_.spec_.subtitles.path = std::move(path);
    return *this;
}

TextBuilder& TextBuilder::animator(const TextAnimatorSpec& anim) {
    parent_.spec_.text_animators.push_back(anim);
    mark_fixed_pitch_if_needed(parent_.spec_, anim);
    return *this;
}

TextBuilder& TextBuilder::animators(std::vector<TextAnimatorSpec> anims) {
    for (const auto& anim : anims) {
        mark_fixed_pitch_if_needed(parent_.spec_, anim);
    }
    parent_.spec_.text_animators.insert(parent_.spec_.text_animators.end(), anims.begin(), anims.end());
    return *this;
}

TextBuilder& TextBuilder::animate(const TextAnimatorSpec& anim) {
    return animator(anim);
}

TextBuilder& TextBuilder::animate(std::vector<TextAnimatorSpec> anims) {
    return animators(std::move(anims));
}

TextBuilder& TextBuilder::highlight(const TextHighlightSpec& hl) {
    parent_.spec_.text_highlights.push_back(hl);
    return *this;
}

TextBuilder& TextBuilder::highlights(std::vector<TextHighlightSpec> hls) {
    parent_.spec_.text_highlights.insert(parent_.spec_.text_highlights.end(), hls.begin(), hls.end());
    return *this;
}

LayerBuilder& TextBuilder::done() {
    return parent_;
}

} // namespace tachyon::scene
