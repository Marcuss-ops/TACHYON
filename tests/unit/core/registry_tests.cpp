#include <gtest/gtest.h>
#include "tachyon/core/registry/typed_registry.h"

namespace tachyon::registry::test {

struct MockSpec {
    std::string id;
    int value;
};

TEST(TypedRegistryTest, BasicOperations) {
    TypedRegistry<MockSpec> registry;
    
    MockSpec s1{"test.id.1", 10};
    MockSpec s2{"test.id.2", 20};
    
    registry.register_spec(s1);
    registry.register_spec(s2);
    
    EXPECT_EQ(registry.size(), 2);
    EXPECT_TRUE(registry.contains("test.id.1"));
    EXPECT_TRUE(registry.contains("test.id.2"));
    EXPECT_FALSE(registry.contains("nonexistent"));
    
    const auto* found = registry.find("test.id.1");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->value, 10);
    
    auto ids = registry.list_ids();
    EXPECT_EQ(ids.size(), 2);
    // std::map sorts by ID
    EXPECT_EQ(ids[0], "test.id.1");
    EXPECT_EQ(ids[1], "test.id.2");
    
    EXPECT_TRUE(registry.erase("test.id.1"));
    EXPECT_EQ(registry.size(), 1);
    EXPECT_FALSE(registry.contains("test.id.1"));
}

TEST(TypedRegistryTest, ThreadSafety) {
    TypedRegistry<MockSpec> registry;
    
    // Just a basic check that it doesn't crash with multiple threads
    // In a real scenario we'd do more intensive stress testing
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&registry, i]() {
            registry.register_spec({"id." + std::to_string(i), i});
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(registry.size(), 10);
}

} // namespace tachyon::registry::test
