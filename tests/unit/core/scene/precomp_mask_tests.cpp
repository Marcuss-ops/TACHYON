#include "tachyon/core/scene/evaluation/evaluator.h"
#include <iostream>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

} // namespace

bool run_precomp_mask_tests() {
    using namespace tachyon;

    // test precomp evaluation
    {
        SceneSpec scene;
        CompositionSpec sub_comp;
        sub_comp.id = "sub";
        sub_comp.width = 100;
        sub_comp.height = 100;
        sub_comp.frame_rate = {30, 1};
        
        LayerSpec sub_layer;
        sub_layer.identity.id = "sub_layer";
        sub_layer.identity.type = LayerType::Solid;
        sub_layer.identity.enabled = true;
        sub_layer.playback.timing.start = 0.0;
        sub_layer.playback.timing.duration = 10.0;
        sub_comp.layers.push_back(sub_layer);
        
        scene.compositions.push_back(sub_comp);
        
        CompositionSpec main_comp;
        main_comp.id = "main";
        main_comp.width = 1920;
        main_comp.height = 1080;
        main_comp.frame_rate = {30, 1};
        
        LayerSpec precomp_layer;
        precomp_layer.identity.id = "precomp";
        precomp_layer.identity.type = LayerType::Precomp;
        precomp_layer.source.precomp_id = "sub";
        precomp_layer.identity.enabled = true;
        precomp_layer.playback.timing.start = 0.0;
        precomp_layer.playback.timing.duration = 10.0;
        main_comp.layers.push_back(precomp_layer);
        
        scene.compositions.push_back(main_comp);
        
        auto evaluated = scene::evaluate_scene_composition_state(scene, "main", std::int64_t(0));
        check_true(evaluated.has_value(), "main comp should be evaluated");
        if (evaluated) {
            check_true(evaluated->layers.size() == 1, "main comp should have 1 layer");
            check_true(evaluated->layers[0].nested_composition != nullptr, "precomp layer should have nested composition state");
            if (evaluated->layers[0].nested_composition) {
                check_true(evaluated->layers[0].nested_composition->composition_id == "sub", "nested comp id should be 'sub'");
                check_true(evaluated->layers[0].nested_composition->layers.size() == 1, "nested comp should have 1 layer");
            }
        }
    }

    // test track matte resolution
    {
        CompositionSpec comp;
        comp.id = "matte_test";
        comp.width = 100;
        comp.height = 100;
        
        LayerSpec matte_layer;
        matte_layer.identity.id = "matte";
        matte_layer.identity.type = LayerType::Solid;
        comp.layers.push_back(matte_layer);
        
        LayerSpec masked_layer;
        masked_layer.identity.id = "masked";
        masked_layer.identity.type = LayerType::Solid;
        masked_layer.track_matte_type = TrackMatteType::Alpha;
        masked_layer.track_matte_layer_id = "matte";
        comp.layers.push_back(masked_layer);
        
        auto evaluated = scene::evaluate_composition_state(comp, std::int64_t(0));
        check_true(evaluated.layers.size() == 2, "evaluated comp should have 2 layers");
        check_true(evaluated.layers[1].track_matte_layer_index.has_value(), "masked layer should have matte layer index");
        if (evaluated.layers[1].track_matte_layer_index) {
            // Note: matte layer index is relative to ALL layers in composition
            check_true(*evaluated.layers[1].track_matte_layer_index == 0, "matte index should be 0");
        }
    }

    return g_failures == 0;
}
