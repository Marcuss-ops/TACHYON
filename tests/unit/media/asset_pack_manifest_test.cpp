#include "tachyon/media/asset_pack/asset_pack_manifest.h"
#include "tachyon/media/compression/mesh_compressor.h"
#include "tachyon/media/compression/texture_compressor.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

int g_failures = 0;

void test_manifest_write_read_roundtrip() {
    tachyon::media::AssetPackManifest manifest;
    manifest.version = "0.2";

    // Mesh with special characters in names
    tachyon::media::PackedMeshEntry mesh;
    mesh.id = "test_mesh";
    mesh.source_path = "test.glb#mesh0";
    mesh.packed_path = "meshes/test_mesh.draco";
    mesh.codec = "draco";
    mesh.source_bytes = 1024;
    mesh.packed_bytes = 512;
    mesh.texture_ids = {"tex_diffuse", "tex_normal"};
    manifest.meshes.push_back(mesh);

    // Mesh with problematic characters
    tachyon::media::PackedMeshEntry mesh2;
    mesh2.id = "mesh_with\"quote_and\\backslash";
    mesh2.source_path = "test.glb#mesh1";
    mesh2.packed_path = "meshes/mesh2.draco";
    mesh2.codec = "draco";
    mesh2.source_bytes = 2048;
    mesh2.packed_bytes = 768;
    manifest.meshes.push_back(mesh2);

    // Texture
    tachyon::media::PackedTextureEntry tex;
    tex.id = "tex_diffuse";
    tex.source_path = "test.glb#image0";
    tex.packed_path = "textures/tex_diffuse.basis";
    tex.codec = "basis";
    tex.source_bytes = 65536;
    tex.packed_bytes = 8192;
    manifest.textures.push_back(tex);

    // Font
    tachyon::media::PackedFontEntry font;
    font.id = "font_main";
    font.source_path = "fonts/arial.ttf";
    font.packed_path = "fonts/arial.packed";
    font.source_bytes = 123456;
    font.packed_bytes = 123456;
    manifest.fonts.push_back(font);

    // Write
    std::string manifest_path = "test_manifest_rt.json";
    if (!tachyon::media::write_asset_pack_manifest(manifest, manifest_path)) {
        std::cerr << "FAIL: Failed to write manifest." << std::endl;
        g_failures++;
        return;
    }

    // Read back
    tachyon::media::AssetPackManifest read_back;
    if (!tachyon::media::read_asset_pack_manifest(manifest_path, read_back)) {
        std::cerr << "FAIL: Failed to read manifest." << std::endl;
        g_failures++;
        std::filesystem::remove(manifest_path);
        return;
    }

    // Validate version
    if (read_back.version != "0.2") {
        std::cerr << "FAIL: Version mismatch. Expected '0.2', got '" << read_back.version << "'" << std::endl;
        g_failures++;
    }

    // Validate meshes
    if (read_back.meshes.size() != 2) {
        std::cerr << "FAIL: Expected 2 meshes, got " << read_back.meshes.size() << std::endl;
        g_failures++;
    } else {
        const auto& m0 = read_back.meshes[0];
        if (m0.id != "test_mesh") {
            std::cerr << "FAIL: mesh[0].id. Expected 'test_mesh', got '" << m0.id << "'" << std::endl;
            g_failures++;
        }
        if (m0.codec != "draco") {
            std::cerr << "FAIL: mesh[0].codec. Expected 'draco', got '" << m0.codec << "'" << std::endl;
            g_failures++;
        }
        if (m0.source_bytes != 1024) {
            std::cerr << "FAIL: mesh[0].source_bytes. Expected 1024, got " << m0.source_bytes << std::endl;
            g_failures++;
        }
        if (m0.texture_ids.size() != 2 || m0.texture_ids[0] != "tex_diffuse") {
            std::cerr << "FAIL: mesh[0].texture_ids" << std::endl;
            g_failures++;
        }

        const auto& m1 = read_back.meshes[1];
        if (m1.id != "mesh_with\"quote_and\\backslash") {
            std::cerr << "FAIL: mesh[1].id with special chars. Got '" << m1.id << "'" << std::endl;
            g_failures++;
        }
    }

    // Validate textures
    if (read_back.textures.size() != 1) {
        std::cerr << "FAIL: Expected 1 texture, got " << read_back.textures.size() << std::endl;
        g_failures++;
    } else {
        const auto& t0 = read_back.textures[0];
        if (t0.id != "tex_diffuse" || t0.codec != "basis" || t0.packed_bytes != 8192) {
            std::cerr << "FAIL: texture[0] fields mismatch" << std::endl;
            g_failures++;
        }
    }

    // Validate fonts
    if (read_back.fonts.size() != 1) {
        std::cerr << "FAIL: Expected 1 font, got " << read_back.fonts.size() << std::endl;
        g_failures++;
    } else {
        const auto& f0 = read_back.fonts[0];
        if (f0.id != "font_main" || f0.source_bytes != 123456) {
            std::cerr << "FAIL: font[0] fields mismatch" << std::endl;
            g_failures++;
        }
    }

    // Read the raw JSON and check it's valid, escaped etc.
    {
        std::ifstream ifs(manifest_path);
        std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        // Check that quotes in the mesh id are escaped
        if (content.find("mesh_with\\\"quote") == std::string::npos) {
            std::cerr << "FAIL: JSON escaping not found for quote character" << std::endl;
            g_failures++;
        }
        if (content.find("\\\\backslash") == std::string::npos) {
            std::cerr << "FAIL: JSON escaping not found for backslash" << std::endl;
            g_failures++;
        }
    }

    std::filesystem::remove(manifest_path);

    if (g_failures == 0) {
        std::cout << "PASS: test_manifest_write_read_roundtrip" << std::endl;
    }
}

void test_manifest_read_invalid_json() {
    // Test reading non-existent file
    tachyon::media::AssetPackManifest manifest;
    if (tachyon::media::read_asset_pack_manifest("nonexistent_file_xyz.json", manifest)) {
        std::cerr << "FAIL: Should have failed for non-existent file" << std::endl;
        g_failures++;
    } else {
        std::cout << "PASS: test_manifest_read_invalid_json (missing file)" << std::endl;
    }
}

void test_manifest_read_bad_json() {
    std::string path = "test_manifest_bad.json";
    {
        std::ofstream ofs(path);
        ofs << "this is not json";
    }

    tachyon::media::AssetPackManifest manifest;
    if (tachyon::media::read_asset_pack_manifest(path, manifest)) {
        std::cerr << "FAIL: Should have failed for invalid JSON" << std::endl;
        g_failures++;
    } else {
        std::cout << "PASS: test_manifest_read_bad_json" << std::endl;
    }

    std::filesystem::remove(path);
}

void test_manifest_reject_absolute_paths() {
    tachyon::media::AssetPackManifest manifest;
    std::string path = "test_manifest_abs.json";
    {
        std::ofstream ofs(path);
        ofs << "{\n"
            << "  \"version\": \"0.1\",\n"
            << "  \"meshes\": [{\n"
            << "    \"id\": \"bad\",\n"
            << "    \"source_path\": \"/etc/passwd\",\n"
            << "    \"packed_path\": \"/tmp/evil\",\n"
            << "    \"codec\": \"none\"\n"
            << "  }]\n"
            << "}\n";
    }

    tachyon::media::AssetPackManifest result;
    if (tachyon::media::read_asset_pack_manifest(path, result)) {
        std::cerr << "FAIL: Should have failed for absolute paths" << std::endl;
        g_failures++;
        std::filesystem::remove(path);
        return;
    }
    std::cout << "PASS: test_manifest_reject_absolute_paths" << std::endl;

    std::filesystem::remove(path);
}

void test_manifest_reject_dotdot_paths() {
    tachyon::media::AssetPackManifest manifest;
    std::string path = "test_manifest_dotdot.json";
    {
        std::ofstream ofs(path);
        ofs << "{\n"
            << "  \"version\": \"0.1\",\n"
            << "  \"meshes\": [{\n"
            << "    \"id\": \"bad\",\n"
            << "    \"packed_path\": \"meshes/../../evil.draco\",\n"
            << "    \"codec\": \"none\"\n"
            << "  }]\n"
            << "}\n";
    }

    tachyon::media::AssetPackManifest result;
    if (tachyon::media::read_asset_pack_manifest(path, result)) {
        std::cerr << "FAIL: Should have failed for dotdot paths" << std::endl;
        g_failures++;
        std::filesystem::remove(path);
        return;
    }
    std::cout << "PASS: test_manifest_reject_dotdot_paths" << std::endl;

    std::filesystem::remove(path);
}

void test_manifest_escape_special_chars() {
    tachyon::media::AssetPackManifest manifest;
    manifest.version = "0.1";

    tachyon::media::PackedMeshEntry mesh;
    mesh.id = "newline\ntest";
    mesh.source_path = "normal_path";
    mesh.packed_path = "meshes/normal.meshbin";
    mesh.codec = "none";
    manifest.meshes.push_back(mesh);

    std::string path = "test_manifest_escape.json";
    if (!tachyon::media::write_asset_pack_manifest(manifest, path)) {
        std::cerr << "FAIL: write failed for special chars" << std::endl;
        g_failures++;
        return;
    }

    // Read raw JSON and check the newline is escaped
    {
        std::ifstream ifs(path);
        std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        if (content.find("newline\\ntest") == std::string::npos) {
            std::cerr << "FAIL: newline not escaped in JSON output" << std::endl;
            g_failures++;
        } else {
            std::cout << "PASS: test_manifest_escape_special_chars" << std::endl;
        }
    }

    // Roundtrip
    tachyon::media::AssetPackManifest result;
    if (!tachyon::media::read_asset_pack_manifest(path, result)) {
        std::cerr << "FAIL: roundtrip read failed" << std::endl;
        g_failures++;
        std::filesystem::remove(path);
        return;
    }

    if (result.meshes[0].id != "newline\ntest") {
        std::cerr << "FAIL: newline not preserved in roundtrip" << std::endl;
        g_failures++;
    }

    std::filesystem::remove(path);
}

void test_noop_compressors() {
    tachyon::media::MeshCompressionInput minput;
    minput.positions = {1.0f, 2.0f, 3.0f};
    auto moutput = tachyon::media::none_mesh_compressor().compress(minput);
    if (moutput.codec != "none" || moutput.bytes.empty()) {
        std::cerr << "FAIL: None mesh compressor." << std::endl;
        g_failures++;
    }

    tachyon::media::TextureCompressionInput tinput;
    tinput.width = 1;
    tinput.height = 1;
    tinput.channels = 4;
    tinput.rgba = {255, 0, 0, 255};
    auto toutput = tachyon::media::none_texture_compressor().compress(tinput);
    if (toutput.codec != "none" || toutput.bytes.empty()) {
        std::cerr << "FAIL: None texture compressor." << std::endl;
        g_failures++;
    }

    std::cout << "PASS: test_noop_compressors" << std::endl;
}

void test_draco_compressor() {
#if defined(TACHYON_ENABLE_DRACO)
    tachyon::media::MeshCompressionInput minput;
    // A simple triangle
    minput.positions = {0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f};
    minput.indices = {0, 1, 2};
    // Add UVs to verify they are preserved
    minput.uvs = {0.0f, 0.0f,  1.0f, 0.0f,  0.5f, 1.0f};

    auto moutput = tachyon::media::draco_mesh_compressor().compress(minput);
    if (moutput.codec != "draco" || moutput.bytes.empty()) {
        std::cerr << "FAIL: Draco mesh compressor." << std::endl;
        g_failures++;
    } else {
        std::cout << "PASS: Draco compression successful: " << moutput.bytes.size() << " bytes." << std::endl;
    }
#endif
}

void test_basis_compressor() {
#if defined(TACHYON_ENABLE_BASIS)
    tachyon::media::TextureCompressionInput tinput;
    tinput.width = 16;
    tinput.height = 16;
    tinput.channels = 4;
    tinput.rgba.resize(16 * 16 * 4, 255); // Solid white

    auto toutput = tachyon::media::basis_texture_compressor().compress(tinput);
    if (toutput.codec != "basis" || toutput.bytes.empty()) {
        std::cerr << "FAIL: Basis texture compressor." << std::endl;
        g_failures++;
    } else {
        std::cout << "PASS: Basis compression successful: " << toutput.bytes.size() << " bytes." << std::endl;
    }
#endif
}

} // namespace

bool run_asset_pack_manifest_tests() {
    std::cout << "Running Asset Pack Manifest tests..." << std::endl;
    g_failures = 0;

    test_manifest_write_read_roundtrip();
    test_manifest_read_invalid_json();
    test_manifest_read_bad_json();
    test_manifest_reject_absolute_paths();
    test_manifest_reject_dotdot_paths();
    test_manifest_escape_special_chars();
    test_noop_compressors();
    test_draco_compressor();
    test_basis_compressor();

    if (g_failures == 0) {
        std::cout << "All Asset Pack Manifest tests passed!" << std::endl;
    } else {
        std::cout << "Asset Pack Manifest tests failed with " << g_failures << " errors." << std::endl;
    }

    return g_failures == 0;
}
