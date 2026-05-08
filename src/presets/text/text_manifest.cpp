#include "tachyon/presets/text/text_manifest.h"
#include "tachyon/presets/text/text_registry.h"
#include "tachyon/text/animation/text_presets.h"
#include "tachyon/text/core/TextLayerSpec.h"
#include <vector>

namespace tachyon::presets {

using namespace tachyon;

namespace {

// --- Layer Presets ---

TextLayerPresetSpec create_basic_text_preset() {
    TextLayerPresetSpec spec;
    spec.id = "tachyon.textlayer.basic";
    spec.metadata = {"tachyon.textlayer.basic", "Basic Text", "Simple text layer", "text", {"basic"}};
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
    spec.metadata = {"tachyon.textanim.fade_in", "Fade In", "Simple character fade in", "text.animator", {"fade"}};
    spec.schema = registry::ParameterSchema({
        {"stagger", "Stagger", "Delay between characters", 0.03, 0.0, 1.0},
        {"duration", "Duration", "Total reveal duration", 0.7, 0.1, 5.0}
    });
    spec.factory = [](const registry::ParameterBag& p) {
        return std::vector<TextAnimatorSpec>{
            tachyon::text::make_fade_in_animator(
                "characters_excluding_spaces",
                p.get_or<double>("stagger", 0.03),
                p.get_or<double>("duration", 0.7)
            )
        };
    };
    return spec;
}

TextAnimatorPresetSpec create_typewriter_animator_preset() {
    TextAnimatorPresetSpec spec;
    spec.id = "tachyon.textanim.typewriter";
    spec.metadata = {"tachyon.textanim.typewriter", "Typewriter", "Classic typewriter effect", "text.animator", {"typewriter"}};
    spec.schema = registry::ParameterSchema({
        {"cps", "Chars/Sec", "Characters per second", 20.0, 1.0, 100.0},
        {"cursor", "Cursor", "Cursor character", "|"}
    });
    spec.factory = [](const registry::ParameterBag& p) {
        return std::vector<TextAnimatorSpec>{
            tachyon::text::make_typewriter_animator(
                p.get_or<double>("cps", 20.0),
                p.get_or<std::string>("cursor", "|")
            )
        };
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
        create_typewriter_animator_preset()
    };
}

} // namespace tachyon::presets
