#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include <gtest/gtest.h>

namespace tachyon::test {

TEST(LayerTimingTest, CanonicalEvaluation) {
    LayerSpec layer;
    layer.timing.start = 2.0;
    layer.timing.duration = 3.0;
    layer.timing.source_in = 1.0;
    layer.timing.source_out = 4.0;

    auto is_active = [](const LayerSpec& l, double t) {
        return t >= l.timing.start && t < (l.timing.start + l.timing.duration);
    };

    EXPECT_FALSE(is_active(layer, 1.99));
    EXPECT_TRUE(is_active(layer, 2.00));
    EXPECT_TRUE(is_active(layer, 4.99));
    EXPECT_FALSE(is_active(layer, 5.00));
}

} // namespace tachyon::test
