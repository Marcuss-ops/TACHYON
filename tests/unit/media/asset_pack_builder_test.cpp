#include "tachyon/media/asset_pack/asset_pack_builder.h"
#include <iostream>
#include <filesystem>
#include <fstream>

namespace {

int g_failures = 0;

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

} // namespace

bool run_asset_pack_builder_tests() {
    std::cout << "Running Asset Pack Builder tests..." << std::endl;
    g_failures = 0;

    test_builder_fail_invalid_input();

    if (g_failures == 0) {
        std::cout << "All Asset Pack Builder tests passed!" << std::endl;
    } else {
        std::cout << "Asset Pack Builder tests failed with " << g_failures << " errors." << std::endl;
    }

    return g_failures == 0;
}
