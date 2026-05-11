#include "tachyon/media/asset_pack/asset_pack_manifest.h"
#include "tachyon/media/compression/mesh_compressor.h"
#include "tachyon/media/compression/texture_compressor.h"

#include <iostream>

namespace {

int g_failures = 0;

void test_manifest_write() {
    tachyon::media::AssetPackManifest manifest;
    manifest.version = "0.2";
    
    tachyon::media::PackedMeshEntry mesh;
    mesh.id = "test_mesh";
    mesh.source_path = "test.glb#mesh0";
    mesh.packed_path = "meshes/test_mesh.pack";
    mesh.codec = "none";
    manifest.meshes.push_back(mesh);

    if (!tachyon::media::write_asset_pack_manifest(manifest, "test_manifest.json")) {
        std::cerr << "Failed to write manifest." << std::endl;
        g_failures++;
    }
}

void test_noop_compressors() {
    tachyon::media::MeshCompressionInput minput;
    minput.positions = {1.0f, 2.0f, 3.0f};
    auto moutput = tachyon::media::none_mesh_compressor().compress(minput);
    if (moutput.codec != "none" || moutput.bytes.empty()) {
        std::cerr << "None mesh compressor failed." << std::endl;
        g_failures++;
    }

    tachyon::media::TextureCompressionInput tinput;
    tinput.rgba = {255, 0, 0, 255};
    auto toutput = tachyon::media::none_texture_compressor().compress(tinput);
    if (toutput.codec != "none" || toutput.bytes.empty()) {
        std::cerr << "None texture compressor failed." << std::endl;
        g_failures++;
    }
}

void test_draco_compressor() {
#if defined(TACHYON_ENABLE_DRACO)
    tachyon::media::MeshCompressionInput minput;
    // A simple triangle
    minput.positions = {0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f};
    minput.indices = {0, 1, 2};
    
    auto moutput = tachyon::media::draco_mesh_compressor().compress(minput);
    if (moutput.codec != "draco" || moutput.bytes.empty()) {
        std::cerr << "Draco mesh compressor failed." << std::endl;
        g_failures++;
    } else {
        std::cout << "Draco compression successful: " << moutput.bytes.size() << " bytes." << std::endl;
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
        std::cerr << "Basis texture compressor failed." << std::endl;
        g_failures++;
    } else {
        std::cout << "Basis compression successful: " << toutput.bytes.size() << " bytes." << std::endl;
    }
#endif
}

} // namespace

bool run_asset_pack_manifest_tests() {
    std::cout << "Running Asset Pack Manifest tests..." << std::endl;
    g_failures = 0;

    test_manifest_write();
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
