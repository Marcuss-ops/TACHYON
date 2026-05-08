#include "tachyon/scene/text_builder.h"
#include "tachyon/scene/builder.h"
#include "tachyon/presets/text/text_registry.h"
#include "tachyon/core/registry/parameter_bag.h"

namespace tachyon::scene {

TextBuilder& TextBuilder::content(std::string t) {
    parent_.spec_.type = LayerType::Text;
    parent_.spec_.text_content = std::move(t);
    return *this;
}

TextBuilder& TextBuilder::font(std::string f) {
    parent_.spec_.font_id = std::move(f);
    return *this;
}

TextBuilder& TextBuilder::font_size(double sz) {
    parent_.spec_.font_size = sz;
    return *this;
}

TextBuilder& TextBuilder::box(float w, float h, TextBoxMode mode) {
    parent_.spec_.text_box.width = w;
    parent_.spec_.text_box.height = h;
    parent_.spec_.text_box.mode = mode;
    return *this;
}

TextBuilder& TextBuilder::align(HorizontalAlign h) {
    parent_.spec_.text_box.horizontal_align = h;
    return *this;
}

TextBuilder& TextBuilder::valign(VerticalAlign v) {
    parent_.spec_.text_box.vertical_align = v;
    return *this;
}

TextBuilder& TextBuilder::line_height(float factor) {
    parent_.spec_.text_box.line_height_factor = factor;
    return *this;
}

TextBuilder& TextBuilder::tracking(float amount) {
    parent_.spec_.text_box.tracking_amount = amount;
    return *this;
}

TextBuilder& TextBuilder::fill(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    parent_.spec_.fill_color.value = {r, g, b, a};
    return *this;
}

TextBuilder& TextBuilder::stroke(uint8_t r, uint8_t g, uint8_t b, uint8_t a, float width) {
    parent_.spec_.stroke_color.value = {r, g, b, a};
    parent_.spec_.stroke_width = width;
    return *this;
}

TextBuilder& TextBuilder::subtitle_path(std::string path) {
    parent_.spec_.subtitle_path = std::move(path);
    return *this;
}

TextBuilder& TextBuilder::animator(const TextAnimatorSpec& anim) {
    parent_.spec_.text_animators.push_back(anim);
    return *this;
}

TextBuilder& TextBuilder::animators(std::vector<TextAnimatorSpec> anims) {
    parent_.spec_.text_animators.insert(parent_.spec_.text_animators.end(), anims.begin(), anims.end());
    return *this;
}

TextBuilder& TextBuilder::highlight(const TextHighlightSpec& hl) {
    parent_.spec_.text_highlights.push_back(hl);
    return *this;
}

TextBuilder& TextBuilder::highlights(std::vector<TextHighlightSpec> hls) {
    parent_.spec_.text_highlights.insert(parent_.spec_.text_highlights.end(), hls.begin(), hls.end());
    return *this;
}

TextBuilder& TextBuilder::animation_preset(const std::string& id, const presets::TextRegistry* registry) {
    if (registry) {
        parent_.spec_.text_animators = registry->create_animators(id, registry::ParameterBag{});
    } else {
        presets::TextManifest text_manifest;
        presets::TextRegistry local_registry(text_manifest);
        parent_.spec_.text_animators = local_registry.create_animators(id, registry::ParameterBag{});
    }
    return *this;
}

LayerBuilder& TextBuilder::done() {
    return parent_;
}

} // namespace tachyon::scene
