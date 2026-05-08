#include "tachyon/background_registry.h"
#include "tachyon/presets/background/background_builders.h"
#include <algorithm>
#include <cassert>
#include <iostream>

bool run_background_registry_tests() {
    using namespace tachyon;
    using namespace tachyon::presets;

    std::cout << "Running BackgroundRegistry tests..." << std::endl;

    auto& registry = BackgroundRegistry::instance();

    // Register all builtins
    registry.register_all_builtins();

    {
        const auto ids = registry.list_all_ids();
        assert(!ids.empty()); // builtins registered
    }

    {
        BackgroundParams params;
        params.kind = "tachyon.background.kind.aura";
        auto layer = build_background(params);
        assert(layer.preset_id == "tachyon.background.kind.aura");
        assert(layer.procedural.has_value());
        assert(layer.procedural->kind == "tachyon.background.kind.aura");
    }

    {
        BackgroundParams params;
        params.kind = "tachyon.background.kind.unknown_kind";
        auto layer = build_background(params);
        assert(layer.preset_id == "tachyon.background.kind.unknown_kind");
        assert(layer.procedural.has_value());
        assert(layer.procedural->kind == "tachyon.background.kind.unknown_kind");
    }

    std::cout << "BackgroundRegistry tests passed!" << std::endl;
    return true;
}
