#include "test_utils.h"
#include "tachyon/renderer2d/effects/core/transitions/transition_fast_paths.h"
#include "tachyon/renderer2d/effects/core/transitions/artistic_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/basic_transitions.h"
#include "tachyon/core/ids/builtin_ids.h"
#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/effects/effect_host.h"
#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/transition_registry.h"
#include <cmath>
#include <algorithm>
#include <iostream>

using namespace tachyon;
using namespace tachyon::renderer2d;

bool run_transition_fast_paths_tests() {
    const uint32_t width = 320;
    const uint32_t height = 180;
    SurfaceRGBA from(width, height);
    SurfaceRGBA to(width, height);
    
    // Fill with some data
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            from.set_pixel(x, y, Color(static_cast<float>(x)/width, static_cast<float>(y)/height, 0.0f, 1.0f));
            to.set_pixel(x, y, Color(0.0f, static_cast<float>(y)/height, static_cast<float>(x)/width, 1.0f));
        }
    }
    
    SurfaceRGBA fast_output(width, height);
    SurfaceRGBA generic_output(width, height);
    
    // Test multiple progress points
    const std::vector<float> tests = {0.1f, 0.3f, 0.5f, 0.8f};
    
    EffectRegistry registry;
    TransitionRegistry transition_registry;
    register_builtin_transitions(transition_registry);
    for (auto* desc : transition_registry.list_all()) {
        if (!desc) continue;
        TransitionDescriptor& d = const_cast<TransitionDescriptor&>(*desc);
        resolve_basic_transition_implementations(d);
        resolve_artistic_transition_implementations(d);
    }
    register_builtin_effects(registry, transition_registry);
    auto host = create_effect_host(registry);

    bool all_ok = true;
    for (float t : tests) {
        // Fast Path
        bool hit_fast = apply_transition_fast_path(std::string(ids::transition::pixelate), fast_output, from, &to, t, 1);
        if (!hit_fast) {
            std::cerr << "FAIL: Pixelate fast path not hit at t=" << t << std::endl;
            all_ok = false;
            continue;
        }
        
        // Generic Path
        EffectParams params;
        params.params["t"] = t;
        params.params["transition_id"] = std::string(ids::transition::pixelate);
        params.params["transition_to"] = static_cast<const SurfaceRGBA*>(&to);
        
        auto result = host->apply("tachyon.effect.transition.glsl", from, params);
        if (!result.value.has_value()) {
            std::cerr << "FAIL: Generic path application failed" << std::endl;
            all_ok = false;
            continue;
        }
        generic_output = *result.value;
        
        // Compare
        float total_error = 0.0f;
        float max_error = 0.0f;
        for (uint32_t i = 0; i < width * height * 4; ++i) {
            float err = std::abs(fast_output.pixels()[i] - generic_output.pixels()[i]);
            total_error += err;
            max_error = std::max(max_error, err);
        }
        
        float mean_error = total_error / (width * height * 4);
        if (mean_error > 0.05f || max_error > 0.15f) {
            std::cerr << "FAIL: Pixelate fast path error too high at t=" << t 
                      << " (mean=" << mean_error << ", max=" << max_error << ")" << std::endl;
            all_ok = false;
        }
    }

    // Fallback test
    SurfaceRGBA output_small(50, 50);
    if (apply_transition_fast_path(std::string(ids::transition::pixelate), output_small, from, &to, 0.5f, 1)) {
        std::cerr << "FAIL: Fast path should fallback on dimension mismatch" << std::endl;
        all_ok = false;
    }

    return all_ok;
}
