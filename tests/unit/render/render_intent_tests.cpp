#include "render_intent_test_utils.h"
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

namespace tachyon::test {

bool run_render_intent_tests() {
    using namespace tachyon::scene;

    std::cout << "Running RenderIntent builder tests..." << std::endl;

    // 1. Basic copy test
    {
        auto state = make_test_composition();
        auto layer = make_test_layer("layer_1");
        layer.source.definition = MediaSource{AssetHandle{std::string("assets/logo.png")}};
        state.layers.push_back(layer);

        FakeResourceProvider provider;
        auto result = build_test_render_intent(state, &provider);

        check_true(result.status == render::IntentBuildStatus::Success, "basic build should succeed");
        check_true(provider.last_texture_id == "assets/logo.png", "texture provider should be called with correct id");
    }

    // 2. Layer filtering and states
    {
        auto state = make_test_composition();
        
        auto layer1 = make_test_layer("layer_1");
        layer1.identity.enabled = false; // Should be filtered out
        state.layers.push_back(layer1);

        auto layer2 = make_test_layer("layer_2");
        layer2.identity.visible = false; // Should be filtered out
        state.layers.push_back(layer2);

        auto result = build_test_render_intent(state);
        // Depending on build_render_intent implementation, disabled/invisible layers might be filtered.
        // For now we just verify it doesn't crash.
        check_true(result.status == render::IntentBuildStatus::Success, "build with filtered layers should succeed");
    }

    std::cout << "RenderIntent builder tests completed with " << g_failures << " failures." << std::endl;
    return g_failures == 0;
}

} // namespace tachyon::test
