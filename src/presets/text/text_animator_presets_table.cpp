#include "tachyon/presets/text/text_animator_preset_registry.h"
#include "tachyon/text/animation/text_presets.h"

namespace tachyon::presets {

using namespace tachyon::text;

void TextAnimatorPresetRegistry::load_builtins() {
    register_spec({"tachyon.textanim.fade_in",
                     {"tachyon.textanim.fade_in", "Fade In", "Simple fade in animation", "text.anim", {"fade", "basic"}},
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return std::vector<TextAnimatorSpec>{make_fade_in_animator(based_on, stagger_delay, reveal_duration)};
                     }});

    register_spec({"tachyon.textanim.slide_in",
                     {"tachyon.textanim.slide_in", "Slide In", "Slide in from offset", "text.anim", {"slide", "basic"}},
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return std::vector<TextAnimatorSpec>{make_slide_in_animator(based_on, stagger_delay, 28.0, reveal_duration)};
                     }});

    register_spec({"tachyon.textanim.pop_in",
                     {"tachyon.textanim.pop_in", "Pop In", "Pop in with scale and slide", "text.anim", {"pop", "dynamic"}},
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return std::vector<TextAnimatorSpec>{make_pop_in_animator(based_on, stagger_delay, 18.0, reveal_duration)};
                     }});

    register_spec({"tachyon.textanim.typewriter",
                     {"tachyon.textanim.typewriter", "Typewriter", "Character-by-character reveal", "text.anim", {"typewriter", "classic"}},
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         double cps = stagger_delay > 0 ? 1.0 / stagger_delay : 20.0;
                         return std::vector<TextAnimatorSpec>{make_typewriter_minimal_animator(cps, false)};
                     }});

    register_spec({"tachyon.textanim.blur_to_focus",
                     {"tachyon.textanim.blur_to_focus", "Blur to Focus", "Blur radius fade out", "text.anim", {"blur", "cinematic"}},
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return std::vector<TextAnimatorSpec>{make_blur_to_focus_animator(based_on, reveal_duration, 8.0)};
                     }});

    register_spec({"tachyon.textanim.minimal_fade_up",
                     {"tachyon.textanim.minimal_fade_up", "Minimal Fade Up", "Fade up with Y offset", "text.anim", {"fade", "up"}},
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return std::vector<TextAnimatorSpec>{make_minimal_fade_up_animator(based_on, reveal_duration, 12.0)};
                     }});

    register_spec({"tachyon.textanim.tracking_reveal",
                     {"tachyon.textanim.tracking_reveal", "Tracking Reveal", "Letter spacing reveal", "text.anim", {"tracking", "reveal"}},
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return std::vector<TextAnimatorSpec>{make_tracking_reveal_animator(based_on, reveal_duration, 40.0)};
                     }});

    register_spec({"tachyon.textanim.soft_scale_in",
                     {"tachyon.textanim.soft_scale_in", "Soft Scale In", "Scale up from small", "text.anim", {"scale", "soft"}},
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return std::vector<TextAnimatorSpec>{make_soft_scale_in_animator(based_on, reveal_duration, 0.95)};
                     }});

    register_spec({"tachyon.textanim.subtle_y_rotate",
                     {"tachyon.textanim.subtle_y_rotate", "Subtle Y Rotate", "Rotation on Y axis", "text.anim", {"rotate", "y-axis"}},
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return std::vector<TextAnimatorSpec>{make_subtle_y_rotate_animator(based_on, reveal_duration, 5.0)};
                     }});

    register_spec({"tachyon.textanim.fill_wipe",
                     {"tachyon.textanim.fill_wipe", "Fill Wipe", "Color fill wipe", "text.anim", {"color", "wipe"}},
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return std::vector<TextAnimatorSpec>{make_fill_wipe_animator(based_on, reveal_duration)};
                     }});

    register_spec({"tachyon.textanim.outline_to_solid",
                     {"tachyon.textanim.outline_to_solid", "Outline to Solid", "Outline width to zero", "text.anim", {"outline", "solid"}},
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return std::vector<TextAnimatorSpec>{make_outline_to_solid_animator(based_on, reveal_duration, 1.0)};
                     }});

    register_spec({"tachyon.textanim.bounce_in",
                     {"tachyon.textanim.bounce_in", "Bounce In", "Bouncing slide in", "text.anim", {"bounce", "dynamic"}},
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return std::vector<TextAnimatorSpec>{make_bounce_in_animator(based_on, stagger_delay, reveal_duration, 34.0)};
                     }});

    register_spec({"tachyon.textanim.word_punch",
                     {"tachyon.textanim.word_punch", "Word Punch", "Punchy scale for words", "text.anim", {"word", "punch", "dynamic"}},
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return std::vector<TextAnimatorSpec>{make_word_punch_animator(stagger_delay, reveal_duration, 1.12)};
                     }});

    register_spec({"tachyon.textanim.word_by_word",
                     {"tachyon.textanim.word_by_word", "Word by Word", "Reveal words one by one", "text.anim", {"word", "basic"}},
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return std::vector<TextAnimatorSpec>{make_word_by_word_opacity_animator(stagger_delay, reveal_duration)};
                     }});

    register_spec({"tachyon.textanim.split_line",
                     {"tachyon.textanim.split_line", "Split Line", "Reveal lines one by one", "text.anim", {"line", "stagger"}},
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return std::vector<TextAnimatorSpec>{make_split_line_stagger_animator(stagger_delay, reveal_duration)};
                     }});

    register_spec({"tachyon.textanim.morphing",
                     {"tachyon.textanim.morphing", "Morphing", "Morphing words effect", "text.anim", {"morphing", "experimental"}},
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return std::vector<TextAnimatorSpec>{make_morphing_words_animator(reveal_duration)};
                     }});

    register_spec({"tachyon.textanim.underline",
                     {"tachyon.textanim.underline", "Underline", "Drawing underline", "text.anim", {"underline", "draw"}},
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return std::vector<TextAnimatorSpec>{make_underline_draw_animator("words", reveal_duration, 2.0)};
                     }});

    register_spec({"tachyon.textanim.number_flip",
                     {"tachyon.textanim.number_flip", "Number Flip", "Flipping numbers effect", "text.anim", {"number", "flip"}},
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return std::vector<TextAnimatorSpec>{make_number_flip_minimal_animator("characters", reveal_duration)};
                     }});

    // Composite Presets
    register_spec({"tachyon.textanim.fade_up",
                     {"tachyon.textanim.fade_up", "Fade Up (Composite)", "Fade up with blur and scale", "text.anim", {"composite", "fade", "up"}},
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return std::vector<TextAnimatorSpec>{
                             make_minimal_fade_up_animator(based_on, reveal_duration, 12.0),
                             make_blur_to_focus_animator(based_on, reveal_duration, 8.0),
                             make_soft_scale_in_animator(based_on, reveal_duration, 0.95)
                         };
                     }});

    register_spec({"tachyon.textanim.kinetic",
                     {"tachyon.textanim.kinetic", "Kinetic", "Motion blur with tracking", "text.anim", {"kinetic", "blur"}},
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return std::vector<TextAnimatorSpec>{
                             make_kinetic_blur_animator(200.0, reveal_duration),
                             make_tracking_reveal_animator(based_on, reveal_duration, 40.0)
                         };
                     }});

    register_spec({"tachyon.textanim.brushed_metal",
                     {"tachyon.textanim.brushed_metal", "Brushed Metal Title", "Premium metallic effect", "text.anim", {"metal", "premium"}},
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return std::vector<TextAnimatorSpec>{
                             make_tracking_reveal_animator(based_on, reveal_duration, 40.0),
                             make_blur_to_focus_animator(based_on, reveal_duration, 12.0),
                             make_minimal_fade_up_animator(based_on, reveal_duration, 20.0)
                         };
                     }});
}

} // namespace tachyon::presets
