#include "tachyon/presets/text/text_manifest.h"
#include "tachyon/presets/text/text_layer_preset_registry.h"
#include "tachyon/presets/text/text_animator_preset_registry.h"
#include "tachyon/presets/text/text_params.h"
#include <vector>

namespace tachyon::presets {

namespace {

TextLayerPresetSpec create_basic_text_preset() {
    TextLayerPresetSpec spec;
    spec.id = "tachyon.textlayer.basic";
    spec.metadata = {"tachyon.textlayer.basic", "Basic Text", "Simple text layer", "text", {"basic"}};
    spec.schema = registry::ParameterSchema({
        {"text", "Text", "Text content", "Hello World"},
        {"font_size", "Font Size", "Size in points", 48.0, 1.0, 500.0}
    });
    spec.factory = [](const registry::ParameterBag& bag) {
        LayerSpec layer;
        layer.type = LayerType::Text;
        // Apply text parameters
        return layer;
    };
    return spec;
}

} // namespace

std::vector<TextLayerPresetSpec> TextManifest::generate_layer_preset_specs() const {
    std::vector<TextLayerPresetSpec> specs;
    
    specs.push_back(create_basic_text_preset());
    
    // Add more presets here as needed
    
    return specs;
}

std::vector<TextAnimatorPresetSpec> TextManifest::generate_animator_preset_specs() const {
    std::vector<TextAnimatorPresetSpec> specs;
    
    // Add animator presets here
    // For now, return empty vector
    
    return specs;
}

} // namespace tachyon::presets
