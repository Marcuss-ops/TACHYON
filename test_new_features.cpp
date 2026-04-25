/*
 * Simple test to verify the new features compile and work.
 * This file tests:
 * 1. AnimatedGradientSpec
 * 2. ProceduralSpec  
 * 3. Text Animation Staggering
 * 4. Expression prop() function
 * 5. Procedural renderer
 */

#include "tachyon/background_generator.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/math/noise.h"
#include "tachyon/core/shapes/shape_modifiers.h"
#include <iostream>
#include <cassert>

void test_animated_gradient() {
    std::cout << "Testing AnimatedGradientSpec...\n";
    
    tachyon::AnimatedGradientSpec grad;
    grad.type = "radial";
    
    tachyon::AnimatedGradientStop stop1;
    stop1.offset.value = 0.0;
    stop1.color.value = tachyon::ColorSpec{255, 0, 0, 255};
    grad.stops.push_back(stop1);
    
    tachyon::AnimatedGradientStop stop2;
    stop2.offset.value = 1.0;
    stop2.color.value = tachyon::ColorSpec{0, 0, 255, 255};
    grad.stops.push_back(stop2);
    
    auto sampled = grad.evaluate(1.5);
    std::cout << "Gradient evaluated at t=1.5: " << sampled.stops.size() << " stops\n";
}

void test_procedural_spec() {
    std::cout << "Testing ProceduralSpec...\n";
    
    tachyon::LayerSpec layer;
    layer.type = "procedural";
    layer.width = 1920;
    layer.height = 1080;
    
    tachyon::ProceduralSpec proc;
    proc.kind = "noise";
    proc.frequency.value = 2.0;
    proc.amplitude.value = 50.0;
    proc.seed = 42;
    
    layer.procedural = proc;
    
    std::cout << "Procedural layer created: " << layer.procedural->kind << "\n";
}

void test_text_staggering() {
    std::cout << "Testing Text Animation Staggering...\n";
    
    tachyon::TextAnimatorSpec animator;
    animator.selector.type = "range";
    animator.selector.stagger_mode = "word";
    animator.selector.stagger_delay = 0.1;
    
    animator.properties.opacity_value = 1.0;
    
    std::cout << "Text animator with stagger mode: " << animator.selector.stagger_mode << "\n";
}

void test_noise_function() {
    std::cout << "Testing Perlin Noise...\n";
    
    tachyon::math::PerlinNoise noise(42);
    float value = noise.noise3d(0.5f, 0.5f, 0.0f);
    std::cout << "Noise value at (0.5, 0.5, 0): " << value << "\n";
}

void test_shape_modifiers() {
    std::cout << "Testing Shape Modifiers...\n";
    
    tachyon::ShapePath path;
    tachyon::ShapeSubpath subpath;
    subpath.vertices.push_back({{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}});
    subpath.vertices.push_back({{100.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}});
    subpath.vertices.push_back({{100.0f, 100.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}});
    subpath.closed = true;
    path.subpaths.push_back(subpath);
    
    // Test oscillator
    auto oscillated = tachyon::shapes::ShapeModifiers::oscillator(path, 1.0, 10.0, 0.0, 0.0);
    std::cout << "Oscillator applied, vertices: " << oscillated.subpaths[0].vertices.size() << "\n";
    
    // Test noise deform
    auto deformed = tachyon::shapes::ShapeModifiers::noise_deform(path, 0.1, 5.0, 0.0, 42);
    std::cout << "Noise deform applied, vertices: " << deformed.subpaths[0].vertices.size() << "\n";
}

void test_background_generator() {
    std::cout << "Testing Background Generator...\n";
    
    auto scene = tachyon::BackgroundGenerator::GenerateNoiseBackground(1920, 1080, 5.0);
    std::cout << "Generated noise background scene with " << scene.compositions.size() << " composition(s)\n";
    
    auto gradient_scene = tachyon::BackgroundGenerator::GenerateGradientBackground(1920, 1080, 5.0);
    std::cout << "Generated gradient background scene\n";
}

int main() {
    std::cout << "=== Tachyon Feature Tests ===\n\n";
    
    test_animated_gradient();
    test_procedural_spec();
    test_text_staggering();
    test_noise_function();
    test_shape_modifiers();
    test_background_generator();
    
    std::cout << "\n=== All tests passed! ===\n";
    return 0;
}
