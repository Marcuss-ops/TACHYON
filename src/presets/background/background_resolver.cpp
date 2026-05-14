#include "tachyon/presets/background/background_resolver.h"
#include "tachyon/content/preset_catalog.h"
#include "tachyon/presets/background/background_builders.h"
#include "tachyon/background_registry.h"
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
                // Resolve via BackgroundRegistry - find solid background descriptor
                auto* desc = BackgroundRegistry::instance().find_by_id("tachyon.background.solid");
                if (desc && desc->build) {
                    registry::ParameterBag bag;
                    bag.set("color", *bg.parsed_color);
                    bag.set("width", static_cast<float>(width));
                    bag.set("height", static_cast<float>(height));
                    bag.set("duration", duration);
                    auto layer = desc->build(bag);
                    if (layer.identity.type != LayerType::NullLayer) {
                        if (layer.identity.id.empty()) layer.identity.id = "bg_solid_" + bg.value;
                        result.layers = { layer };
                        return result;
                    }
                }
                result.status = BackgroundResolutionResult::Status::InvalidColor;
                result.error_message = "Solid background descriptor not found or build failed.";
                return result;
            }
            result.status = BackgroundResolutionResult::Status::InvalidColor;
            result.error_message = "Background color specification is invalid or missing.";
            return result;

        case BackgroundType::Asset:
            // Resolve via BackgroundRegistry - find image background descriptor
            {
                auto* desc = BackgroundRegistry::instance().find_by_id("tachyon.background.image");
                if (desc && desc->build) {
                    registry::ParameterBag bag;
                    bag.set("asset_path", bg.value);
                    bag.set("width", static_cast<float>(width));
                    bag.set("height", static_cast<float>(height));
                    bag.set("duration", duration);
                    auto layer = desc->build(bag);
                    if (layer.identity.type != LayerType::NullLayer) {
                        if (layer.identity.id.empty()) layer.identity.id = "bg_image_" + bg.value;
                        result.layers = { layer };
                        return result;
                    }
                }
                // BackgroundRegistry lookup failed or returned NullLayer
                result.status = BackgroundResolutionResult::Status::AssetMissing;
                result.error_message = "Image background descriptor not found or build failed for: " + bg.value;
                return result;
            }

        case BackgroundType::Preset: {
            registry::ParameterBag bag;
            bag.set("width", static_cast<float>(width));
            bag.set("height", static_cast<float>(height));
            bag.set("duration", duration);

            auto layer = content::PresetCatalog::instance().create_background(bg.value, bag);
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
            return {};
        }
    }

    return res.layers;
}

} // namespace tachyon::presets
