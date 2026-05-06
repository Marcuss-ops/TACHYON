#include "tachyon/presets/text/text_builders.h"
#include "tachyon/presets/builders_common.h"
#include "tachyon/presets/text/text_animator_preset_registry.h"
#include "tachyon/text/animation/text_presets.h"

namespace tachyon::presets {

namespace {

LayerSpec build_text_with_animation(const TextParams& p, std::string animation_id) {
    TextParams copy = p;
    copy.animation = std::move(animation_id);
    return build_text(copy);
}

std::vector<TextAnimatorSpec> resolve_text_animators(const TextParams& p) {
    if (!p.animations.empty()) {
        return p.animations;
    }

    const std::string animation_id = p.animation.empty() ? "tachyon.textanim.fade_up" : p.animation;

    registry::ParameterBag bag;
    bag.set("based_on", "characters_excluding_spaces");
    bag.set("stagger_delay", static_cast<double>(p.stagger_delay));
    bag.set("reveal_duration", static_cast<double>(p.reveal_duration));

    return tachyon::presets::TextAnimatorPresetRegistry::instance().create(animation_id, bag);
}

LayerSpec build_text_base(const TextParams& p) {
    LayerSpec l = make_base_layer("text_layer", "Text", "text", {
        p.in_point, p.out_point, p.x, p.y, p.text_w, p.text_h, p.opacity
    });
    
    l.alignment   = p.alignment;
    l.text_content = p.text;
    l.font_id     = p.font_id;
    l.font_size.value = static_cast<double>(p.font_size);
    l.fill_color  = AnimatedColorSpec(p.color);

    apply_layer_transitions(l, p.enter_preset, p.enter_duration, p.exit_preset, p.exit_duration);

    return l;
}

} // namespace

// ---------------------------------------------------------------------------
// Dispatcher
// ---------------------------------------------------------------------------

LayerSpec build_text(const TextParams& p) {
    LayerSpec l = build_text_base(p);
    l.text_animators = resolve_text_animators(p);
    return l;
}

LayerSpec build_text_fade_up(const TextParams& p) {
    return build_text_with_animation(p, "tachyon.textanim.fade_up");
}

LayerSpec build_text_blur_to_focus(const TextParams& p) {
    return build_text_with_animation(p, "tachyon.textanim.blur_to_focus");
}

LayerSpec build_text_typewriter(const TextParams& p) {
    return build_text_with_animation(p, "tachyon.textanim.typewriter.classic");
}

LayerSpec build_text_typewriter_cursor(const TextParams& p) {
    return build_text_with_animation(p, "tachyon.textanim.typewriter.cursor");
}

LayerSpec build_text_typewriter_soft(const TextParams& p) {
    return build_text_with_animation(p, "tachyon.textanim.typewriter.soft");
}

LayerSpec build_text_slide_in(const TextParams& p) {
    return build_text_with_animation(p, "tachyon.textanim.slide_in");
}

LayerSpec build_text_tracking_reveal(const TextParams& p) {
    return build_text_with_animation(p, "tachyon.textanim.tracking_reveal");
}

LayerSpec build_text_outline_to_solid(const TextParams& p) {
    return build_text_with_animation(p, "tachyon.textanim.outline_to_solid");
}

LayerSpec build_text_number_flip(const TextParams& p) {
    return build_text_with_animation(p, "tachyon.textanim.number_flip");
}

LayerSpec build_text_word_by_word(const TextParams& p) {
    return build_text_with_animation(p, "tachyon.textanim.typewriter.word");
}

LayerSpec build_text_word_cursor(const TextParams& p) {
    return build_text_with_animation(p, "tachyon.textanim.typewriter.word_cursor");
}

LayerSpec build_text_split_line(const TextParams& p) {
    return build_text_with_animation(p, "tachyon.textanim.typewriter.line");
}

LayerSpec build_text_sentence(const TextParams& p) {
    return build_text_with_animation(p, "tachyon.textanim.typewriter.sentence");
}

LayerSpec build_text_soft_scale(const TextParams& p) {
    return build_text_with_animation(p, "tachyon.textanim.soft_scale_in");
}

LayerSpec build_text_curtain_box(const TextParams& p) {
    return build_text_with_animation(p, "tachyon.textanim.curtain_box");
}

LayerSpec build_text_fill_wipe(const TextParams& p) {
    return build_text_with_animation(p, "tachyon.textanim.fill_wipe");
}

LayerSpec build_text_slide_mask(const TextParams& p) {
    return build_text_with_animation(p, "tachyon.textanim.slide_mask");
}

LayerSpec build_text_kinetic(const TextParams& p) {
    return build_text_with_animation(p, "tachyon.textanim.kinetic");
}

LayerSpec build_text_pop(const TextParams& p) {
    return build_text_with_animation(p, "tachyon.textanim.pop_in");
}

LayerSpec build_text_y_rotate(const TextParams& p) {
    return build_text_with_animation(p, "tachyon.textanim.subtle_y_rotate");
}

LayerSpec build_text_morphing(const TextParams& p) {
    return build_text_with_animation(p, "tachyon.textanim.morphing");
}

LayerSpec build_text_underline(const TextParams& p) {
    return build_text_with_animation(p, "tachyon.textanim.underline");
}

LayerSpec build_text_brushed_metal_title(const TextParams& p) {
    return build_text_with_animation(p, "tachyon.textanim.brushed_metal");
}

// ---------------------------------------------------------------------------
// Scene builder
// ---------------------------------------------------------------------------

SceneSpec build_text_scene(const TextParams& text, const SceneParams& scene) {
    SceneSpec spec;
    spec.project.id   = "tachyon_text_scene";
    spec.project.name = "Text Scene";

    CompositionSpec comp;
    comp.id         = "main";
    comp.name       = "Main";
    comp.width      = scene.width;
    comp.height     = scene.height;
    comp.duration   = scene.duration;
    comp.frame_rate = {scene.fps, 1};
    comp.background = BackgroundSpec::from_string(scene.bg_color);

    // Solid background layer
    LayerSpec bg_layer;
    bg_layer.id = "bg_solid";
    bg_layer.name = "Background";
    bg_layer.type = LayerType::Solid;
    bg_layer.type_string = "solid";
    bg_layer.enabled = true;
    bg_layer.visible = true;
    bg_layer.in_point = 0.0;
    bg_layer.out_point = scene.duration;
    bg_layer.width = scene.width;
    bg_layer.height = scene.height;
    bg_layer.opacity = 1.0;
    bg_layer.fill_color = AnimatedColorSpec(ColorSpec{13, 17, 23, 255});
    
    comp.layers.push_back(std::move(bg_layer));
    comp.layers.push_back(build_text(text));

    spec.compositions.push_back(std::move(comp));
    return spec;
}

LayerSpec build_text_layer(const std::string& text,
                           const std::string& font_id,
                           double font_size,
                           const std::string& animation_style,
                           double duration) {
    TextParams p;
    p.text = text;
    p.font_id = font_id;
    p.font_size = static_cast<uint32_t>(font_size);
    p.animation = animation_style;
    p.in_point = 0.0;
    p.out_point = duration;
    p.reveal_duration = 0.5;
    
    return build_text(p);
}

} // namespace tachyon::presets
