#include "tachyon/presets/background/background_resolver.h"
#include "tachyon/presets/background/background_preset_registry.h"
#include "tachyon/presets/background/fluent.h"
#include "tachyon/core/registry/parameter_bag.h"

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

} // namespace tachyon::presets
