#include "tachyon/runtime/cache/frame_cache.h"

#include <iostream>
#include <string>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

} // namespace

bool run_frame_cache_tests() {
    using namespace tachyon;

    g_failures = 0;

    FrameCache cache;
    auto fb = std::make_shared<tachyon::renderer2d::Framebuffer>(16, 16);
    fb->clear(tachyon::renderer2d::Color::red());
    
    FrameCacheKey key(12345ULL, "frame:A");
    cache.store_frame(key, fb);

    check_true(cache.lookup_frame(key) != nullptr, "Cache hit with matching key");
    
    FrameCacheKey other_key(67890ULL, "frame:A");
    check_true(cache.lookup_frame(other_key) == nullptr, "Cache miss when hash changes");

    cache.clear();
    check_true(cache.lookup_frame(key) == nullptr, "Clear removes entries");

    return g_failures == 0;
}
