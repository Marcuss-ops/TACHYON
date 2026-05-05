#include "tachyon/presets/text/text_builders.h"
#include "tachyon/presets/builders_common.h"
#include "tachyon/presets/text/text_animator_preset_registry.h"
#include "tachyon/text/animation/text_presets.h"

namespace tachyon::presets {

namespace {

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

    // If typed animators are provided via fluent API, use them directly
    if (!p.animations.empty()) {
        l.text_animators = p.animations;
        return l;
    }
    
    // Otherwise, use the registry for the named animation
    std::string anim_id = p.animation.empty() ? "tachyon.textanim.fade_up" : p.animation;
    
    registry::ParameterBag bag;
    bag.set("based_on", "characters_excluding_spaces");
    bag.set("stagger_delay", static_cast<double>(p.stagger_delay));
    bag.set("reveal_duration", static_cast<double>(p.reveal_duration));

    l.text_animators = tachyon::presets::TextAnimatorPresetRegistry::instance().create(anim_id, bag);

    return l;
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
    bg_layer.type = "solid";
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
