#include "tachyon/renderer3d/modifiers/modifier3d_registry.h"
#include "tachyon/renderer3d/modifiers/tilt_3d_modifier.h"
#include "tachyon/renderer3d/surface/textured_plane_builder.h"
#include "tachyon/renderer3d/core/mesh_types.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/renderer2d/resource/render_context.h"

#include <iostream>
#include <memory>
#include <vector>
#include <cmath>

namespace {

static int g_failures = 0;

static void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

} // namespace

bool run_3d_modifier_tests() {
    using namespace tachyon;
    using namespace tachyon::renderer3d;

    // 1. Registry test
    Modifier3DRegistry registry;
    register_builtin_modifiers(registry);
    
    ThreeDModifierSpec tilt_spec;
    tilt_spec.type = "tachyon.modifier3d.tilt";
    tilt_spec.scalar_params["tilt_x"] = AnimatedScalarSpec(45.0);
    tilt_spec.scalar_params["tilt_y"] = AnimatedScalarSpec(0.0);

    auto modifier = registry.create(tilt_spec.type);
    check_true(modifier != nullptr, "Modifier 'tilt' created from registry");

    // 2. Mesh generation test
    LayerSurface surface;
    surface.width = 100;
    surface.height = 100;
    
    math::Vector2 size{100.0f, 100.0f};
    math::Vector2 anchor{50.0f, 50.0f};
    
    Mesh3D mesh = TexturedPlaneBuilder::build(surface, size, anchor);
    check_true(mesh.vertices.size() == 4, "Mesh has 4 vertices");
    check_true(mesh.indices.size() == 6, "Mesh has 6 indices");

    // Check vertex 0 (top-left)
    // left = -50, top = -50
    check_true(std::abs(mesh.vertices[0].position.x + 50.0f) < 0.001f, "Vertex 0 X is -50");
    check_true(std::abs(mesh.vertices[0].position.y + 50.0f) < 0.001f, "Vertex 0 Y is -50");

    // 3. Tilt modifier application test
    LayerSpec layer;
    renderer2d::RenderContext ctx;
    
    modifier->apply(mesh, 0.0, ctx);
    
    // After 45 degree tilt on X axis, vertex 0 Y should change
    // rotation_x(45deg): y' = y*cos(45) - z*sin(45) = -50*cos(45) - 0 = -50 * 0.707 = -35.35
    // z' = y*sin(45) + z*cos(45) = -50*sin(45) + 0 = -35.35
    check_true(std::abs(mesh.vertices[0].position.y + 35.3553f) < 0.1f, "Vertex 0 Y rotated correctly");
    check_true(std::abs(mesh.vertices[0].position.z + 35.3553f) < 0.1f, "Vertex 0 Z rotated correctly");

    // 4. Parallax modifier test
    ThreeDModifierSpec parallax_spec;
    parallax_spec.type = "tachyon.modifier3d.parallax";
    parallax_spec.scalar_params["depth"] = AnimatedScalarSpec(100.0);
    
    auto parallax_mod = registry.create(parallax_spec.type);
    check_true(parallax_mod != nullptr, "Modifier 'parallax' created from registry");
    
    parallax_mod->apply(mesh, 0.0, ctx);
    // After 100 depth shift, vertex 0 Z should be -35.35 + 100 = 64.65
    check_true(std::abs(mesh.vertices[0].position.z - 64.6446f) < 0.1f, "Vertex 0 Z shifted by parallax");

    return g_failures == 0;
}
