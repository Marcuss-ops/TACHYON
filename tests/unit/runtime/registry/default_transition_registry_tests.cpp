#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/transition_registry.h"
#include "tachyon/core/ids/builtin_ids.h"
#include <cassert>
#include <iostream>

int main() {
    using namespace tachyon;

    std::cout << "Running Default Transition Registry completeness tests..." << std::endl;

    auto registry = create_default_transition_registry();

    const auto all_ids = registry.list_all_ids();
    assert(!all_ids.empty());

    for (auto* desc : registry.list_all()) {
        if (!desc) continue;

        // Every descriptor should have at least one implementation attached if it supports CPU/Fused
        if (desc->runtime_kind == TransitionRuntimeKind::CpuPixel) {
            if (desc->cpu_fn == nullptr && desc->direct_cpu_fn == nullptr) {
                std::cerr << "FAILED: Transition '" << desc->id << "' (CpuPixel) has no implementation attached.\n";
                return 1;
            }
        }
        if (desc->runtime_kind == TransitionRuntimeKind::FusedKernel) {
            if (desc->direct_cpu_fn == nullptr && desc->cpu_fn == nullptr) {
                std::cerr << "FAILED: Transition '" << desc->id << "' (FusedKernel) has no implementation attached.\n";
                return 1;
            }
        }
    }

    // Verify key transitions
    static const std::string_view kKeys[] = {
        ids::transition::crossfade,
        ids::transition::luma_dissolve,
        ids::transition::slide_up,
        ids::transition::flash_cut,
        "tachyon.transition.lightleak.golden_sweep"
    };

    for (const auto id : kKeys) {
        auto* desc = registry.resolve(id);
        if (!desc) {
            std::cerr << "FAILED: Key transition '" << id << "' not found in registry.\n";
            return 1;
        }
        if (desc->cpu_fn == nullptr && desc->direct_cpu_fn == nullptr) {
             std::cerr << "FAILED: Key transition '" << id << "' has no implementation attached.\n";
             return 1;
        }
    }

    std::cout << "Default Transition Registry completeness tests passed!" << std::endl;
    return 0;
}
