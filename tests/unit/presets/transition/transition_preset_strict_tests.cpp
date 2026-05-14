#include "tachyon/presets/transition/transition_preset_registry.h"
#include <gtest/gtest.h>

namespace tachyon::test {

TEST(TransitionPresetStrictTest, UnknownThrows) {
    presets::TransitionPresetRegistry registry;
    registry::ParameterBag params;

    EXPECT_THROW({
        registry.create("tachyon.transition.fake_missing", params);
    }, std::runtime_error);
}

TEST(TransitionPresetStrictTest, NoneReturnsNoOp) {
    presets::TransitionPresetRegistry registry;
    registry::ParameterBag params;

    auto spec = registry.create("none", params);
    EXPECT_EQ(spec.kind, TransitionKind::None);
    EXPECT_EQ(spec.transition_id, "tachyon.transition.none");
}

} // namespace tachyon::test
