#include "test_utils.h"
#include "tachyon/core/assets/asset_resolution.h"
#include "tachyon/core/media/asset_resolver_interface.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include <filesystem>
#include <iostream>

namespace tachyon::core::assets {

class MockAssetResolver : public ::tachyon::media::IAssetResolver {
public:
    std::optional<std::filesystem::path> resolve_path(const std::string& asset_id, ::tachyon::media::AssetType type) const override {
        (void)type;
        if (asset_id == "valid.png") {
            return std::filesystem::path("C:/fake/valid.png");
        }
        return std::nullopt;
    }
};

bool run_asset_resolution_tests() {
    // Test 1: Valid asset
    {
        SceneSpec scene;
        CompositionSpec comp;
        comp.id = "main";
        
        LayerSpec layer;
        layer.identity.id = "img1";
        layer.identity.type = LayerType::Image;
        MediaSource source;
        source.asset.id = "valid.png";
        layer.source = source;
        
        comp.layers.push_back(layer);
        scene.compositions.push_back(comp);

        MockAssetResolver resolver;
        auto result = resolve_assets(scene, resolver);

        if (!result.diagnostics.ok()) return false;
        if (result.value.size() != 1) return false;
        if (!result.value.count("img1")) return false;
        if (!result.value.at("img1").exists) return false;
        if (result.value.at("img1").source_path.string().find("valid.png") == std::string::npos) return false;
    }

    // Test 2: Missing asset
    {
        SceneSpec scene;
        CompositionSpec comp;
        comp.id = "main";
        
        LayerSpec layer;
        layer.identity.id = "img1";
        layer.identity.type = LayerType::Image;
        MediaSource source;
        source.asset.id = "missing.png";
        layer.source = source;
        
        comp.layers.push_back(layer);
        scene.compositions.push_back(comp);

        MockAssetResolver resolver;
        auto result = resolve_assets(scene, resolver);

        if (result.diagnostics.ok()) return false;
        if (result.value.at("img1").exists) return false;
        if (!result.diagnostics.has_error("asset.missing")) return false;
    }

    return true;
}

} // namespace tachyon::core::assets
