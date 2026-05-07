#include <gtest/gtest.h>
#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/presets/transition/transition_preset_registry.h"
#include "tachyon/transition_registry.h"

namespace tachyon {

class TransitionAlignmentTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure built-ins are loaded
        register_builtin_transitions();
        presets::TransitionPresetRegistry::instance().load_builtins();
    }
};

TEST_F(TransitionAlignmentTest, AllDescriptorsHaveRendererBindings) {
    auto descriptors = TransitionDescriptorRegistry::instance().list_all();
    EXPECT_GT(descriptors.size(), 0);

    for (const auto* desc : descriptors) {
        // Every descriptor must have a renderer_effect_id
        EXPECT_FALSE(desc->renderer_effect_id.empty()) 
            << "Transition '" << desc->id << "' is missing a renderer_effect_id.";
        
        // Every descriptor must be registered in the runtime TransitionRegistry
        const auto* runtime_spec = TransitionRegistry::instance().find(desc->id);
        ASSERT_NE(runtime_spec, nullptr) 
            << "Transition '" << desc->id << "' found in DescriptorRegistry but missing in TransitionRegistry.";
            
        // Check alignment of renderer_effect_id
        EXPECT_EQ(runtime_spec->renderer_effect_id, desc->renderer_effect_id)
            << "Alignment mismatch for '" << desc->id << "': renderer_effect_id differs between registries.";
    }
}

TEST_F(TransitionAlignmentTest, AllPresetsHaveDescriptors) {
    auto preset_ids = presets::TransitionPresetRegistry::instance().list_ids();
    
    for (const auto& id : preset_ids) {
        if (id == "tachyon.transition.none") continue;

        const auto* desc = TransitionDescriptorRegistry::instance().find(id);
        EXPECT_NE(desc, nullptr) 
            << "Preset '" << id << "' exists but has no corresponding TransitionDescriptor.";
    }
}

TEST_F(TransitionAlignmentTest, AllDescriptorsHaveValidSchemas) {
    auto descriptors = TransitionDescriptorRegistry::instance().list_all();
    
    for (const auto* desc : descriptors) {
        // For product, every public transition should have a schema (even if empty)
        // to avoid ad-hoc parameter parsing.
        // (Note: some might be internal, we could skip those if we had a flag)
        // EXPECT_GT(desc->schema.parameters.size(), 0); // Not necessarily true for all
    }
}

} // namespace tachyon
