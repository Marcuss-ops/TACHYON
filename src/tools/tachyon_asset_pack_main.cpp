#include "tachyon/media/asset_pack/asset_pack_builder.h"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: tachyon-asset-pack <input> --out <output_dir> "
                     "[--mesh-codec none|draco] [--texture-codec none|basisu]\n";
        return 1;
    }

    tachyon::media::AssetPackBuilderOptions options;
    options.input_path = argv[1];

    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--out" && i + 1 < argc) {
            options.output_dir = argv[++i];
        } else if (arg == "--mesh-codec" && i + 1 < argc) {
            options.mesh_codec = argv[++i];
        } else if (arg == "--texture-codec" && i + 1 < argc) {
            options.texture_codec = argv[++i];
        } else {
            std::cerr << "Unknown or incomplete argument: " << arg << "\n";
            return 1;
        }
    }

    if (options.output_dir.empty()) {
        std::cerr << "Missing --out <output_dir>\n";
        return 1;
    }

    tachyon::media::AssetPackBuilder builder;
    const auto result = builder.build(options);

    if (!result.ok) {
        std::cerr << "Asset pack failed: " << result.error << "\n";
        return 1;
    }

    std::cout << "Asset pack written to: " << options.output_dir << "\n";
    std::cout << "Meshes: " << result.manifest.meshes.size() << "\n";
    std::cout << "Textures: " << result.manifest.textures.size() << "\n";

    return 0;
}