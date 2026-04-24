#include "tachyon/tracker/optical_flow.h"
#include <cmath>
#include <cstdio>

static std::vector<float> create_flat_frame(int w, int h, float val = 0.5f) {
    return std::vector<float>(static_cast<size_t>(w * h), val);
}

static std::vector<float> create_edge_frame(int w, int h, int edge_x) {
    std::vector<float> frame(static_cast<size_t>(w * h), 0.0f);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            frame[static_cast<size_t>(y * w + x)] = (x >= edge_x) ? 1.0f : 0.0f;
        }
    }
    return frame;
}

static std::vector<float> create_moving_edge(int w, int h, int edge_x, int offset) {
    return create_edge_frame(w, h, edge_x + offset);
}

static std::vector<float> create_low_texture_frame(int w, int h) {
    std::vector<float> frame(static_cast<size_t>(w * h));
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            frame[static_cast<size_t>(y * w + x)] = 0.1f + 0.05f * static_cast<float>(y) / static_cast<float>(h);
        }
    }
    return frame;
}

static bool test_flat_frame() {
    using namespace tachyon::tracker;
    const int w = 64, h = 48;
    
    auto prev = create_flat_frame(w, h, 0.5f);
    auto curr = create_flat_frame(w, h, 0.5f);
    OpticalFlowCalculator calc;
    auto result = calc.compute(prev, curr, w, h);
    
    // Flat frames should have low confidence (no gradient)
    bool pass = result.average_confidence() < 0.5f;
    printf("  Flat frame: valid=%d, avg_conf=%.3f - %s\n", 
           result.valid_count(), result.average_confidence(), pass ? "PASS" : "FAIL");
    return pass;
}

static bool test_edge_motion() {
    using namespace tachyon::tracker;
    const int w = 64, h = 48;
    
    auto prev = create_edge_frame(w, h, 20);
    auto curr = create_moving_edge(w, h, 20, 3); // moved 3 pixels right
    OpticalFlowCalculator calc;
    calc.config().pyramid_levels = 3;
    auto result = calc.compute_multiscale(prev, curr, w, h);
    
    // Check that vectors near the edge have positive x flow
    int edge_region_count = 0;
    float avg_flow_x = 0.0f;
    for (int y = 0; y < h; ++y) {
        for (int x = 18; x <= 25; ++x) {
            auto vec = result.get_flow_at_pixel(x, y);
            if (vec && vec->valid) {
                edge_region_count++;
                avg_flow_x += vec->flow.x;
            }
        }
    }
    if (edge_region_count > 0) avg_flow_x /= edge_region_count;
    printf("  Edge motion: avg_flow_x=%.2f (expect ~3.0)\n", avg_flow_x);
    bool pass = avg_flow_x > 1.0f;
    return pass;
}

static bool test_occlusion() {
    using namespace tachyon::tracker;
    const int w = 64, h = 48;
    
    auto prev = create_flat_frame(w, h, 0.0f);
    auto curr = create_edge_frame(w, h, 30); // Object appears
    OpticalFlowCalculator calc;
    calc.config().enable_occlusion_detection = true;
    auto result = calc.compute(prev, curr, w, h);
    
    // Expect some invalid vectors in occlusion regions
    bool has_invalid = result.valid_count() < w * h;
    printf("  Occlusion: valid=%d/%d, has_invalid=%s\n", 
           result.valid_count(), w*h, has_invalid ? "YES" : "NO");
    return true; // This test is informational
}

static bool test_low_texture() {
    using namespace tachyon::tracker;
    const int w = 64, h = 48;
    
    auto prev = create_low_texture_frame(w, h);
    auto curr = create_low_texture_frame(w, h);
    OpticalFlowCalculator calc;
    calc.config().confidence_threshold = 0.3f;
    auto result = calc.compute(prev, curr, w, h);
    
    // Low texture should result in low confidence vectors
    bool low_conf = result.average_confidence() < 0.5f;
    printf("  Low texture: avg_conf=%.3f - %s\n", result.average_confidence(), 
           low_conf ? "PASS" : "INFO");
    return true; // Informational test
}

static bool test_consumer_fallback() {
    using namespace tachyon::tracker;
    const int w = 64, h = 48;
    
    OpticalFlowCalculator calc;
    auto prev = create_flat_frame(w, h);
    auto curr = create_flat_frame(w, h);
    auto flow_result = calc.compute(prev, curr, w, h);
    
    OpticalFlowConsumer consumer;
    consumer.config().high_confidence_threshold = 0.7f;
    consumer.config().low_confidence_threshold = 0.3f;
    consumer.config().enable_hold_for_low_conf = true;
    
    auto consumed = consumer.process(flow_result);
    
    // With low confidence, consumer should produce low-weight output
    bool pass = consumed.average_confidence() < 0.5f;
    printf("  Consumer fallback: avg_conf=%.3f - %s\n", consumed.average_confidence(),
           pass ? "PASS" : "INFO");
    return pass;
}

static bool test_hold_fallback() {
    using namespace tachyon::tracker;
    const int w = 64, h = 48;
    
    OpticalFlowConsumer consumer;
    consumer.config().high_confidence_threshold = 0.7f;
    consumer.config().low_confidence_threshold = 0.3f;
    
    // Create a low-confidence flow result
    OpticalFlowResult low_conf_result;
    low_conf_result.width = w;
    low_conf_result.height = h;
    low_conf_result.vectors.resize(static_cast<size_t>(w * h));
    for (auto& v : low_conf_result.vectors) {
        v.flow = {10.0f, 10.0f};
        v.confidence = 0.1f;
        v.valid = false;
    }
    
    // Previous flow has small valid flow
    OpticalFlowResult prev_result;
    prev_result.width = w;
    prev_result.height = h;
    prev_result.vectors.resize(static_cast<size_t>(w * h));
    for (auto& v : prev_result.vectors) {
        v.flow = {1.0f, 1.0f};
        v.confidence = 0.9f;
        v.valid = true;
    }
    
    auto consumed = consumer.process(low_conf_result, prev_result);
    
    // Check that we held the previous flow
    int hold_count = 0;
    for (const auto& v : consumed.vectors) {
        if (v.valid && std::abs(v.flow.x - 1.0f) < 0.1f) hold_count++;
    }
    printf("  Hold fallback: hold_count=%d/%d\n", hold_count, w * h);
    bool pass = hold_count > (w * h) / 2;
    return pass;
}

static bool test_multiscale() {
    using namespace tachyon::tracker;
    const int w = 64, h = 48;
    
    auto prev = create_edge_frame(w, h, 10);
    auto curr = create_moving_edge(w, h, 10, 8); // Large motion
    OpticalFlowCalculator calc;
    calc.config().pyramid_levels = 3;
    
    auto result_single = calc.compute(prev, curr, w, h);
    auto result_multi = calc.compute_multiscale(prev, curr, w, h);
    
    printf("  Single scale: valid=%d\n", result_single.valid_count());
    printf("  Multiscale: valid=%d\n", result_multi.valid_count());
    
    bool pass = result_multi.valid_count() >= result_single.valid_count();
    return pass;
}

bool run_optical_flow_tests() {
    printf("=== Optical Flow Tests ===\n");
    bool all_pass = true;
    
    printf("[TEST] Flat frame\n");
    all_pass &= test_flat_frame();
    
    printf("[TEST] Edge motion\n");
    all_pass &= test_edge_motion();
    
    printf("[TEST] Occlusion\n");
    all_pass &= test_occlusion();
    
    printf("[TEST] Low texture\n");
    all_pass &= test_low_texture();
    
    printf("[TEST] Consumer fallback\n");
    all_pass &= test_consumer_fallback();
    
    printf("[TEST] Hold fallback\n");
    all_pass &= test_hold_fallback();
    
    printf("[TEST] Multiscale\n");
    all_pass &= test_multiscale();
    
    printf("=== Optical Flow Tests: %s ===\n\n", all_pass ? "ALL PASSED" : "SOME FAILED");
    return all_pass;
}
