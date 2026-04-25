#include "tachyon/text/fonts/font.h"
#include "tachyon/text/layout/layout.h"
#include "tachyon/text/layout/layout_cache.h"
#include "tachyon/text/layout/cluster_iterator.h"
#include "tachyon/text/i18n/bidi_engine.h"

#include <iostream>
#include <vector>
#include <string>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    } else {
        std::cout << "PASS: " << message << '\n';
    }
}

void test_layout_cache() {
    using namespace tachyon::text;
    std::cout << "Testing LayoutCache...\n";

    LayoutCache& cache = LayoutCache::get_instance();
    cache.clear();

    LayoutCacheKey key;
    key.text = "Hello Cache";
    key.font_id = 1;
    key.pixel_size = 24;
    key.alignment = TextAlignment::Left;
    key.tracking = 0.0f;
    key.word_wrap = true;
    key.multiline = true;
    key.box_width = 0;
    key.box_height = 0;
    key.use_sdf = false;

    TextLayoutResult result;
    result.width = 100;
    result.height = 30;

    cache.put(key, result);

    TextLayoutResult cached_result;
    check_true(cache.get(key, cached_result), "LayoutCache retrieval");
    check_true(cached_result.width == 100, "LayoutCache width match");
    check_true(cached_result.height == 30, "LayoutCache height match");

    // Test mismatch
    key.text = "Different Text";
    check_true(!cache.get(key, cached_result), "LayoutCache miss on different text");
}

void test_cluster_iterator() {
    using namespace tachyon::text;

    // Test 1: Basic ASCII
    {
        std::vector<std::uint32_t> cp = {'H', 'e', 'l', 'l', 'o'};
        auto clusters = ClusterIterator::segment(cp);
        check_true(clusters.size() == 5, "ASCII segmenting");
    }

    // Test 2: Combining Marks
    {
        // 'a' + acute accent (U+0301)
        std::vector<std::uint32_t> cp = {'a', 0x0301, 'b'};
        auto clusters = ClusterIterator::segment(cp);
        check_true(clusters.size() == 2, "Combining marks segmenting");
        if (clusters.size() >= 1) {
            check_true(clusters[0].length == 2, "Cluster with combining mark has length 2");
        }
    }

    // Test 3: ZWJ
    {
        // Emoji ZWJ sequence
        std::vector<std::uint32_t> cp = {0x1F468, 0x200D, 0x1F469};
        auto clusters = ClusterIterator::segment(cp);
        check_true(clusters.size() == 1, "ZWJ sequence segmenting");
    }
}

void test_bidi_paragraphs() {
    using namespace tachyon::text;

    // Test 1: Multiple paragraphs
    {
        std::vector<std::uint32_t> cp = {'A', '\n', 0x05D0}; // Latin 'A', newline, Hebrew Aleph
        auto runs = BidiEngine::analyze(cp);
        check_true(runs.size() >= 2, "Multiple paragraphs detected");
        
        bool found_rtl = false;
        bool found_ltr = false;
        for (const auto& run : runs) {
            if (run.direction == CharacterDirection::RTL) found_rtl = true;
            if (run.direction == CharacterDirection::LTR) found_ltr = true;
        }
        check_true(found_ltr, "LTR paragraph found");
        check_true(found_rtl, "RTL paragraph found");
    }
}

} // namespace

int main() {
    std::cout << "Running Phase 1 Text Foundation Tests...\n";
    
    test_layout_cache();
    test_cluster_iterator();
    test_bidi_paragraphs();

    if (g_failures == 0) {
        std::cout << "All Phase 1 tests passed!\n";
        return 0;
    } else {
        std::cout << g_failures << " tests failed.\n";
        return 1;
    }
}
