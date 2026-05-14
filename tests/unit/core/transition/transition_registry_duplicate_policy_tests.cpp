#include "tachyon/transition_registry.h"
#include <gtest/gtest.h>

namespace tachyon::test {

TEST(TransitionRegistryTest, DuplicatePolicyReject) {
    TransitionRegistry registry;
    registry.set_duplicate_policy(RegistryDuplicatePolicy::Reject);
    
    TransitionDescriptor a;
    a.id = "test";
    a.display_name = "A";
    
    registry.register_descriptor(a);
    
    TransitionDescriptor b;
    b.id = "test";
    b.display_name = "B";
    
    EXPECT_THROW(registry.register_descriptor(b), RegistryError);
}

TEST(TransitionRegistryTest, DuplicatePolicyWarn) {
    TransitionRegistry registry;
    registry.set_duplicate_policy(RegistryDuplicatePolicy::Warn);
    
    TransitionDescriptor a;
    a.id = "test";
    a.display_name = "A";
    
    registry.register_descriptor(a);
    
    TransitionDescriptor b;
    b.id = "test";
    b.display_name = "B";
    
    registry.register_descriptor(b);
    
    EXPECT_EQ(registry.find_by_id("test")->display_name, "A");
}

TEST(TransitionRegistryTest, DuplicatePolicyReplace) {
    TransitionRegistry registry;
    registry.set_duplicate_policy(RegistryDuplicatePolicy::Replace);
    
    TransitionDescriptor a;
    a.id = "test";
    a.display_name = "A";
    a.aliases = {"alias_a"};
    
    registry.register_descriptor(a);
    
    TransitionDescriptor b;
    b.id = "test";
    b.display_name = "B";
    b.aliases = {"alias_b"};
    
    registry.register_descriptor(b);
    
    EXPECT_EQ(registry.find_by_id("test")->display_name, "B");
    EXPECT_EQ(registry.find_by_alias("alias_a"), nullptr);
    EXPECT_NE(registry.find_by_alias("alias_b"), nullptr);
}

} // namespace tachyon::test
