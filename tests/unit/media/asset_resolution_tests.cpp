#include "tachyon/media/asset_resolution.h"
#include "tachyon/runtime/core/diagnostics.h"
#include "tachyon/core/spec/scene_spec.h"

#include <filesystem>
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

} // namespace

bool run_asset_resolution_tests() {
    using namespace tachyon;

    // Test relative resolution
    {
        SceneSpec scene;
        AssetSpec asset;
        asset.id = "test_img";
        asset.type = "image";
        asset.source = "logo.png";
        scene.assets.push_back(asset);

        // We use the current directory as dummy root
        const auto root = std::filesystem::current_path();
        const auto result = resolve_assets(scene, root);

        // It should fail because logo.png doesn't exist in current_path
        check_true(!result.value.has_value(), "resolution should fail for missing files");
        check_true(!result.diagnostics.ok(), "missing files should produce diagnostic errors");
        bool found_error = false;
        for (const auto& d : result.diagnostics.diagnostics) {
            if (d.code == "asset.missing") {
                found_error = true;
                break;
            }
        }
        check_true(found_error, "diagnostic should contain 'asset.missing' code");
    }

    return g_failures == 0;
}
