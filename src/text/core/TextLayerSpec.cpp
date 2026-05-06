#include "tachyon/text/core/TextLayerSpec.h"
#include "tachyon/presets/builders_common.h"
#include "tachyon/presets/text/text_animator_preset_registry.h"
#include "tachyon/text/animation/text_presets.h"

#include <utility>

namespace tachyon {

namespace {

std::vector<TextAnimatorSpec> resolve_text_animators(const presets::TextParams& p) {
    if (!p.animations.empty()) {
        return p.animations;
    }

    const std::string animation_id = p.animation.empty() ? "tachyon.textanim.fade_up" : p.animation;

    registry::ParameterBag bag;
    bag.set("based_on", "characters_excluding_spaces");
    bag.set("stagger_delay", static_cast<double>(p.stagger_delay));
    bag.set("reveal_duration", static_cast<double>(p.reveal_duration));

    return presets::TextAnimatorPresetRegistry::instance().create(animation_id, bag);
}

} // namespace

TextLayerSpec build_text_spec(const presets::TextParams& p) {
    TextLayerSpec spec;
    spec.in_point = p.in_point;
    spec.out_point = p.out_point;
    spec.x = p.x;
    spec.y = p.y;
    spec.width = p.text_w;
    spec.height = p.text_h;
    spec.opacity = p.opacity;

    spec.alignment = p.alignment;
    spec.text_content = p.text;
    spec.font_id = p.font_id;
    spec.font_size = static_cast<double>(p.font_size);
    spec.fill_color = AnimatedColorSpec(p.color);

    spec.text_animators = resolve_text_animators(p);

    return spec;
}

LayerSpec make_layer_from_text_spec(const TextLayerSpec& spec) {
    presets::LayerParams p;
    p.in_point = spec.in_point;
    p.out_point = spec.out_point;
    p.x = static_cast<float>(spec.x);
    p.y = static_cast<float>(spec.y);
    p.w = static_cast<float>(spec.width);
    p.h = static_cast<float>(spec.height);
    p.opacity = static_cast<float>(spec.opacity);

    LayerSpec l = presets::make_base_layer(spec.id, spec.name, "text", p);

    l.alignment = spec.alignment;
    l.text_content = spec.text_content;
    l.font_id = spec.font_id;
    l.font_size.value = spec.font_size;
    l.fill_color = spec.fill_color;
    l.stroke_color = spec.stroke_color;
    l.stroke_width = spec.stroke_width;
    l.text_animators = spec.text_animators;
    l.text_highlights = spec.text_highlights;
    l.subtitle_path = spec.subtitle_path;
    l.subtitle_outline_color = spec.subtitle_outline_color;
    l.subtitle_outline_width = spec.subtitle_outline_width;
    l.word_timestamp_path = spec.word_timestamp_path;

    return l;
}

} // namespace tachyon
