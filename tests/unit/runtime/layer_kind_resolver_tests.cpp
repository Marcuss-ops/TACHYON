#include "test_utils.h"
#include "tachyon/runtime/core/graph/layer_kind_resolver.h"
#include <iostream>
#include <cassert>

using namespace tachyon;

bool run_layer_kind_resolver_tests() {
    std::cout << "[LayerKindResolverTests] Starting LayerKindResolver unit tests...\n";

    // Test 1: Resolve known types
    {
        assert(LayerKindResolver::resolve(1) == LayerType::Solid);
        assert(LayerKindResolver::resolve(2) == LayerType::Shape);
        assert(LayerKindResolver::resolve(3) == LayerType::Image);
        assert(LayerKindResolver::resolve(4) == LayerType::Text);
        assert(LayerKindResolver::resolve(5) == LayerType::Precomp);
        assert(LayerKindResolver::resolve(6) == LayerType::Procedural);
        assert(LayerKindResolver::resolve(8) == LayerType::Video);
        assert(LayerKindResolver::resolve(11) == LayerType::Mask);
        assert(LayerKindResolver::resolve(12) == LayerType::NullLayer);
        
        std::cout << "[LayerKindResolverTests] Known type resolution verified!\n";
    }

    // Test 2: Fallback for unknown types
    {
        assert(LayerKindResolver::resolve(999) == LayerType::NullLayer);
        assert(LayerKindResolver::resolve(0) == LayerType::NullLayer);
        
        std::cout << "[LayerKindResolverTests] Unknown type fallback verified!\n";
    }

    std::cout << "[LayerKindResolverTests] ALL TESTS PASSED!\n";
    return true;
}
