#include "tachyon/core/scene/evaluation/evaluator.h"
#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"

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
        scene.version = "1.0";
        
        CompositionSpec sub_comp;
        sub_comp.id = "sub";
        sub_comp.width = 100;
        sub_comp.height = 100;
        sub_comp.frame_rate = {30, 1};
        
        LayerSpec sub_layer;
        sub_layer.id = "sub_layer";
        sub_layer.type = "solid";
        sub_layer.enabled = true;
        sub_layer.in_point = 0.0;
        sub_layer.out_point = 10.0;
        sub_comp.layers.push_back(sub_layer);
        
        scene.compositions.push_back(sub_comp);
        
        CompositionSpec main_comp;
        main_comp.id = "main";
        main_comp.width = 1920;
        main_comp.height = 1080;
        main_comp.frame_rate = {30, 1};
        
        LayerSpec precomp_layer;
        precomp_layer.id = "precomp";
        precomp_layer.type = "precomp";
        precomp_layer.precomp_id = "sub";
        precomp_layer.enabled = true;
        precomp_layer.in_point = 0.0;
        precomp_layer.out_point = 10.0;
        main_comp.layers.push_back(precomp_layer);
        
        scene.compositions.push_back(main_comp);
        
        auto evaluated = scene::evaluate_scene_composition_state(scene, "main", 0LL);
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
        matte_layer.id = "matte";
        matte_layer.type = "solid";
        comp.layers.push_back(matte_layer);
        
        LayerSpec masked_layer;
        masked_layer.id = "masked";
        masked_layer.type = "solid";
        masked_layer.track_matte_type = TrackMatteType::Alpha;
        masked_layer.track_matte_layer_id = "matte";
        comp.layers.push_back(masked_layer);
        
        auto evaluated = scene::evaluate_composition_state(comp, 0LL);
        check_true(evaluated.layers.size() == 2, "evaluated comp should have 2 layers");
        check_true(evaluated.layers[1].track_matte_layer_index.has_value(), "masked layer should have matte layer index");
        if (evaluated.layers[1].track_matte_layer_index) {
            check_true(*evaluated.layers[1].track_matte_layer_index == 0, "matte index should be 0");
        }
    }

    return g_failures == 0;
}
