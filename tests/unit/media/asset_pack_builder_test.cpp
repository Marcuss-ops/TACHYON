#include "tachyon/media/asset_pack/asset_pack_builder.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

int g_failures = 0;

std::string base64_triangle_glb() {
    // Minimal glTF asset with one indexed triangle and no textures.
    // Buffer payload contains:
    // - 3 vec3 positions
    // - 3 uint16 indices
    // Padding aligns bufferViews to 4-byte boundaries.
    return R"({
  "asset": { "version": "2.0" },
  "buffers": [
    {
      "byteLength": 42,
      "uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAABAAIA"
    }
  ],
  "bufferViews": [
    { "buffer": 0, "byteOffset": 0,  "byteLength": 36 },
    { "buffer": 0, "byteOffset": 36, "byteLength": 6 }
  ],
  "accessors": [
    {
      "bufferView": 0,
      "componentType": 5126,
      "count": 3,
      "type": "VEC3",
      "max": [1.0, 1.0, 0.0],
      "min": [0.0, 0.0, 0.0]
    },
    {
      "bufferView": 1,
      "componentType": 5123,
      "count": 3,
      "type": "SCALAR"
    }
  ],
  "meshes": [
    {
      "name": "tri",
      "primitives": [
        {
          "attributes": { "POSITION": 0 },
          "indices": 1,
          "mode": 4
        }
      ]
    }
  ],
  "nodes": [
    { "mesh": 0 }
  ],
  "scenes": [
    { "nodes": [0] }
  ],
  "scene": 0
})";
}

bool write_text_file(const std::filesystem::path& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    file << content;
    return static_cast<bool>(file);
}

void test_builder_fail_invalid_input() {
    tachyon::media::AssetPackBuilder builder;
    tachyon::media::AssetPackBuilderOptions options;
    options.input_path = "non_existent.glb";
    options.output_dir = "test_output";
    
    auto result = builder.build(options);
    if (result.ok) {
        std::cerr << "Builder should have failed for non-existent input." << std::endl;
        g_failures++;
    }
}

void test_builder_minimal_gltf_fixture() {
    const std::filesystem::path input_path = "test_builder_triangle.gltf";
    const std::filesystem::path output_dir = "test_builder_triangle_out";

    std::filesystem::remove_all(output_dir);
    std::filesystem::remove(input_path);

    if (!write_text_file(input_path, base64_triangle_glb())) {
        std::cerr << "Builder fixture setup failed: could not write glTF file." << std::endl;
        g_failures++;
        return;
    }

    tachyon::media::AssetPackBuilder builder;
    tachyon::media::AssetPackBuilderOptions options;
    options.input_path = input_path.string();
    options.output_dir = output_dir.string();
    options.mesh_codec = "none";
    options.texture_codec = "none";

    auto result = builder.build(options);
    if (!result.ok) {
        std::cerr << "Builder should have succeeded for minimal glTF fixture: " << result.error << std::endl;
        g_failures++;
        std::filesystem::remove(input_path);
        std::filesystem::remove_all(output_dir);
        return;
    }

    if (result.manifest.meshes.size() != 1) {
        std::cerr << "Builder fixture expected 1 mesh, got " << result.manifest.meshes.size() << std::endl;
        g_failures++;
    }
    if (!result.manifest.textures.empty()) {
        std::cerr << "Builder fixture expected no textures, got " << result.manifest.textures.size() << std::endl;
        g_failures++;
    }

    const auto manifest_path = output_dir / "manifest.json";
    const auto mesh_path = output_dir / result.manifest.meshes.front().packed_path.str();
    if (!std::filesystem::exists(manifest_path)) {
        std::cerr << "Builder fixture missing manifest output." << std::endl;
        g_failures++;
    }
    if (!std::filesystem::exists(mesh_path)) {
        std::cerr << "Builder fixture missing packed mesh output." << std::endl;
        g_failures++;
    }

    std::filesystem::remove(input_path);
    std::filesystem::remove_all(output_dir);
}

} // namespace

bool run_asset_pack_builder_tests() {
    std::cout << "Running Asset Pack Builder tests..." << std::endl;
    g_failures = 0;

    test_builder_fail_invalid_input();
    test_builder_minimal_gltf_fixture();

    if (g_failures == 0) {
        std::cout << "All Asset Pack Builder tests passed!" << std::endl;
    } else {
        std::cout << "Asset Pack Builder tests failed with " << g_failures << " errors." << std::endl;
    }

    return g_failures == 0;
}
