#include "tachyon/renderer2d/deform/mesh_deform.h"
#include "tachyon/renderer2d/raster/mesh_rasterizer.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include <cmath>
#include <iostream>
#include <string>

namespace {
int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

bool nearly_equal(float a, float b, float eps = 1e-5f) {
    return std::abs(a - b) <= eps;
}

} // namespace

bool run_mesh_deform_tests() {
    g_failures = 0;
    
    using namespace tachyon::renderer2d;
    
    // Test: generate_grid_mesh creates correct structure
    {
        DeformMesh mesh = generate_grid_mesh(100.0f, 100.0f, 5, 5);
        
        // 6x6 vertices = 36 vertices
        check_true(mesh.vertex_count() == 36, "grid 5x5 has 36 vertices");
        
        // 5x5x2 triangles = 50 triangles
        check_true(mesh.triangle_count() == 50, "grid 5x5 has 50 triangles");
        
        // Check vertex positions
        check_true(nearly_equal(mesh.vertices[0].position.x, 0.0f), "first vertex x=0");
        check_true(nearly_equal(mesh.vertices[0].position.y, 0.0f), "first vertex y=0");
        check_true(nearly_equal(mesh.vertices[35].position.x, 100.0f), "last vertex x=100");
        check_true(nearly_equal(mesh.vertices[35].position.y, 100.0f), "last vertex y=100");
        
        // Check UV coordinates
        check_true(nearly_equal(mesh.vertices[0].uv.x, 0.0f), "first vertex u=0");
        check_true(nearly_equal(mesh.vertices[0].uv.y, 0.0f), "first vertex v=0");
        check_true(nearly_equal(mesh.vertices[35].uv.x, 1.0f), "last vertex u=1");
        check_true(nearly_equal(mesh.vertices[35].uv.y, 1.0f), "last vertex v=1");
    }
    
    // Test: add_pin works
    {
        DeformMesh mesh = generate_grid_mesh(100.0f, 100.0f, 2, 2);
        std::size_t pin_id = add_pin(mesh, {50.0f, 50.0f});
        
        check_true(mesh.pin_count() == 1, "one pin added");
        check_true(pin_id == 0, "first pin has id 0");
        check_true(nearly_equal(mesh.pins[0].position.x, 50.0f), "pin at x=50");
        check_true(nearly_equal(mesh.pins[0].position.y, 50.0f), "pin at y=50");
    }
    
    // Test: move_pin deforms mesh
    {
        DeformMesh mesh = generate_grid_mesh(100.0f, 100.0f, 2, 2);
        add_pin(mesh, {50.0f, 50.0f});
        
        // Move pin to (80, 80)
        bool moved = move_pin(mesh, 0, {80.0f, 80.0f});
        check_true(moved, "pin moved successfully");
        
        // The center vertex should have moved
        // With falloff_radius=100, all vertices should be influenced
        bool deformed = false;
        for (const auto& v : mesh.vertices) {
            if (!nearly_equal(v.position.x, v.rest_position.x) ||
                !nearly_equal(v.position.y, v.rest_position.y)) {
                deformed = true;
                break;
            }
        }
        check_true(deformed, "mesh deformed after pin move");
    }
    
    // Test: reset_mesh_to_rest restores original positions
    {
        DeformMesh mesh = generate_grid_mesh(100.0f, 100.0f, 2, 2);
        add_pin(mesh, {50.0f, 50.0f});
        
        // Move pin
        move_pin(mesh, 0, {80.0f, 80.0f});
        
        // Reset
        reset_mesh_to_rest(mesh);
        
        // All vertices should be at rest position
        bool at_rest = true;
        for (const auto& v : mesh.vertices) {
            if (!nearly_equal(v.position.x, v.rest_position.x) ||
                !nearly_equal(v.position.y, v.rest_position.y)) {
                at_rest = false;
                break;
            }
        }
        check_true(at_rest, "mesh reset to rest positions");
    }
    
    // Test: rasterize produces output
    {
        DeformMesh mesh = generate_grid_mesh(100.0f, 100.0f, 5, 5);
        
        // Create a simple checkerboard source surface
        SurfaceRGBA source(10, 10);
        for (uint32_t y = 0; y < 10; ++y) {
            for (uint32_t x = 0; x < 10; ++x) {
                bool white = (x + y) % 2 == 0;
                source.set_pixel(x, y, white ? Color::white() : Color::black());
            }
        }
        
        // Rasterize
        std::vector<std::uint8_t> output_rgba(100 * 100 * 4);
        MeshRasterizer::rasterize(mesh, source, output_rgba.data(), 100, 100);
        
        // Check some pixels were rendered (not all zero)
        bool has_data = false;
        for (std::size_t i = 0; i < output_rgba.size(); ++i) {
            if (output_rgba[i] != 0) {
                has_data = true;
                break;
            }
        }
        check_true(has_data, "rasterize produces non-empty output");
    }
    
    std::cout << "Mesh deform tests: " << g_failures << " failures\n";
    return g_failures == 0;
}
