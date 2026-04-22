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
    CachedFrame frame{
        FrameCacheEntry{FrameCacheKey{"frame:A"}, "frame A"},
        12345ULL, // scene_hash instead of "scene:v1"
        tachyon::renderer2d::Framebuffer(16, 16),
        {"scene.parameter"}
    };
    frame.frame.clear(tachyon::renderer2d::Color::red());
    cache.store(frame);

    check_true(cache.lookup(FrameCacheKey{"frame:A"}, 12345ULL) != nullptr, "Cache hit with matching scene hash");
    check_true(cache.lookup(FrameCacheKey{"frame:A"}, 67890ULL) == nullptr, "Cache miss when scene hash changes");

    cache.invalidate("scene.parameter");
    check_true(cache.lookup(FrameCacheKey{"frame:A"}, 12345ULL) == nullptr, "Invalidation removes dependent entry");

    return g_failures == 0;
}
