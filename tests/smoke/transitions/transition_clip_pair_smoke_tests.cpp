#include "tachyon/renderer2d/effects/core/transitions/transition_apply.h"
#include "tachyon/transition_registry.h"
#include "tachyon/core/ids/builtin_ids.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include <iostream>
#include <vector>

using namespace tachyon;
using namespace renderer2d;

int main() {
    std::cout << "Running Transition Clip Pair smoke tests..." << std::endl;

    auto registry = create_default_transition_registry();
    
    SurfaceRGBA from(1920, 1080);
    from.clear(Color{255, 0, 0, 255});
    
    SurfaceRGBA to(1920, 1080);
    to.clear(Color{0, 255, 0, 255});

    std::vector<std::string> test_ids = {
        std::string(ids::transition::crossfade),
        std::string(ids::transition::slide),
        std::string(ids::transition::zoom),
        std::string(ids::transition::luma_dissolve),
        std::string(ids::transition::flash_cut),
        "tachyon.transition.circle_iris"
    };

    for (const auto& id : test_ids) {
        std::cout << "Testing transition: " << id << " ... ";
        TransitionApplyRequest request;
        request.transition_id = id;
        request.from = &from;
        request.to = &to;
        request.progress = 0.5f;
        request.thread_count = 4;

        auto res = apply_transition(request, registry);
        if (!res.ok) {
            std::cerr << "FAILED: " << res.error_message << std::endl;
            return 1;
        }
        std::cout << "OK" << std::endl;
    }

    std::cout << "Transition Clip Pair smoke tests passed!" << std::endl;
    return 0;
}
