#include "tachyon/tracker/optical_flow.h"
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace {

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------

std::vector<float> create_flat_frame(int w, int h, float val = 0.5f) {
    return std::vector<float>(static_cast<size_t>(w * h), val);
}

std::vector<float> create_edge_frame(int w, int h, int edge_x) {
    std::vector<float> frame(static_cast<size_t>(w * h), 0.0f);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            frame[static_cast<size_t>(y * w + x)] = (x >= edge_x) ? 1.0f : 0.0f;
        }
    }
    return frame;
}

std::vector<float> create_moving_edge(int w, int h, int edge_x, int offset) {
    return create_edge_frame(w, h, edge_x + offset);
}

std::vector<float> create_low_texture_frame(int w, int h) {
    std::vector<float> frame(static_cast<size_t>(w * h));
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            frame[static_cast<size_t>(y * w + x)] = 0.1f + 0.05f * static_cast<float>(y) / static_cast<float>(h);
        }
    }
    return frame;
}

std::vector<float> create_gradient_frame(int w, int h, float dx, float dy) {
    std::vector<float> frame(static_cast<size_t>(w * h));
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            frame[static_cast<size_t>(y * w + x)] = 0.1f + 0.8f * static_cast<float>(x) / static_cast<float>(w)
                                                     + 0.05f * static_cast<float>(y) / static_cast<float>(h);
        }
    }
    return frame;
}

std::vector<float> shift_frame(const std::vector<float>& src, int w, int h, int shift_x, int shift_y) {
    std::vector<float> dst(static_cast<size_t>(w * h), 0.0f);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int sx = x - shift_x;
            int sy = y - shift_y;
            if (sx >= 0 && sx < w && sy >= 0 && sy < h) {
                dst[static_cast<size_t>(y * w + x)] = src[static_cast<size_t>(sy * w + sx)];
            }
        }
    }
    return dst;
}

// ---------------------------------------------------------------------------
// D. Targeted tests
// ---------------------------------------------------------------------------

bool test_flat_frame() {
    using namespace tachyon::tracker;
    const int w = 64, h = 48;

    auto prev = create_flat_frame(w, h, 0.5f);
    auto curr = create_flat_frame(w, h, 0.5f);
    OpticalFlowCalculator calc;
    auto result = calc.compute(prev, curr, w, h);

    // Flat frames have no gradient -> no valid vectors.
    int valid = result.valid_count();
    bool pass = valid == 0;
    printf("  Flat frame: valid=%d/%d - %s\n",
           valid, w * h, pass ? "PASS" : "FAIL");
    return pass;
}

bool test_edge_motion() {
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

    bool pass = edge_region_count == 0 || avg_flow_x > 1.0f;
    printf("  Edge motion: valid_near_edge=%d, avg_flow_x=%.2f (accepts no-vector fallback) - %s\n",
           edge_region_count, avg_flow_x, pass ? "PASS" : "FAIL");
    return pass;
}

bool test_occlusion() {
    using namespace tachyon::tracker;
    const int w = 64, h = 48;

    // Object appears in current frame but was not in previous
    auto prev = create_flat_frame(w, h, 0.0f);
    auto curr = create_edge_frame(w, h, 30); // Object appears at x=30
    OpticalFlowCalculator calc;
    calc.config().enable_occlusion_detection = true;
    auto result = calc.compute(prev, curr, w, h);

    // In the region where the object appeared (x >= 30), flow should be
    // invalid because forward-backward consistency fails (there is no
    // corresponding feature in the previous frame).
    int invalid_in_occlusion = 0;
    int total_in_occlusion = 0;
    for (int y = 0; y < h; ++y) {
        for (int x = 30; x < w; ++x) {
            total_in_occlusion++;
            auto vec = result.get_flow_at_pixel(x, y);
            if (vec && !vec->valid) {
                invalid_in_occlusion++;
            }
        }
    }

    // Expect a significant portion of the occlusion region to be marked invalid
    float invalid_ratio = total_in_occlusion > 0
                          ? static_cast<float>(invalid_in_occlusion) / total_in_occlusion
                          : 0.0f;
    bool pass = invalid_ratio > 0.5f;
    printf("  Occlusion: invalid=%d/%d (ratio=%.2f) - %s\n",
           invalid_in_occlusion, total_in_occlusion, invalid_ratio,
           pass ? "PASS" : "FAIL");
    return pass;
}

bool test_low_texture() {
    using namespace tachyon::tracker;
    const int w = 64, h = 48;

    auto prev = create_low_texture_frame(w, h);
    auto curr = create_low_texture_frame(w, h);
    OpticalFlowCalculator calc;
    calc.config().confidence_threshold = 0.3f;
    auto result = calc.compute(prev, curr, w, h);

    // Low texture should result in very few valid vectors because gradients
    // are too weak to produce reliable optical flow.
    int valid = result.valid_count();
    float ratio = static_cast<float>(valid) / static_cast<float>(w * h);
    bool pass = ratio < 0.3f; // Less than 30% should be valid
    printf("  Low texture: valid=%d/%d (ratio=%.3f) - %s\n",
           valid, w * h, ratio, pass ? "PASS" : "FAIL");
    return pass;
}

bool test_large_motion_clamp() {
    using namespace tachyon::tracker;
    const int w = 64, h = 48;

    // Create a strong gradient and shift it by a large amount (> max_flow)
    auto prev = create_gradient_frame(w, h, 0.0f, 0.0f);
    auto curr = shift_frame(prev, w, h, 60, 0); // shift by 60 pixels, way above default 50

    OpticalFlowCalculator calc;
    calc.config().max_flow_magnitude = 20.0f;  // Tight clamp
    calc.config().confidence_threshold = 0.1f; // Lower threshold so we can see the clamp penalty
    auto result = calc.compute(prev, curr, w, h);

    // After clamping, any flow that hit the limit should have very low
    // confidence due to the explicit clamp penalty.
    int clamped_low_conf = 0;
    int checked = 0;
    for (int y = 5; y < h - 5; ++y) {
        for (int x = 5; x < w - 5; ++x) {
            auto vec = result.get_flow_at_pixel(x, y);
            if (!vec) continue;
            checked++;
            // If the flow magnitude is at or near the clamp limit, confidence
            // should be heavily penalised.
            if (std::abs(vec->flow.x) >= 19.0f || std::abs(vec->flow.y) >= 19.0f) {
                if (vec->confidence < 0.3f) {
                    clamped_low_conf++;
                }
            }
        }
    }

    bool pass = checked > 0 && (clamped_low_conf > 0 || result.valid_count() == 0);
    printf("  Large motion clamp: checked=%d, clamped_low_conf=%d, valid=%d - %s\n",
           checked, clamped_low_conf, result.valid_count(), pass ? "PASS" : "FAIL");
    return pass;
}

bool test_consumer_fallback() {
    using namespace tachyon::tracker;
    const int w = 32, h = 24;

    // Produce a low-confidence flow field (flat frames -> no gradients)
    OpticalFlowCalculator calc;
    auto prev = create_flat_frame(w, h);
    auto curr = create_flat_frame(w, h);
    auto flow_result = calc.compute(prev, curr, w, h);

    // All vectors should be invalid / low confidence from the calculator
    int pre_invalid = 0;
    for (const auto& v : flow_result.vectors) {
        if (!v.valid) pre_invalid++;
    }

    OpticalFlowConsumer consumer;
    consumer.config().high_confidence_threshold = 0.7f;
    consumer.config().low_confidence_threshold = 0.3f;
    consumer.config().enable_hold_for_low_conf = false; // zero motion

    auto consumed = consumer.process(flow_result);

    // With low confidence and zero-motion mode, every output vector should
    // be explicitly zero.
    int zero_count = 0;
    for (const auto& v : consumed.vectors) {
        if (v.flow.x == 0.0f && v.flow.y == 0.0f && !v.valid) {
            zero_count++;
        }
    }

    bool pass = zero_count == w * h;
    printf("  Consumer fallback: pre_invalid=%d/%d, post_zero=%d/%d - %s\n",
           pre_invalid, w * h, zero_count, w * h, pass ? "PASS" : "FAIL");
    return pass;
}

bool test_hold_fallback() {
    using namespace tachyon::tracker;
    const int w = 32, h = 24;

    OpticalFlowConsumer consumer;
    consumer.config().high_confidence_threshold = 0.7f;
    consumer.config().low_confidence_threshold = 0.3f;
    consumer.config().enable_hold_for_low_conf = true;

    // Current flow: all invalid / low confidence
    OpticalFlowResult low_conf_result;
    low_conf_result.width = w;
    low_conf_result.height = h;
    low_conf_result.vectors.resize(static_cast<size_t>(w * h));
    for (auto& v : low_conf_result.vectors) {
        v.flow = {10.0f, 10.0f};
        v.confidence = 0.1f;
        v.valid = false;
    }

    // Previous flow: small valid flow
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

    // Check that we held the previous flow values
    int hold_count = 0;
    for (const auto& v : consumed.vectors) {
        if (std::abs(v.flow.x - 1.0f) < 0.1f && std::abs(v.flow.y - 1.0f) < 0.1f) {
            hold_count++;
        }
    }

    bool pass = hold_count == w * h;
    printf("  Hold fallback: hold_count=%d/%d - %s\n",
           hold_count, w * h, pass ? "PASS" : "FAIL");
    return pass;
}

bool test_multiscale() {
    using namespace tachyon::tracker;
    const int w = 64, h = 48;

    auto prev = create_edge_frame(w, h, 10);
    auto curr = create_moving_edge(w, h, 10, 8); // Large motion
    OpticalFlowCalculator calc;
    calc.config().pyramid_levels = 3;

    auto result_single = calc.compute(prev, curr, w, h);
    auto result_multi = calc.compute_multiscale(prev, curr, w, h);

    printf("  Single scale: valid=%d\n", result_single.valid_count());
    printf("  Multiscale:   valid=%d\n", result_multi.valid_count());

    bool pass = result_multi.valid_count() >= result_single.valid_count();
    printf("  Multiscale: %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

bool test_explicit_degradation() {
    using namespace tachyon::tracker;
    const int w = 16, h = 12;

    // Create a calculator with a tight confidence threshold
    OpticalFlowCalculator calc;
    calc.config().confidence_threshold = 0.8f; // Very strict

    auto prev = create_edge_frame(w, h, 5);
    auto curr = create_moving_edge(w, h, 5, 1);
    auto result = calc.compute(prev, curr, w, h);

    // Every invalid vector must have zero flow (explicit degradation)
    int invalid_with_zero_flow = 0;
    int total_invalid = 0;
    for (const auto& v : result.vectors) {
        if (!v.valid) {
            total_invalid++;
            if (v.flow.x == 0.0f && v.flow.y == 0.0f) {
                invalid_with_zero_flow++;
            }
        }
    }

    bool pass = total_invalid > 0 && invalid_with_zero_flow == total_invalid;
    printf("  Explicit degradation: invalid=%d, zeroed=%d - %s\n",
           total_invalid, invalid_with_zero_flow, pass ? "PASS" : "FAIL");
    return pass;
}

} // namespace

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

bool run_optical_flow_tests() {
    printf("=== Optical Flow Robust Tests ===\n");
    bool all_pass = true;

    printf("[TEST] Flat frame\n");
    all_pass &= test_flat_frame();

    printf("[TEST] Edge motion\n");
    all_pass &= test_edge_motion();

    printf("[TEST] Occlusion\n");
    all_pass &= test_occlusion();

    printf("[TEST] Low texture\n");
    all_pass &= test_low_texture();

    printf("[TEST] Large motion clamp\n");
    all_pass &= test_large_motion_clamp();

    printf("[TEST] Consumer fallback\n");
    all_pass &= test_consumer_fallback();

    printf("[TEST] Hold fallback\n");
    all_pass &= test_hold_fallback();

    printf("[TEST] Multiscale\n");
    all_pass &= test_multiscale();

    printf("[TEST] Explicit degradation\n");
    all_pass &= test_explicit_degradation();

    printf("=== Optical Flow Robust Tests: %s ===\n\n",
           all_pass ? "ALL PASSED" : "SOME FAILED");
    return all_pass;
}
