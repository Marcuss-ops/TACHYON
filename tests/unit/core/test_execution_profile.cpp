#include <gtest/gtest.h>
#include "tachyon/core/profiles/execution_profile.h"
#include "tachyon/backends/profile_resolver.h"
#include "tachyon/backends/backend_registry.h"
#include "tachyon/tachyon_build_config.h"

namespace tachyon::core::profiles {

TEST(ExecutionProfileTest, PreviewProfileHasExpectedDefaults) {
    auto p = ExecutionProfile::preview();

    EXPECT_EQ(p.name, "preview");
    EXPECT_EQ(p.width, 1280);
    EXPECT_EQ(p.height, 720);
    EXPECT_DOUBLE_EQ(p.fps, 30.0);
    EXPECT_EQ(p.quality, "draft");
    EXPECT_FALSE(p.jit_optimization);
    EXPECT_TRUE(p.cache_enabled);
}

TEST(ExecutionProfileTest, FinalProfileHasExpectedDefaults) {
    auto p = ExecutionProfile::final();

    EXPECT_EQ(p.name, "final");
    EXPECT_EQ(p.width, 1920);
    EXPECT_EQ(p.height, 1080);
    EXPECT_DOUBLE_EQ(p.fps, 60.0);
    EXPECT_EQ(p.quality, "high");
    EXPECT_TRUE(p.jit_optimization);
    EXPECT_TRUE(p.cache_enabled);
}

TEST(ExecutionProfileResolverTest, ResolvesDefaultVideoEncoderDeterministically) {
    auto profile = ExecutionProfile::preview();
    profile.video_encoder = "default";

    auto& registry = tachyon::backends::BackendRegistry::instance();
    auto resolved = tachyon::backends::resolve_profile(profile, registry);

#if TACHYON_ENABLE_MEDIA
    EXPECT_EQ(resolved.video_encoder_id, "ffmpeg");
#else
    EXPECT_EQ(resolved.video_encoder_id, "none");
#endif
}

} // namespace tachyon::core::profiles
