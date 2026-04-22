#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"
#include "tachyon/core/scene/evaluated_state.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/resource/render_context.h"
#include <iostream>
#include <cmath>
#include <string>

namespace {
static int g_failures = 0;
static void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}
}

namespace tachyon {
bool run_tiling_tests() {
    using namespace tachyon;
    
    scene::EvaluatedCompositionState state;
    state.width = 1024;
    state.height = 1024;
    
    // Complex scene with multiple layers to test tiling
    for (int i = 0; i < 10; ++i) {
        scene::EvaluatedLayerState layer;
        layer.id = "layer_" + std::to_string(i);
        layer.type = scene::LayerType::Solid;
        layer.enabled = true;
        layer.active = true;
        layer.visible = true;
        layer.opacity = 0.5;
        layer.width = 512;
        layer.height = 512;
        layer.local_transform.position = {static_cast<float>(i * 50), static_cast<float>(i * 50)};
        layer.fill_color = {255, static_cast<uint8_t>(i * 20), 0, 255};
        layer.blend_mode = "normal";
        state.layers.push_back(layer);
    }
    
    RenderPlan plan;
    plan.quality_policy.resolution_scale = 1.0f;
    
    FrameRenderTask task;
    task.frame_number = 0;
    task.cache_key.value = "tiling-test";
    
    renderer2d::RenderContext2D context;
    
    // 1. Render without tiling
    context.policy.tile_size = 0;
    const auto frame_no_tile = render_evaluated_composition_2d(state, plan, task, context);
    
    // 2. Render with tiling
    context.policy.tile_size = 256;
    const auto frame_with_tile = render_evaluated_composition_2d(state, plan, task, context);
    
    check_true(frame_no_tile.surface != nullptr, "Full frame render failed");
    check_true(frame_with_tile.surface != nullptr, "Tiled render failed");
    
    if (frame_no_tile.surface && frame_with_tile.surface) {
        const auto& s1 = *frame_no_tile.surface;
        const auto& s2 = *frame_with_tile.surface;
        
        check_true(s1.width() == s2.width(), "Width mismatch");
        check_true(s1.height() == s2.height(), "Height mismatch");

        bool identical = true;
        for (uint32_t y = 0; y < s1.height(); ++y) {
            for (uint32_t x = 0; x < s1.width(); ++x) {
                const auto p1 = s1.get_pixel(x, y);
                const auto p2 = s2.get_pixel(x, y);
                if (std::abs(p1.r - p2.r) > 1e-5f || std::abs(p1.g - p2.g) > 1e-5f || std::abs(p1.b - p2.b) > 1e-5f || std::abs(p1.a - p2.a) > 1e-5f) {
                    identical = false;
                    std::cerr << "Mismatch at " << x << "," << y << ": (" 
                              << p1.r << "," << p1.g << "," << p1.b << "," << p1.a << ") vs ("
                              << p2.r << "," << p2.g << "," << p2.b << "," << p2.a << ")\n";
                    break;
                }
            }
            if (!identical) break;
        }
        check_true(identical, "Tiled output is not bit-identical to full-frame output");
    }
    
    return g_failures == 0;
}
}
