#include "tachyon/presets/background/background_kind_registry.h"
#include "tachyon/presets/background/background_builders.h"
#include <algorithm>
#include <cassert>
#include <iostream>

bool run_background_kind_registry_tests() {
    using namespace tachyon::presets;

    std::cout << "Running BackgroundKindRegistry tests..." << std::endl;

    {
        const auto ids = BackgroundKindRegistry::instance().list_ids();
        assert(!ids.empty()); // builtins registered in constructor
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

    std::cout << "BackgroundKindRegistry tests passed!" << std::endl;
    return true;
}
