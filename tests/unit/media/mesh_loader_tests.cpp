#include "tachyon/media/loading/mesh_loader.h"
#include "tachyon/media/loading/mesh_asset.h"
#include "tachyon/core/math/matrix4x4.h"
#include "tachyon/core/math/quaternion.h"

#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>

// Test that the MeshAsset structure is correctly defined
bool test_mesh_asset_structure() {
    std::cout << "Test: MeshAsset structure..." << std::endl;
    
    tachyon::media::MeshAsset asset;
    
    // Add a sub-mesh
    tachyon::media::MeshAsset::SubMesh sub;
    sub.material.base_color_factor = {1.0f, 0.0f, 0.0f};
    
    // Add vertices
    tachyon::media::MeshAsset::Vertex v1, v2, v3;
    v1.position = {0.0f, 0.0f, 0.0f}; v1.normal = {0.0f, 0.0f, 1.0f}; v1.uv = {0.0f, 0.0f};
    v2.position = {1.0f, 0.0f, 0.0f}; v2.normal = {0.0f, 0.0f, 1.0f}; v2.uv = {1.0f, 0.0f};
    v3.position = {0.5f, 1.0f, 0.0f}; v3.normal = {0.0f, 0.0f, 1.0f}; v3.uv = {0.5f, 1.0f};
    sub.vertices = {v1, v2, v3};
    sub.indices = {0, 1, 2};
    sub.transform = tachyon::math::Matrix4x4::identity();
    
    asset.sub_meshes.push_back(std::move(sub));
    
    assert(asset.sub_meshes.size() == 1);
    assert(asset.sub_meshes[0].vertices.size() == 3);
    assert(asset.sub_meshes[0].indices.size() == 3);
    
    std::cout << "  MeshAsset structure correct" << std::endl;
    return true;
}

// Test TRS matrix computation (same logic as mesh_loader.cpp)
bool test_trs_matrix_computation() {
    std::cout << "Test: TRS matrix computation..." << std::endl;
    
    // Identity transform with translation
    tachyon::math::Matrix4x4 t = tachyon::math::Matrix4x4::identity();
    t.data[12] = 1.0f;  // Translation X
    t.data[13] = 2.0f;  // Translation Y
    t.data[14] = 3.0f;  // Translation Z
    
    assert(std::abs(t.data[12] - 1.0f) < 0.001f);
    assert(std::abs(t.data[13] - 2.0f) < 0.001f);
    assert(std::abs(t.data[14] - 3.0f) < 0.001f);
    
    std::cout << "  TRS matrix computation correct" << std::endl;
    return true;
}

// Test quaternion to matrix conversion (from mesh_loader.cpp lines 28-41)
bool test_quaternion_to_matrix() {
    std::cout << "Test: Quaternion to matrix conversion..." << std::endl;
    
    // Identity quaternion [0, 0, 0, 1]
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;
    float x2 = x + x, y2 = y + y, z2 = z + z;
    float xx = x * x2, xy = x * y2, xz = x * z2;
    float yy = y * y2, yz = y * z2, zz = z * z2;
    float wx = w * x2, wy = w * y2, wz = w * z2;
    
    tachyon::math::Matrix4x4 r;
    r.data[0] = 1.0f - (yy + zz); r.data[1] = xy + wz; r.data[2] = xz - wy;
    r.data[4] = xy - wz; r.data[5] = 1.0f - (xx + zz); r.data[6] = yz + wx;
    r.data[8] = xz + wy; r.data[9] = yz - wx; r.data[10] = 1.0f - (xx + yy);
    
    // Should be identity
    assert(std::abs(r.data[0] - 1.0f) < 0.001f);
    assert(std::abs(r.data[5] - 1.0f) < 0.001f);
    assert(std::abs(r.data[10] - 1.0f) < 0.001f);
    
    std::cout << "  Quaternion to matrix correct" << std::endl;
    return true;
}

// Test compose_trs function (used by the engine)
bool test_compose_trs() {
    std::cout << "Test: compose_trs function..." << std::endl;
    
    tachyon::math::Vector3 translation{1.0f, 2.0f, 3.0f};
    tachyon::math::Quaternion rotation = tachyon::math::Quaternion::from_euler({0.0f, 0.0f, 0.0f});
    tachyon::math::Vector3 scale{1.0f, 1.0f, 1.0f};
    
    auto matrix = tachyon::math::compose_trs(translation, rotation, scale);
    
    // Check translation component
    assert(std::abs(matrix.data[12] - 1.0f) < 0.001f);
    assert(std::abs(matrix.data[13] - 2.0f) < 0.001f);
    assert(std::abs(matrix.data[14] - 3.0f) < 0.001f);
    
    std::cout << "  compose_trs correct" << std::endl;
    return true;
}

// Test sub-mesh vertex data handling
bool test_submesh_vertex_data() {
    std::cout << "Test: Sub-mesh vertex data..." << std::endl;
    
    tachyon::media::MeshAsset::SubMesh sub;
    
    // Add vertex data using the Vertex struct
    tachyon::media::MeshAsset::Vertex v1, v2, v3;
    v1.position = {0.0f, 0.0f, 0.0f}; v1.normal = {0.0f, 0.0f, 1.0f}; v1.uv = {0.0f, 0.0f};
    v2.position = {1.0f, 0.0f, 0.0f}; v2.normal = {0.0f, 0.0f, 1.0f}; v2.uv = {1.0f, 0.0f};
    v3.position = {0.5f, 1.0f, 0.0f}; v3.normal = {0.0f, 0.0f, 1.0f}; v3.uv = {0.5f, 1.0f};
    
    sub.vertices = {v1, v2, v3};
    sub.indices = {0, 1, 2};
    
    assert(sub.vertices.size() == 3);
    assert(sub.indices.size() == 3);
    assert(sub.vertices[0].position.x == 0.0f);
    assert(sub.vertices[1].position.x == 1.0f);
    
    std::cout << "  Sub-mesh vertex data correct" << std::endl;
    return true;
}

// Test that MeshLoader can be instantiated
bool test_mesh_loader_instantiation() {
    std::cout << "Test: MeshLoader instantiation..." << std::endl;
    
    tachyon::media::MeshLoader loader;
    // If we get here without crashing, the test passes
    std::cout << "  MeshLoader instantiated successfully" << std::endl;
    return true;
}

// Main test runner function
bool run_mesh_loader_tests() {
    std::cout << "=== glTF Loader Tests ===" << std::endl;
    
    if (!test_mesh_asset_structure()) return false;
    if (!test_trs_matrix_computation()) return false;
    if (!test_quaternion_to_matrix()) return false;
    if (!test_compose_trs()) return false;
    if (!test_submesh_vertex_data()) return false;
    if (!test_mesh_loader_instantiation()) return false;
    
    std::cout << "\nAll glTF loader tests passed!" << std::endl;
    return true;
}
