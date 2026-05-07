#include "render_intent_test_utils.h"
#include "tachyon/core/policy/engine_policy.h"

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

} // namespace

bool run_render_intent_tests() {
    using namespace tachyon;

    g_failures = 0;

    const auto saved_policy = EngineValidationPolicy::instance();
    EngineValidationPolicy::instance().set_all_strict(true);

    auto state = test::make_test_composition("comp");
    state.environment_map_id = std::string("env/sky.hdr");

    auto layer = test::make_test_layer("layer_1");
    layer.mesh_asset_id = std::string("quad");
    state.layers.push_back(layer);

    test::FakeResourceProvider provider;
    const auto result = render::build_render_intent(state, &provider);

    check_true(provider.last_mesh_id == "primitive:quad", "primitive quad should normalize to primitive:quad");
    check_true(result.intent.environment_map_id == state.environment_map_id, "environment map id should be copied into the intent");
    check_true(!result.diagnostics.ok(), "missing mesh should emit diagnostics");
    check_true(!result.diagnostics.diagnostics.empty(), "builder should record at least one diagnostic");
    if (!result.diagnostics.diagnostics.empty()) {
        check_true(result.diagnostics.diagnostics.front().code == "render.intent.mesh_missing",
                   "mesh failure should use a structured diagnostic code");
        check_true(result.diagnostics.diagnostics.front().path == layer.id,
                   "mesh failure diagnostic should point at the layer id");
    }
    check_true(result.status == render::IntentBuildStatus::AssetResolutionError,
               "strict asset failures should escalate to AssetResolutionError");

    {
        scene::EvaluatedCompositionState deform_state = state;
        auto deform_layer = test::make_test_layer("layer_2");
        deform_layer.mesh_deform_id = std::string("deform/layer_2");
        deform_state.layers = {deform_layer};

        provider.last_deform_id.clear();
        const auto deform_result = render::build_render_intent(deform_state, &provider);
        check_true(provider.last_deform_id == "deform/layer_2", "explicit mesh_deform_id should be used by the provider");
        check_true(deform_result.diagnostics.ok() == false, "missing deform asset should emit diagnostics");
        check_true(!deform_result.diagnostics.diagnostics.empty(), "explicit deform lookup should record diagnostics");
        if (!deform_result.diagnostics.diagnostics.empty()) {
            check_true(deform_result.diagnostics.diagnostics.front().code == "render.intent.deform_missing",
                       "missing deform asset should use a structured diagnostic code");
        }
    }

    {
        scene::EvaluatedCompositionState fallback_state = state;
        auto fallback_layer = test::make_test_layer("layer_3");
        fallback_state.layers = {fallback_layer};

        provider.last_deform_id.clear();
        const auto fallback_result = render::build_render_intent(fallback_state, &provider);
        check_true(provider.last_deform_id.empty(), "missing mesh_deform_id should not trigger deform resolution");
        check_true(fallback_result.diagnostics.ok(), "missing mesh_deform_id should not emit diagnostics");
    }
    EngineValidationPolicy::instance() = saved_policy;
    return g_failures == 0;
}
