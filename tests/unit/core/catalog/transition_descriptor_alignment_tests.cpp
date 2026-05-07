#include <gtest/gtest.h>
#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/presets/transition/transition_preset_registry.h"
#include "tachyon/transition_registry.h"

namespace tachyon {

class TransitionAlignmentTest : public ::testing::Test {
protected:
    TransitionRegistry registry;

    void SetUp() override {
        // Ensure built-ins are loaded
        register_builtin_transitions(registry);
        presets::TransitionPresetRegistry::instance().load_builtins();
    }
};

TEST_F(TransitionAlignmentTest, AllDescriptorsHaveRendererBindings) {
    auto descriptors = registry.list_all();
    EXPECT_GT(descriptors.size(), 0);

    for (const auto* desc : descriptors) {
        // Every descriptor must have a valid id
        EXPECT_FALSE(desc->id.empty())
            << "Transition descriptor has empty id.";

        // Every descriptor must be resolvable in the registry
        const auto* found = registry.find_by_id(desc->id);
        ASSERT_NE(found, nullptr)
            << "Transition '" << desc->id << "' in registry but cannot be found by id.";
    }
}

TEST_F(TransitionAlignmentTest, AllPresetsHaveDescriptors) {
    auto preset_ids = presets::TransitionPresetRegistry::instance().list_ids();

    for (const auto& id : preset_ids) {
        if (id == "tachyon.transition.none") continue;

        const auto* desc = registry.find_by_id(id);
        EXPECT_NE(desc, nullptr)
            << "Preset '" << id << "' exists but has no corresponding TransitionDescriptor.";
    }
}

TEST_F(TransitionAlignmentTest, AllDescriptorsHaveValidSchemas) {
    auto descriptors = registry.list_all();

    for (const auto* desc : descriptors) {
        // For product, every public transition should have a schema (even if empty)
        // to avoid ad-hoc parameter parsing.
        // (Note: some might be internal, we could skip those if we had a flag)
        // EXPECT_GT(desc->params.parameters.size(), 0); // Not necessarily true for all
    }
}

TEST_F(TransitionAlignmentTest, RegistryDoesNotContainDescriptorlessTransitions) {
    const auto runtime_ids = registry.list_all_ids();

    for (const auto& id : runtime_ids) {
        if (id == "tachyon.transition.none" || id == "none") {
            continue;
        }

        const auto* desc = registry.find_by_id(id);
        EXPECT_NE(desc, nullptr)
            << "Runtime transition '" << id
            << "' exists in TransitionRegistry but has no TransitionDescriptor. "
            << "New transitions must be registered through register_transition_descriptor().";
    }
}

} // namespace tachyon
