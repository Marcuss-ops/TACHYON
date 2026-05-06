#include "tachyon/presets/text/text_animator_preset_registry.h"
#include "tachyon/text/animation/text_presets.h"

namespace tachyon::presets {

using namespace tachyon::text;

void TextAnimatorPresetRegistry::load_builtins() {
    using namespace registry;

    auto get_text_params = [](const ParameterBag& p) {
        struct {
            std::string based_on;
            double stagger;
            double reveal;
        } res;
        res.based_on = p.get_or<std::string>("based_on", "characters_excluding_spaces");
        res.stagger  = p.get_or<double>("stagger_delay", 0.03);
        res.reveal   = p.get_or<double>("reveal_duration", 0.5);
        return res;
    };

    register_spec({"tachyon.textanim.fade_in",
                     {"tachyon.textanim.fade_in", "Fade In", "Simple fade in animation", "text.anim", {"fade", "basic"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         return std::vector<TextAnimatorSpec>{make_fade_in_animator(params.based_on, params.stagger, params.reveal)};
                     }});

    register_spec({"tachyon.textanim.slide_in",
                     {"tachyon.textanim.slide_in", "Slide In", "Slide in from offset", "text.anim", {"slide", "basic"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         return std::vector<TextAnimatorSpec>{make_slide_in_animator(params.based_on, params.stagger, 28.0, params.reveal)};
                     }});

    register_spec({"tachyon.textanim.pop_in",
                     {"tachyon.textanim.pop_in", "Pop In", "Pop in with scale and slide", "text.anim", {"pop", "dynamic"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         return std::vector<TextAnimatorSpec>{make_pop_in_animator(params.based_on, params.stagger, 18.0, params.reveal)};
                     }});

    register_spec({"tachyon.textanim.typewriter",
                     {"tachyon.textanim.typewriter", "Typewriter", "Character-by-character reveal with cursor", "text.anim", {"typewriter", "classic"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         double cps = params.stagger > 0 ? 1.0 / params.stagger : 20.0;
                         return std::vector<TextAnimatorSpec>{make_typewriter_minimal_animator(cps, true)};
                     }});

    register_spec({"tachyon.textanim.typewriter.classic",
                     {"tachyon.textanim.typewriter.classic", "Typewriter Classic", "Classic cursor-driven typewriter", "text.anim", {"typewriter", "classic"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         double cps = params.stagger > 0 ? 1.0 / params.stagger : 20.0;
                         return std::vector<TextAnimatorSpec>{make_typewriter_minimal_animator(cps, true)};
                     }});

    register_spec({"tachyon.textanim.typewriter.cursor",
                     {"tachyon.textanim.typewriter.cursor", "Typewriter Cursor", "Cursor-forward typewriter with stronger blink", "text.anim", {"typewriter", "cursor"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         double cps = params.stagger > 0 ? 1.0 / params.stagger : 19.0;
                         return std::vector<TextAnimatorSpec>{make_typewriter_cursor_animator(cps)};
                     }});

    register_spec({"tachyon.textanim.typewriter.soft",
                     {"tachyon.textanim.typewriter.soft", "Typewriter Soft", "Soft fade typewriter with blur settle", "text.anim", {"typewriter", "soft"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         double cps = params.stagger > 0 ? 1.0 / params.stagger : 16.0;
                         return std::vector<TextAnimatorSpec>{make_typewriter_soft_animator(cps)};
                     }});

    register_spec({"tachyon.textanim.typewriter.archive",
                     {"tachyon.textanim.typewriter.archive", "Typewriter Archive", "Muted archive-style typewriter", "text.anim", {"typewriter", "archive"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         double cps = params.stagger > 0 ? 1.0 / params.stagger : 16.0;
                         return std::vector<TextAnimatorSpec>{make_typewriter_archive_animator(cps)};
                     }});

    register_spec({"tachyon.textanim.typewriter.terminal",
                     {"tachyon.textanim.typewriter.terminal", "Typewriter Terminal", "Terminal-style console reveal", "text.anim", {"typewriter", "terminal"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         double cps = params.stagger > 0 ? 1.0 / params.stagger : 18.0;
                         return std::vector<TextAnimatorSpec>{make_typewriter_terminal_animator(cps)};
                     }});

    register_spec({"tachyon.textanim.typewriter.word",
                     {"tachyon.textanim.typewriter.word", "Typewriter Word", "Word-by-word reveal with typewriter cadence", "text.anim", {"typewriter", "word"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         double wps = params.stagger > 0 ? 1.0 / params.stagger : 4.0;
                         return std::vector<TextAnimatorSpec>{make_typewriter_word_animator(wps, false)};
                     }});

    register_spec({"tachyon.textanim.typewriter.word_cursor",
                     {"tachyon.textanim.typewriter.word_cursor", "Typewriter Word Cursor", "Word-by-word reveal with cursor", "text.anim", {"typewriter", "word", "cursor"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         double wps = params.stagger > 0 ? 1.0 / params.stagger : 4.0;
                         return std::vector<TextAnimatorSpec>{make_typewriter_word_cursor_animator(wps)};
                     }});

    register_spec({"tachyon.textanim.typewriter.line",
                     {"tachyon.textanim.typewriter.line", "Typewriter Line", "Line-by-line reveal with typewriter cadence", "text.anim", {"typewriter", "line"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         double lps = params.stagger > 0 ? 1.0 / params.stagger : 2.0;
                         return std::vector<TextAnimatorSpec>{make_typewriter_line_animator(lps, false)};
                     }});

    register_spec({"tachyon.textanim.typewriter.sentence",
                     {"tachyon.textanim.typewriter.sentence", "Typewriter Sentence", "Sentence-style reveal with soft timing", "text.anim", {"typewriter", "sentence"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         double wps = params.stagger > 0 ? 1.0 / params.stagger : 2.5;
                         return std::vector<TextAnimatorSpec>{make_typewriter_sentence_animator(wps)};
                     }});

    register_spec({"tachyon.textanim.blur_to_focus",
                     {"tachyon.textanim.blur_to_focus", "Blur to Focus", "Blur radius fade out", "text.anim", {"blur", "cinematic"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         return std::vector<TextAnimatorSpec>{make_blur_to_focus_animator(params.based_on, params.reveal, 8.0)};
                     }});

    register_spec({"tachyon.textanim.minimal_fade_up",
                     {"tachyon.textanim.minimal_fade_up", "Minimal Fade Up", "Fade up with Y offset", "text.anim", {"fade", "up"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         return std::vector<TextAnimatorSpec>{make_minimal_fade_up_animator(params.based_on, params.reveal, 12.0)};
                     }});

    register_spec({"tachyon.textanim.tracking_reveal",
                     {"tachyon.textanim.tracking_reveal", "Tracking Reveal", "Letter spacing reveal", "text.anim", {"tracking", "reveal"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         return std::vector<TextAnimatorSpec>{make_tracking_reveal_animator(params.based_on, params.reveal, 40.0)};
                     }});

    register_spec({"tachyon.textanim.soft_scale_in",
                     {"tachyon.textanim.soft_scale_in", "Soft Scale In", "Scale up from small", "text.anim", {"scale", "soft"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         return std::vector<TextAnimatorSpec>{make_soft_scale_in_animator(params.based_on, params.reveal, 0.95)};
                     }});

    register_spec({"tachyon.textanim.subtle_y_rotate",
                     {"tachyon.textanim.subtle_y_rotate", "Subtle Y Rotate", "Rotation on Y axis", "text.anim", {"rotate", "y-axis"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         return std::vector<TextAnimatorSpec>{make_subtle_y_rotate_animator(params.based_on, params.reveal, 5.0)};
                     }});

    register_spec({"tachyon.textanim.fill_wipe",
                     {"tachyon.textanim.fill_wipe", "Fill Wipe", "Color fill wipe", "text.anim", {"color", "wipe"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         return std::vector<TextAnimatorSpec>{make_fill_wipe_animator(params.based_on, params.reveal)};
                     }});

    register_spec({"tachyon.textanim.outline_to_solid",
                     {"tachyon.textanim.outline_to_solid", "Outline to Solid", "Outline width to zero", "text.anim", {"outline", "solid"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         return std::vector<TextAnimatorSpec>{make_outline_to_solid_animator(params.based_on, params.reveal, 1.0)};
                     }});

    register_spec({"tachyon.textanim.slide_mask",
                     {"tachyon.textanim.slide_mask", "Slide Mask", "Sliding mask reveal", "text.anim", {"mask", "slide"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         return std::vector<TextAnimatorSpec>{make_slide_mask_left_animator(params.based_on, params.reveal)};
                     }});

    register_spec({"tachyon.textanim.curtain_box",
                     {"tachyon.textanim.curtain_box", "Curtain Box", "Box-style curtain reveal", "text.anim", {"mask", "curtain"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         return std::vector<TextAnimatorSpec>{make_curtain_box_minimal_animator(params.based_on, params.reveal)};
                     }});

    register_spec({"tachyon.textanim.morphing",
                     {"tachyon.textanim.morphing", "Morphing", "Morphing words effect", "text.anim", {"morphing", "experimental"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         return std::vector<TextAnimatorSpec>{make_morphing_words_animator(params.reveal)};
                     }});

    register_spec({"tachyon.textanim.bounce_in",
                     {"tachyon.textanim.bounce_in", "Bounce In", "Bouncing slide in", "text.anim", {"bounce", "dynamic"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         return std::vector<TextAnimatorSpec>{make_bounce_in_animator(params.based_on, params.stagger, params.reveal, 34.0)};
                     }});

    register_spec({"tachyon.textanim.word_punch",
                     {"tachyon.textanim.word_punch", "Word Punch", "Punchy scale for words", "text.anim", {"word", "punch", "dynamic"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         return std::vector<TextAnimatorSpec>{make_word_punch_animator(params.stagger, params.reveal, 1.12)};
                     }});

    register_spec({"tachyon.textanim.word_by_word",
                     {"tachyon.textanim.word_by_word", "Word by Word", "Reveal words one by one", "text.anim", {"word", "basic"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         return std::vector<TextAnimatorSpec>{make_word_by_word_opacity_animator(params.stagger, params.reveal)};
                     }});

    register_spec({"tachyon.textanim.split_line",
                     {"tachyon.textanim.split_line", "Split Line", "Reveal lines one by one", "text.anim", {"line", "stagger"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         return std::vector<TextAnimatorSpec>{make_split_line_stagger_animator(params.stagger, params.reveal)};
                     }});

    register_spec({"tachyon.textanim.underline",
                     {"tachyon.textanim.underline", "Underline", "Drawing underline", "text.anim", {"underline", "draw"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         return std::vector<TextAnimatorSpec>{make_underline_draw_animator("words", params.reveal, 2.0)};
                     }});

    register_spec({"tachyon.textanim.number_flip",
                     {"tachyon.textanim.number_flip", "Number Flip", "Flipping numbers effect", "text.anim", {"number", "flip"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         return std::vector<TextAnimatorSpec>{make_number_flip_minimal_animator("characters", params.reveal)};
                     }});

    // Composite Presets
    register_spec({"tachyon.textanim.fade_up",
                     {"tachyon.textanim.fade_up", "Fade Up (Composite)", "Fade up with blur and scale", "text.anim", {"composite", "fade", "up"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         return std::vector<TextAnimatorSpec>{
                             make_minimal_fade_up_animator(params.based_on, params.reveal, 12.0),
                             make_blur_to_focus_animator(params.based_on, params.reveal, 8.0),
                             make_soft_scale_in_animator(params.based_on, params.reveal, 0.95)
                         };
                     }});

    register_spec({"tachyon.textanim.kinetic",
                     {"tachyon.textanim.kinetic", "Kinetic", "Motion blur with tracking", "text.anim", {"kinetic", "blur"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         return std::vector<TextAnimatorSpec>{
                             make_kinetic_blur_animator(200.0, params.reveal),
                             make_tracking_reveal_animator(params.based_on, params.reveal, 40.0)
                         };
                     }});

    register_spec({"tachyon.textanim.brushed_metal",
                     {"tachyon.textanim.brushed_metal", "Brushed Metal Title", "Premium metallic effect", "text.anim", {"metal", "premium"}},
                     {},
                     [get_text_params](const ParameterBag& p) {
                         auto params = get_text_params(p);
                         return std::vector<TextAnimatorSpec>{
                             make_tracking_reveal_animator(params.based_on, params.reveal, 40.0),
                             make_blur_to_focus_animator(params.based_on, params.reveal, 12.0),
                             make_minimal_fade_up_animator(params.based_on, params.reveal, 20.0)
                         };
                     }});
}

} // namespace tachyon::presets
