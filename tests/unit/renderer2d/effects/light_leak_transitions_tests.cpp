#include "test_utils.h"
#include "../../../../src/renderer2d/effects/core/transitions/light_leaks/light_leak_internal.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include <cmath>
#include <iostream>

using namespace tachyon;
using namespace tachyon::renderer2d;

bool run_light_leak_transitions_tests() {
    std::cout << "[RUN] Testing Light Leak Transitions Integrity (Procedural Engine)" << std::endl;
    bool all_ok = true;

    // Re-usable surface setup
    SurfaceRGBA dummy_src(10, 10);
    SurfaceRGBA dummy_dst(10, 10);
    for(uint32_t y=0; y<10; ++y) {
        for(uint32_t x=0; x<10; ++x) {
            dummy_src.set_pixel(x, y, Color(0.1f, 0.1f, 0.1f, 1.0f));
            dummy_dst.set_pixel(x, y, Color(0.8f, 0.8f, 0.8f, 1.0f));
        }
    }

    // --- TEST 1: Determinism ---
    // Running the same exact function twice must yield 100% identical bits.
    {
        LightLeakStyle test_style = {
            "test", "test", "test",
            {1,0,0,1}, {0,1,0,1}, {1,1,1,1},
            0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
            0.0f, 0.0f, LightLeakStyle::Shape::ProceduralRemotion
        };
        
        Color c1 = apply_light_leak_style(0.5f, 0.5f, 0.5f, dummy_src, &dummy_dst, test_style);
        Color c2 = apply_light_leak_style(0.5f, 0.5f, 0.5f, dummy_src, &dummy_dst, test_style);
        
        if (c1.r != c2.r || c1.g != c2.g || c1.b != c2.b) {
            std::cerr << "[FAIL] Test 1: Non-deterministic output detected!" << std::endl;
            all_ok = false;
        } else {
            std::cout << "[PASS] Test 1: Frame Determinism confirmed." << std::endl;
        }
    }

    // --- TEST 2: Progress Mapping & Reveal/Retract Curve ---
    // Curve must peak at 0.5, and be low/zero at 0.0 and 1.0
    // Coordinates MUST be away from center (0.9, 0.9) to escape center vignette suppression
    {
        LightLeakStyle test_style = {
            "test", "test", "test",
            {1,1,1,1}, {1,1,1,1}, {1,1,1,1},
            0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
            0.0f, 0.0f, LightLeakStyle::Shape::ProceduralRemotion
        };
        
        float mask_start = evaluate_light_leak_mask(0.9f, 0.9f, 0.0f, test_style);
        float mask_mid = evaluate_light_leak_mask(0.9f, 0.9f, 0.5f, test_style);
        float mask_end = evaluate_light_leak_mask(0.9f, 0.9f, 1.0f, test_style);
        
        if (mask_start > 0.01f || mask_end > 0.01f) {
            std::cerr << "[FAIL] Test 2: Reveal/Retract bounds leak at t=0/1 (" 
                      << mask_start << " / " << mask_end << ")" << std::endl;
            all_ok = false;
        } else if (mask_mid < 0.05f) {
             std::cerr << "[FAIL] Test 2: Curve does not peak at edges! Mid=" << mask_mid << std::endl;
             all_ok = false;
        } else {
            std::cout << "[PASS] Test 2: Reveal/Retract bell curve confirmed." << std::endl;
        }
    }

    // --- TEST 3: Procedural Noise Seeding ---
    // Changing 'direction' (our seed) must yield DIFFERENT noise outputs
    {
         LightLeakStyle s1 = {
            "test", "test", "test",
            {1,1,1,1}, {1,1,1,1}, {1,1,1,1},
            0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, // Seed 1
            0.0f, 0.0f, LightLeakStyle::Shape::ProceduralRemotion
        };
        LightLeakStyle s2 = {
            "test", "test", "test",
            {1,1,1,1}, {1,1,1,1}, {1,1,1,1},
            0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 4.2f, // Seed 4.2
            0.0f, 0.0f, LightLeakStyle::Shape::ProceduralRemotion
        };
        
        float m1 = evaluate_light_leak_mask(0.9f, 0.9f, 0.5f, s1);
        float m2 = evaluate_light_leak_mask(0.9f, 0.9f, 0.5f, s2);
        
        if (std::abs(m1 - m2) < 0.0001f) {
            std::cerr << "[FAIL] Test 3: Noise seed does not change outputs! M1=" << m1 << " M2=" << m2 << std::endl;
            all_ok = false;
        } else {
            std::cout << "[PASS] Test 3: Noise Seed stability & variation confirmed." << std::endl;
        }
    }

    // --- TEST 4: HueShift Spectral Synthesis ---
    // Changing angle_degrees MUST change the resulting RGB balance dynamically
    {
         LightLeakStyle h0 = {
            "test", "test", "test",
            {1,0,0,1}, {1,0,0,1}, {1,0,0,1},
            0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, // 0 deg = Default Amber/Orange
            0.0f, 0.0f, LightLeakStyle::Shape::ProceduralRemotion
        };
        LightLeakStyle h120 = {
            "test", "test", "test",
            {1,0,0,1}, {1,0,0,1}, {1,0,0,1},
            120.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, // 120 deg shift = Green/Teal territory
            0.0f, 0.0f, LightLeakStyle::Shape::ProceduralRemotion
        };
        
        Color c0 = apply_light_leak_style(0.9f, 0.9f, 0.5f, dummy_src, nullptr, h0);
        Color c120 = apply_light_leak_style(0.9f, 0.9f, 0.5f, dummy_src, nullptr, h120);
        
        float diff_r = std::abs(c0.r - c120.r);
        float diff_g = std::abs(c0.g - c120.g);
        float diff_b = std::abs(c0.b - c120.b);
        
        if (diff_r < 0.001f && diff_g < 0.001f && diff_b < 0.001f) {
            std::cerr << "[FAIL] Test 4: Hue shift had no impact! C0={" << c0.r << "," << c0.g << "," << c0.b << "} C120={" << c120.r << "," << c120.g << "," << c120.b << "}" << std::endl;
            all_ok = false;
        } else {
            std::cout << "[PASS] Test 4: HSL Dynamic Spectral Shift confirmed." << std::endl;
        }
    }

    // --- TEST 5: Linear Frame Composition (Blend Isolation) ---
    // The leak should mathematically aggregate additively onto the underlying base
    {
        LightLeakStyle s = {
            "test", "test", "test",
            {1,1,1,1}, {1,1,1,1}, {1,1,1,1},
            0.0f, 0.2f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, // Narrow width 0.2 so at t=0 center is empty!
            0.0f, 0.0f, LightLeakStyle::Shape::Sweep
        };
        
        // Base is completely black
        SurfaceRGBA black(1, 1); 
        black.set_pixel(0, 0, Color(0.0f, 0.0f, 0.0f, 1.0f));
        
        // Render fully transparent vs something
        Color base_color = apply_light_leak_style(0.5f, 0.5f, 0.0f, black, nullptr, s);
        Color active_color = apply_light_leak_style(0.5f, 0.5f, 0.5f, black, nullptr, s);
        
        // The active color MUST be brighter than zero base
        if (active_color.r <= base_color.r && active_color.g <= base_color.g && active_color.b <= base_color.b) {
             std::cerr << "[FAIL] Test 5: Leak additive intensity check failed! Active={" << active_color.r << "," << active_color.g << "," << active_color.b << "} Base={" << base_color.r << "," << base_color.g << "," << base_color.b << "}" << std::endl;
             all_ok = false;
        } else {
             std::cout << "[PASS] Test 5: Linear Additive composition validated." << std::endl;
        }
    }

    return all_ok;
}
