#include "tachyon/renderer2d/effects/core/transitions/transition_apply.h"
#include "tachyon/transition_registry.h"
#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/core/ids/builtin_ids.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include <cassert>
#include <iostream>

using namespace tachyon;
using namespace renderer2d;

int main() {
    std::cout << "Running Transition Apply Equivalence tests..." << std::endl;

    // 1. Setup registry
    auto registry = create_default_transition_registry();

    // 2. Setup surfaces
    SurfaceRGBA from(100, 100);
    from.clear(Color{255, 0, 0, 255}); // Red

    SurfaceRGBA to(100, 100);
    to.clear(Color{0, 255, 0, 255}); // Green

    // 3. Test Crossfade at 0.5
    TransitionApplyRequest request;
    request.transition_id = ids::transition::crossfade;
    request.from = &from;
    request.to = &to;
    request.progress = 0.5f;
    request.thread_count = 1;

    auto res = apply_transition(request, registry);

    if (!res.ok) {
        std::cerr << "FAILED: apply_transition returned error: " << res.error_message << std::endl;
        return 1;
    }

    if (res.output.width() != 100 || res.output.height() != 100) {
        std::cerr << "FAILED: Output surface size mismatch." << std::endl;
        return 1;
    }

    // Check middle pixel
    Color mid = res.output.get_pixel(50, 50);
    // Crossfade at 0.5 should be roughly 127/128 for red and green
    if (mid.r < 120 || mid.r > 135 || mid.g < 120 || mid.g > 135) {
        std::cerr << "FAILED: Crossfade color mismatch at 0.5. Got R=" << (int)mid.r << " G=" << (int)mid.g << std::endl;
        return 1;
    }

    // 4. Test "none" transition
    request.transition_id = "none";
    request.from = &from;
    request.progress = 0.8f;
    res = apply_transition(request, registry);
    if (!res.ok) {
        std::cerr << "FAILED: 'none' transition failed." << std::endl;
        return 1;
    }
    mid = res.output.get_pixel(50, 50);
    // none is a simple lerp
    if (mid.g < 200) {
        std::cerr << "FAILED: 'none' transition color mismatch at 0.8. Got G=" << (int)mid.g << std::endl;
        return 1;
    }

    std::cout << "Transition Apply Equivalence tests passed!" << std::endl;
    return 0;
}
