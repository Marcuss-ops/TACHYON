#include "tachyon/presets/background/background_resolver.h"
#include "tachyon/presets/background/background_preset_registry.h"
#include "tachyon/presets/background/fluent.h"
#include "tachyon/core/registry/parameter_bag.h"
#include "tachyon/core/policy/engine_policy.h"
#include <iostream>

namespace tachyon::presets {

BackgroundResolutionResult resolve_background(
    const BackgroundSpec& bg,
    int width,
    int height,
    double duration
) {
    BackgroundResolutionResult result;
    
    switch (bg.type) {
        case BackgroundType::None:
            result.status = BackgroundResolutionResult::Status::EmptyByDesign;
            return result;

        case BackgroundType::Color:
            if (bg.parsed_color) {
                result.layers = { background::solid(*bg.parsed_color, width, height, duration) };
                return result;
            }
            result.status = BackgroundResolutionResult::Status::InvalidColor;
            result.error_message = "Background color specification is invalid or missing.";
            return result;

        case BackgroundType::Asset:
            // TODO: check if asset exists in strict mode
            result.layers = { background::image(bg.value, width, height, duration) };
            return result;

        case BackgroundType::Preset: {
            presets::BackgroundPresetRegistry registry;
            registry::ParameterBag bag;
            bag.set("width", static_cast<float>(width));
            bag.set("height", static_cast<float>(height));
            bag.set("duration", duration);
            
            auto layer = registry.create(bg.value, bag);
            if (layer) {
                result.layers = { *layer };
                return result;
            }
            
            result.status = BackgroundResolutionResult::Status::UnknownPreset;
            result.error_message = "Background preset '" + bg.value + "' not found in registry.";
            return result;
        }

        case BackgroundType::Component:
            result.status = BackgroundResolutionResult::Status::UnsupportedComponent;
            result.error_message = "Component-type backgrounds are not yet implemented.";
            return result;

        default:
            result.status = BackgroundResolutionResult::Status::EmptyByDesign;
            return result;
    }
}

std::vector<LayerSpec> resolve_background_to_layers(
    const BackgroundSpec& bg,
    int width,
    int height,
    double duration
) {
    auto res = resolve_background(bg, width, height, duration);
    return res.layers;
}

std::vector<LayerSpec> resolve_background_to_layers_checked(
    const BackgroundSpec& bg,
    int width,
    int height,
    double duration
) {
    auto res = resolve_background(bg, width, height, duration);
    
    if (res.status != BackgroundResolutionResult::Status::Ok && 
        res.status != BackgroundResolutionResult::Status::EmptyByDesign) {
        
        if (EngineValidationPolicy::instance().strict_backgrounds) {
            std::cerr << "[Tachyon] STRICT ERROR: Background resolution failed: " << res.error_message << std::endl;
            // In strict mode, we might want to return an error layer or throw, 
            // but for now we follow the user's advice to avoid silent black frames.
            return {}; 
        }
    }
    
    return res.layers;
}

} // namespace tachyon::presets
