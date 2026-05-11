#include "tachyon/media/asset_pack/asset_pack_builder.h"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace tachyon::media {

namespace {

MeshCompressor& select_mesh_compressor(const std::string& codec) {
#if defined(TACHYON_ENABLE_DRACO)
    if (codec == "draco") {
        return draco_mesh_compressor();
    }
#endif
    if (codec != "none") {
        std::cerr << "Warning: mesh codec " << codec << " requested but not enabled. Falling back to 'none'." << std::endl;
    }
    return none_mesh_compressor();
}

TextureCompressor& select_texture_compressor(const std::string& codec) {
#if defined(TACHYON_ENABLE_BASISU)
    if (codec == "basisu" || codec == "basis") {
        return basis_texture_compressor();
    }
#endif
    if (codec != "none") {
        std::cerr << "Warning: texture codec " << codec << " requested but not enabled. Falling back to 'none'." << std::endl;
    }
    return none_texture_compressor();
}

} // namespace

AssetPackBuilderResult AssetPackBuilder::build(const AssetPackBuilderOptions& options) {
    AssetPackBuilderResult result;

    try {
        std::filesystem::path out_dir = options.output_dir;
        std::filesystem::create_directories(out_dir);
        std::filesystem::create_directories(out_dir / "meshes");
        std::filesystem::create_directories(out_dir / "textures");
        std::filesystem::create_directories(out_dir / "metadata");

        // Use the selected compressors
        MeshCompressor& mesh_comp = select_mesh_compressor(options.mesh_codec);
        TextureCompressor& tex_comp = select_texture_compressor(options.texture_codec);

        // Placeholder for reading scene.glb and iterating over meshes/textures.
        // For PR 4, we just write a dummy manifest to prove the contract.
        
        PackedMeshEntry dummy_mesh;
        dummy_mesh.id = "dummy_mesh_0";
        dummy_mesh.source_path = options.input_path + "#mesh0";
        dummy_mesh.packed_path = "meshes/dummy_mesh_0.pack";
        dummy_mesh.codec = options.mesh_codec;
        
        MeshCompressionInput minput;
        auto moutput = mesh_comp.compress(minput);
        
        dummy_mesh.source_bytes = 0;
        dummy_mesh.packed_bytes = moutput.bytes.size();
        
        result.manifest.meshes.push_back(dummy_mesh);

        // Write the manifest
        write_asset_pack_manifest(result.manifest, (out_dir / "manifest.json").string());

        result.ok = true;
    } catch (const std::exception& e) {
        result.ok = false;
        result.error = e.what();
    }

    return result;
}

} // namespace tachyon::media