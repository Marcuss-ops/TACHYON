#include "tachyon/presets/text/text_builders.h"
#include "tachyon/presets/background/procedural.h"
#include "tachyon/presets/transition/transition_builders.h"
#include "tachyon/text/animation/text_presets.h"
#include "tachyon/core/animation/easing.h"

namespace tachyon::presets {

namespace {

LayerSpec make_text_layer_base(const TextParams& p) {
    LayerSpec l;
    l.id          = "text_layer";
    l.name        = "Text";
    l.type        = "text";
    l.enabled     = true;
    l.visible     = true;
    l.start_time  = p.in_point;
    l.in_point    = p.in_point;
    l.out_point   = p.out_point;
    l.width       = static_cast<int>(p.text_w);
    l.height      = static_cast<int>(p.text_h);
    l.alignment   = p.alignment;
    l.text_content = p.text;
    l.font_id     = p.font_id;
    l.font_size.value = static_cast<double>(p.font_size);
    l.fill_color  = AnimatedColorSpec(p.color);
    l.opacity     = static_cast<double>(p.opacity);
    l.transform.position_x = static_cast<double>(p.x);
    l.transform.position_y = static_cast<double>(p.y);

    if (!p.enter_preset.empty()) {
        TransitionParams tp;
        tp.id = p.enter_preset;
        tp.duration = p.enter_duration;
        l.transition_in = build_transition_enter(tp);
    }
    if (!p.exit_preset.empty()) {
        TransitionParams tp;
        tp.id = p.exit_preset;
        tp.duration = p.exit_duration;
        l.transition_out = build_transition_exit(tp);
    }

    return l;
}

LayerSpec make_bg_layer(const SceneParams& s) {
    if (s.bg_procedural) {
        using namespace background::procedural_bg;
        if (s.bg_kind == "aura")    return aura(s.width, s.height, palettes::neon_grid(), s.duration);
        if (s.bg_kind == "grid")    return modern_tech_grid(s.width, s.height, palettes::neon_grid(), 60.0, s.duration);
        if (s.bg_kind == "stars")   return classico_premium(s.width, s.height, palettes::premium_dark(), s.duration);
        if (s.bg_kind == "stripes") return classico_premium(s.width, s.height, palettes::premium_dark(), s.duration); // Default since stripes missing
        return aura(s.width, s.height, palettes::neon_grid(), s.duration);
    }
    LayerSpec solid;
    solid.id         = "bg_solid";
    solid.name       = "Background";
    solid.type       = "solid";
    solid.enabled    = true;
    solid.visible    = true;
    solid.in_point   = 0.0;
    solid.out_point  = s.duration;
    solid.width      = s.width;
    solid.height     = s.height;
    solid.opacity    = 1.0;
    solid.fill_color = AnimatedColorSpec(ColorSpec{13, 17, 23, 255});
    return solid;
}

} // namespace

// ---------------------------------------------------------------------------
// Named variants
// ---------------------------------------------------------------------------

LayerSpec build_text_fade_up(const TextParams& p) {
    auto l = make_text_layer_base(p);
    l.text_animators = {
        ::tachyon::text::make_minimal_fade_up_animator("characters_excluding_spaces", p.reveal_duration, 12.0),
        ::tachyon::text::make_blur_to_focus_animator("characters_excluding_spaces", p.reveal_duration, 8.0),
        ::tachyon::text::make_soft_scale_in_animator("characters_excluding_spaces", p.reveal_duration, 0.95),
    };
    return l;
}

LayerSpec build_text_blur_to_focus(const TextParams& p) {
    auto l = make_text_layer_base(p);
    l.text_animators = {
        ::tachyon::text::make_blur_to_focus_animator("characters_excluding_spaces", p.reveal_duration, 8.0),
    };
    return l;
}

LayerSpec build_text_typewriter(const TextParams& p) {
    auto l = make_text_layer_base(p);
    l.text_animators = {
        ::tachyon::text::make_typewriter_minimal_animator(20.0, false),
    };
    return l;
}

LayerSpec build_text_slide_in(const TextParams& p) {
    auto l = make_text_layer_base(p);
    l.text_animators = {
        ::tachyon::text::make_slide_in_animator("characters_excluding_spaces", p.stagger_delay, 28.0, p.reveal_duration),
    };
    return l;
}

LayerSpec build_text_tracking_reveal(const TextParams& p) {
    auto l = make_text_layer_base(p);
    l.text_animators = {
        ::tachyon::text::make_tracking_reveal_animator("characters_excluding_spaces", p.reveal_duration, 40.0),
    };
    return l;
}

LayerSpec build_text_outline_to_solid(const TextParams& p) {
    auto l = make_text_layer_base(p);
    l.text_animators = {
        ::tachyon::text::make_outline_to_solid_animator("characters_excluding_spaces", p.reveal_duration, 1.0),
    };
    return l;
}

LayerSpec build_text_number_flip(const TextParams& p) {
    auto l = make_text_layer_base(p);
    l.text_animators = {
        ::tachyon::text::make_number_flip_minimal_animator("characters", p.reveal_duration),
    };
    return l;
}

LayerSpec build_text_word_by_word(const TextParams& p) {
    auto l = make_text_layer_base(p);
    l.text_animators = {
        ::tachyon::text::make_word_by_word_opacity_animator(p.stagger_delay, p.reveal_duration),
    };
    return l;
}

LayerSpec build_text_split_line(const TextParams& p) {
    auto l = make_text_layer_base(p);
    l.text_animators = {
        ::tachyon::text::make_split_line_stagger_animator(p.stagger_delay, p.reveal_duration),
    };
    return l;
}

LayerSpec build_text_soft_scale(const TextParams& p) {
    auto l = make_text_layer_base(p);
    l.text_animators = {
        ::tachyon::text::make_soft_scale_in_animator("characters_excluding_spaces", p.reveal_duration, 0.95),
    };
    return l;
}

LayerSpec build_text_curtain_box(const TextParams& p) {
    auto l = make_text_layer_base(p);
    l.text_animators = {
        ::tachyon::text::make_curtain_box_minimal_animator("characters_excluding_spaces", p.reveal_duration),
    };
    return l;
}

LayerSpec build_text_fill_wipe(const TextParams& p) {
    auto l = make_text_layer_base(p);
    l.text_animators = {
        ::tachyon::text::make_fill_wipe_animator("characters_excluding_spaces", p.reveal_duration),
    };
    return l;
}

LayerSpec build_text_slide_mask(const TextParams& p) {
    auto l = make_text_layer_base(p);
    l.text_animators = {
        ::tachyon::text::make_slide_mask_left_animator("characters_excluding_spaces", p.reveal_duration),
    };
    return l;
}

LayerSpec build_text_kinetic(const TextParams& p) {
    auto l = make_text_layer_base(p);
    l.text_animators = {
        ::tachyon::text::make_kinetic_blur_animator(200.0, p.reveal_duration),
        ::tachyon::text::make_tracking_reveal_animator("characters_excluding_spaces", p.reveal_duration, 40.0),
    };
    return l;
}

LayerSpec build_text_pop(const TextParams& p) {
    auto l = make_text_layer_base(p);
    l.text_animators = {
        ::tachyon::text::make_pop_in_animator("characters", p.stagger_delay, 18.0, p.reveal_duration),
    };
    return l;
}

LayerSpec build_text_y_rotate(const TextParams& p) {
    auto l = make_text_layer_base(p);
    l.text_animators = {
        ::tachyon::text::make_subtle_y_rotate_animator("characters_excluding_spaces", p.reveal_duration, 5.0),
    };
    return l;
}

LayerSpec build_text_morphing(const TextParams& p) {
    auto l = make_text_layer_base(p);
    l.text_animators = {
        ::tachyon::text::make_morphing_words_animator(p.reveal_duration),
    };
    return l;
}

LayerSpec build_text_underline(const TextParams& p) {
    auto l = make_text_layer_base(p);
    l.text_animators = {
        ::tachyon::text::make_underline_draw_animator("words", p.reveal_duration, 2.0),
    };
    return l;
}

LayerSpec build_text_brushed_metal_title(const TextParams& p) {
    auto l = make_text_layer_base(p);
    // Metallic look: Silver/Steel colors
    l.fill_color = AnimatedColorSpec(ColorSpec{192, 192, 200, 255});
    l.text_animators = {
        ::tachyon::text::make_tracking_reveal_animator("characters_excluding_spaces", p.reveal_duration, 40.0),
        ::tachyon::text::make_blur_to_focus_animator("characters_excluding_spaces", p.reveal_duration, 12.0),
        ::tachyon::text::make_minimal_fade_up_animator("characters_excluding_spaces", p.reveal_duration, 20.0),
    };
    return l;
}

// ---------------------------------------------------------------------------
// Dispatcher
// ---------------------------------------------------------------------------

LayerSpec build_text(const TextParams& p) {
    // If typed animators are provided via fluent API, use them directly
    if (!p.animations.empty()) {
        LayerSpec l = make_text_layer_base(p);
        l.text_animators = p.animations;
        return l;
    }
    
    if (p.animation == "blur_to_focus")    return build_text_blur_to_focus(p);
    if (p.animation == "typewriter")       return build_text_typewriter(p);
    if (p.animation == "slide_in")         return build_text_slide_in(p);
    if (p.animation == "tracking_reveal")  return build_text_tracking_reveal(p);
    if (p.animation == "outline_to_solid") return build_text_outline_to_solid(p);
    if (p.animation == "number_flip")      return build_text_number_flip(p);
    if (p.animation == "word_by_word")     return build_text_word_by_word(p);
    if (p.animation == "split_line")       return build_text_split_line(p);
    if (p.animation == "soft_scale")       return build_text_soft_scale(p);
    if (p.animation == "curtain_box")      return build_text_curtain_box(p);
    if (p.animation == "fill_wipe")        return build_text_fill_wipe(p);
    if (p.animation == "slide_mask")       return build_text_slide_mask(p);
    if (p.animation == "kinetic")          return build_text_kinetic(p);
    if (p.animation == "pop")              return build_text_pop(p);
    if (p.animation == "y_rotate")         return build_text_y_rotate(p);
    if (p.animation == "morphing")         return build_text_morphing(p);
    if (p.animation == "underline")        return build_text_underline(p);
    if (p.animation == "brushedMetal")    return build_text_brushed_metal_title(p);
    return build_text_fade_up(p);
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

    comp.layers.push_back(make_bg_layer(scene));
    comp.layers.push_back(build_text(text));

    spec.compositions.push_back(std::move(comp));
    return spec;
}

LayerSpec build_text_layer(const std::string& text,
                           const std::string& font_id,
                           double font_size,
                           const std::string& animation_style,
                           double duration) {
    LayerSpec l;
    l.id = "text_layer";
    l.name = "Text";
    l.type = "text";
    l.enabled = true;
    l.visible = true;
    l.start_time = 0.0;
    l.in_point = 0.0;
    l.out_point = duration;
    l.alignment = "left";
    l.text_content = text;
    l.font_id = font_id;
    l.font_size.value = font_size;
    l.fill_color = AnimatedColorSpec(ColorSpec{255, 255, 255, 255});
    l.opacity = 1.0;
    l.transform.position_x = 0.0;
    l.transform.position_y = 0.0;

    TextAnimatorSpec animator;
    animator.name = animation_style;
    l.text_animators.push_back(std::move(animator));

    return l;
}

} // namespace tachyon::presets
