#include "tachyon/scene/text_builder.h"
#include "tachyon/scene/builder.h"
#include "tachyon/presets/text/text_animator_preset_registry.h"
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

TextBuilder& TextBuilder::animation_preset(const std::string& id) {
    parent_.spec_.text_animators = presets::TextAnimatorPresetRegistry::instance().create(id, registry::ParameterBag{});
    return *this;
}

LayerBuilder& TextBuilder::done() {
    return parent_;
}

} // namespace tachyon::scene
