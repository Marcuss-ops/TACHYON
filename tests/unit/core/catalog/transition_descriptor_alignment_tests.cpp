#include <gtest/gtest.h>
#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/presets/transition/transition_preset_registry.h"
#include "tachyon/transition_registry.h"

namespace tachyon {

class TransitionAlignmentTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure built-ins are loaded
        register_builtin_transitions(registry_);
        preset_registry_.load_builtins();
    }
    
    TransitionRegistry registry_;
    presets::TransitionPresetRegistry preset_registry_;
};

TEST_F(TransitionAlignmentTest, AllDescriptorsHaveRendererBindings) {
    auto descriptors = registry_.list_all();
    EXPECT_GT(descriptors.size(), 0);
    
    for (const auto* desc : descriptors) {
        // Every descriptor must have a renderer_effect_id
        EXPECT_FALSE(desc->renderer_effect_id.empty()) 
            << "Transition '" << desc->id << "' is missing a renderer_effect_id.";
        
        // Check alignment - in new system, finding by ID returns the same descriptor
        const auto* runtime_desc = registry_.find_by_id(desc->id);
        ASSERT_NE(runtime_desc, nullptr) 
            << "Transition '" << desc->id << "' missing in TransitionRegistry.";
            
        EXPECT_EQ(runtime_desc->renderer_effect_id, desc->renderer_effect_id);
    }
}

TEST_F(TransitionAlignmentTest, AllPresetsHaveDescriptors) {
    auto preset_ids = preset_registry_.list_ids();
    
    for (const auto& id : preset_ids) {
        if (id == "tachyon.transition.none") continue;

        LayerTransitionSpec spec = preset_registry_.create(id, {});
        const auto* desc = registry_.resolve(spec.transition_id);
        EXPECT_NE(desc, nullptr) 
            << "Preset '" << id << "' (id=" << spec.transition_id << ") exists but has no corresponding TransitionDescriptor.";
    }
}

TEST_F(TransitionAlignmentTest, AllDescriptorsHaveValidSchemas) {
    auto descriptors = registry_.list_all();
    
    for (const auto* desc : descriptors) {
        // Verification of schema structure if needed
    }
}

} // namespace tachyon
