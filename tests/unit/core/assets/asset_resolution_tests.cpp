#include "test_utils.h"
#include "tachyon/core/assets/asset_resolution.h"
#include "tachyon/core/media/asset_resolver_interface.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include <filesystem>
#include <iostream>
#include <algorithm>

namespace tachyon::core::assets {

class MockAssetResolver : public ::tachyon::media::IAssetResolver {
public:
    const Config& config() const override {
        static Config s_config;
        return s_config;
    }

    std::optional<std::filesystem::path> resolve_path(const std::string& asset_id, ::tachyon::media::AssetType type) const override {
        (void)type;
        if (asset_id == "valid.png") {
            return std::filesystem::path("C:/fake/valid.png");
        }
        return std::nullopt;
    }

    std::shared_ptr<const renderer2d::SurfaceRGBA> resolve_image_shared(
        const std::string& spec, 
        ::tachyon::AlphaMode alpha_mode = ::tachyon::AlphaMode::Straight,
        ::tachyon::ResolveMode mode = ::tachyon::ResolveMode::PermissiveWithWarning) override {
        (void)spec; (void)alpha_mode; (void)mode;
        return nullptr;
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
        scene.project.id = "test_scene";

        MockAssetResolver resolver;
        auto result = resolve_assets(scene, resolver);

        if (!result.ok()) return false;
        if (result.value->size() != 1) return false;
        if (result.value->find("img1") == result.value->end()) return false;
        if (!result.value->at("img1").exists) return false;
        if (result.value->at("img1").source_path.string().find("valid.png") == std::string::npos) return false;
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
        scene.project.id = "test_scene_missing";

        MockAssetResolver resolver;
        auto result = resolve_assets(scene, resolver);

        if (result.ok()) return false;
        if (result.value && result.value->at("img1").exists) return false;
        
        bool found_error = std::any_of(result.diagnostics.diagnostics.begin(), result.diagnostics.diagnostics.end(),
            [](const auto& d) { return d.code == "asset.missing"; });
        if (!found_error) return false;
    }

    return true;
}

} // namespace tachyon::core::assets
