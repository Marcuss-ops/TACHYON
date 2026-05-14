#include "tachyon/runtime/execution/planning/render_preflight.h"
#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/transition_registry.h"
#include <gtest/gtest.h>

namespace tachyon::test {

TEST(RenderPreflightTest, DetectsMissingEffect) {
    renderer2d::EffectRegistry effect_reg;
    TransitionRegistry transition_reg;
    
    SceneSpec scene;
    scene.compositions.emplace_back();
    scene.compositions[0].id = "main";
    scene.compositions[0].layers.emplace_back();
    scene.compositions[0].layers[0].id = "glow_layer";
    scene.compositions[0].layers[0].type = LayerType::Image;
    
    EffectSpec fx;
    fx.type = "tachyon.effect.fake";
    scene.compositions[0].layers[0].effects.push_back(fx);
    
    auto result = runtime::RenderPreflight::validate(scene, &effect_reg, &transition_reg);
    
    EXPECT_FALSE(result.success);
    bool found_error = false;
    for (const auto& err : result.errors) {
        if (err.find("Effect type 'tachyon.effect.fake' not found") != std::string::npos) {
            found_error = true;
        }
    }
    EXPECT_TRUE(found_error);
}

TEST(RenderPreflightTest, DetectsMissingTransition) {
    renderer2d::EffectRegistry effect_reg;
    TransitionRegistry transition_reg;
    
    SceneSpec scene;
    scene.compositions.emplace_back();
    scene.compositions[0].id = "main";
    scene.compositions[0].layers.emplace_back();
    scene.compositions[0].layers[0].id = "layer1";
    scene.compositions[0].layers[0].type = LayerType::Image;
    scene.compositions[0].layers[0].transition_in.transition_id = "missing_transition";
    
    auto result = runtime::RenderPreflight::validate(scene, &effect_reg, &transition_reg);
    
    EXPECT_FALSE(result.success);
    bool found_error = false;
    for (const auto& err : result.errors) {
        if (err.find("Transition in not found: missing_transition") != std::string::npos) {
            found_error = true;
        }
    }
    EXPECT_TRUE(found_error);
}

} // namespace tachyon::test
