#include "tachyon/core/spec/authoring_service.h"
#include <gtest/gtest.h>

namespace tachyon::test {

TEST(AuthoringServiceCacheKeyTest, HashChangesWithSource) {
    std::string source_a = "void build_scene() { /* A */ }";
    std::string source_b = "void build_scene() { /* B */ }";
    
    auto hash_a = core::spec::AuthoringService::compute_cache_hash(source_a);
    auto hash_b = core::spec::AuthoringService::compute_cache_hash(source_b);
    
    EXPECT_NE(hash_a, hash_b);
}

TEST(AuthoringServiceCacheKeyTest, HashIsStable) {
    std::string source = "void build_scene() { /* stable */ }";
    
    auto hash_a = core::spec::AuthoringService::compute_cache_hash(source);
    auto hash_b = core::spec::AuthoringService::compute_cache_hash(source);
    
    EXPECT_EQ(hash_a, hash_b);
}

} // namespace tachyon::test
