#include "tachyon/scene/builder.h"
#include "tachyon/presets/effects/effect_preset_registry.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

} // namespace

bool run_effect_preset_registry_tests() {
    using namespace tachyon;
    using namespace tachyon::presets;
    using tachyon::scene::LayerBuilder;

    g_failures = 0;

    EffectPresetRegistry registry;

    const auto ids = registry.list_ids();
    const std::string canonical_id = "tachyon.effect.generator.light_leak";
    const std::string legacy_id = "tachyon.effect.light_leak";

    check_true(std::count(ids.begin(), ids.end(), canonical_id) == 1, "canonical light leak id is registered once");
    check_true(std::count(ids.begin(), ids.end(), legacy_id) == 0, "legacy light leak alias is not registered");
    check_true(registry.find(canonical_id) != nullptr, "canonical light leak preset exists");
    check_true(registry.find(legacy_id) == nullptr, "legacy light leak preset is absent");

    {
        registry::ParameterBag params;
        params.set("progress", 0.5);
        params.set("speed", 1.25);
        params.set("seed", 7.0);

        const EffectSpec effect = registry.create(canonical_id, params);
        check_true(effect.enabled, "canonical preset is enabled");
        check_true(effect.type == canonical_id, "canonical preset returns canonical effect type");
        check_true(effect.scalars.contains("progress"), "canonical preset keeps progress parameter");
    }

    {
        bool threw = false;
        try {
            (void)registry.create("tachyon.effect.fake.nonexistent", {});
        } catch (const std::runtime_error& e) {
            threw = std::string(e.what()).find("Unknown effect 'tachyon.effect.fake.nonexistent'") != std::string::npos;
        }
        check_true(threw, "unknown effect id throws a clear runtime error");
    }

    {
        auto comp = tachyon::scene::Composition("effect_builder_comp", registry)
            .size(640, 360)
            .layer("leak", [](LayerBuilder& l) {
                registry::ParameterBag params;
                params.set("preset", 0.0);
                params.set("progress", 0.5);
                params.set("speed", 1.0);
                params.set("seed", 3.0);
                l.effects().preset("tachyon.effect.generator.light_leak", params).done();
            })
            .build();

        check_true(comp.layers.size() == 1, "builder creates one layer");
        check_true(!comp.layers[0].effects.empty(), "builder creates one effect");
        if (!comp.layers[0].effects.empty()) {
            check_true(comp.layers[0].effects[0].type == canonical_id, "builder uses canonical light leak effect id");
        }
    }

    return g_failures == 0;
}
