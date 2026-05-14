#include "tachyon/renderer2d/effects/effect_validation.h"
#include "tachyon/renderer2d/effects/effect_registry.h"
#include <gtest/gtest.h>

namespace tachyon::test {

TEST(EffectParamValidationTest, TypoFails) {
    renderer2d::EffectRegistry registry;
    renderer2d::EffectDescriptor desc;
    desc.id = "tachyon.effect.blur";
    desc.schema.params.push_back(registry::ParameterDef("radius", "Radius", "", 10.0f));
    registry.register_spec(std::move(desc));

    EffectSpec fx;
    fx.type = "tachyon.effect.blur";
    fx.set_param("raduis", 20.0f); // typo

    auto result = renderer2d::validate_effect(fx, registry);
    EXPECT_FALSE(result.valid);
    
    bool found_typo_error = false;
    for (const auto& issue : result.issues) {
        if (issue.message.find("Unknown parameter 'raduis'") != std::string::npos) {
            found_typo_error = true;
        }
    }
    EXPECT_TRUE(found_typo_error);
}

TEST(EffectParamValidationTest, UnknownEffectFails) {
    renderer2d::EffectRegistry registry;
    
    EffectSpec fx;
    fx.type = "tachyon.effect.unknown";
    
    auto result = renderer2d::validate_effect(fx, registry);
    EXPECT_FALSE(result.valid);
}

} // namespace tachyon::test
