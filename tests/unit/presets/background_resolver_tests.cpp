#include "tachyon/presets/background/background_resolver.h"
#include "tachyon/presets/background/background_preset_registry.h"
#include "tachyon/core/spec/schema/objects/background_spec.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include <cassert>
#include <iostream>

bool run_background_resolver_tests() {
    using namespace tachyon;
    using namespace tachyon::presets;

    std::cout << "Running BackgroundResolver tests..." << std::endl;

    // Warm up the preset registry so Preset-type tests work.
    BackgroundPresetRegistry::instance();

    // 1. Color → single Solid layer
    {
        auto bg = BackgroundSpec::from_string("#1a1a2e");
        assert(bg.is_color());

        auto res = resolve_background(bg, 1920, 1080, 5.0);
        assert(res.status == BackgroundResolutionResult::Status::Ok);
        assert(res.layers.size() == 1);

        const auto& layer = res.layers[0];
        assert(layer.type == LayerType::Solid);
        assert(!layer.id.empty());
        assert(layer.width == 1920);
        assert(layer.height == 1080);
        assert(layer.out_point == 5.0);
        assert(layer.enabled);
    }

    // 2. Color without parseable value → InvalidColor
    {
        BackgroundSpec bg;
        bg.type = BackgroundType::Color;
        // parsed_color left empty — resolver should produce InvalidColor status
        auto res = resolve_background(bg, 1920, 1080);
        assert(res.status == BackgroundResolutionResult::Status::InvalidColor);
        assert(res.layers.empty());
        assert(!res.error_message.empty());
    }

    // 3. Asset → single Image layer with asset_id set
    {
        BackgroundSpec bg;
        bg.type = BackgroundType::Asset;
        bg.value = "backgrounds/city.jpg";

        auto res = resolve_background(bg, 1280, 720, 3.0);
        assert(res.status == BackgroundResolutionResult::Status::Ok);
        assert(res.layers.size() == 1);

        const auto& layer = res.layers[0];
        assert(layer.type == LayerType::Image);
        assert(layer.asset_id == "backgrounds/city.jpg");
        assert(layer.width == 1280);
        assert(layer.height == 720);
        assert(layer.out_point == 3.0);
        assert(layer.enabled);
    }

    // 4. Preset (valid) → single non-null layer with preset_id set
    {
        BackgroundSpec bg;
        bg.type = BackgroundType::Preset;
        bg.value = "tachyon.backgroundpreset.cinematic_aura";

        auto res = resolve_background(bg, 1920, 1080, 8.0);
        assert(res.status == BackgroundResolutionResult::Status::Ok);
        assert(res.layers.size() == 1);

        const auto& layer = res.layers[0];
        assert(layer.type != LayerType::NullLayer);
        assert(!layer.id.empty());
        assert(layer.enabled);
        assert(layer.preset_id == "tachyon.backgroundpreset.cinematic_aura");
    }

    // 5. Preset (non-existent) → UnknownPreset
    {
        BackgroundSpec bg;
        bg.type = BackgroundType::Preset;
        bg.value = "tachyon.backgroundpreset.does_not_exist_xyz";

        auto res = resolve_background(bg, 1920, 1080);
        assert(res.status == BackgroundResolutionResult::Status::UnknownPreset);
        assert(res.layers.empty());
        assert(!res.error_message.empty());
    }

    // 6. Component → UnsupportedComponent
    {
        BackgroundSpec bg;
        bg.type = BackgroundType::Component;
        bg.value = "some_component_id";

        auto res = resolve_background(bg, 1920, 1080);
        assert(res.status == BackgroundResolutionResult::Status::UnsupportedComponent);
        assert(res.layers.empty());
        assert(!res.error_message.empty());
    }

    // 7. Every registered preset must produce a valid layer through the resolver
    {
        const auto ids = BackgroundPresetRegistry::instance().list_ids();
        assert(!ids.empty());

        registry::ParameterBag bag;
        bag.set("width", 1920.0f);
        bag.set("height", 1080.0f);
        bag.set("duration", 5.0);

        for (const auto& id : ids) {
            BackgroundSpec bg;
            bg.type = BackgroundType::Preset;
            bg.value = id;

            auto layers = resolve_background_to_layers(bg, 1920, 1080, 5.0);
            if (layers.empty()) {
                std::cerr << "BackgroundResolver: preset '" << id << "' resolved to empty layers.\n";
                return false;
            }
            const auto& layer = layers[0];
            if (layer.id.empty()) {
                std::cerr << "BackgroundResolver: preset '" << id << "' produced layer with empty id.\n";
                return false;
            }
            if (layer.type == LayerType::NullLayer) {
                std::cerr << "BackgroundResolver: preset '" << id << "' produced NullLayer.\n";
                return false;
            }
            if (!layer.enabled) {
                std::cerr << "BackgroundResolver: preset '" << id << "' produced disabled layer.\n";
                return false;
            }
        }
    }

    std::cout << "BackgroundResolver tests passed!" << std::endl;
    return true;
}
