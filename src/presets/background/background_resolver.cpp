#include "tachyon/presets/background/background_resolver.h"
#include "tachyon/presets/background/background_preset_registry.h"
#include "tachyon/presets/background/fluent.h"
#include "tachyon/core/registry/parameter_bag.h"
#include "tachyon/background_catalog.h"
#include <iostream>

namespace tachyon::presets {

std::vector<LayerSpec> resolve_background_to_layers(
    const BackgroundSpec& bg,
    int width,
    int height,
    double duration
) {
    switch (bg.type) {
        case BackgroundType::Color:
            if (bg.parsed_color) {
                return { background::solid(*bg.parsed_color, width, height, duration) };
            }
            return {};

        case BackgroundType::Asset:
            return { background::image(bg.value, width, height, duration) };

        case BackgroundType::Preset: {
            registry::ParameterBag bag;
            bag.set("width", static_cast<float>(width));
            bag.set("height", static_cast<float>(height));
            bag.set("duration", duration);
            auto layer = BackgroundPresetRegistry::instance().create(bg.value, bag);
            if (layer) return { *layer };
            return {};
        }

        case BackgroundType::Component:
        default:
            return {};
    }
}

std::vector<LayerSpec> resolve_background_to_layers_checked(
    const BackgroundSpec& bg,
    int width,
    int height,
    double duration
) {
    if (bg.type == BackgroundType::Preset) {
        auto& catalog = BackgroundCatalog::instance();
        std::string error;
        if (!catalog.validate_preset(bg.value, error)) {
            std::cerr << "[Tachyon] Background validation failed: " << error << std::endl;
            // Throwing or returning empty depending on engine policy.
            // Strict mode means no silent fallback.
            return {}; 
        }
    }
    
    return resolve_background_to_layers(bg, width, height, duration);
}

} // namespace tachyon::presets
