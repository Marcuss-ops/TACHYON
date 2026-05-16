#include <gtest/gtest.h>
#include "tachyon/backends/backend_init.h"
#include "tachyon/backends/backend_registry.h"
#include "tachyon/tachyon_build_config.h"
#include <algorithm>

namespace tachyon::backends {

TEST(BackendInitTest, RegistersExpectedBackends) {
    initialize_all_backends();
    auto& registry = BackendRegistry::instance();

#if TACHYON_ENABLE_MEDIA
    {
        auto names = registry.list_video_encoders();
        EXPECT_TRUE(std::find(names.begin(), names.end(), "ffmpeg") != names.end());
    }
#endif

#if TACHYON_ENABLE_WHISPER
    {
        auto names = registry.list_audio_analyzers();
        EXPECT_TRUE(std::find(names.begin(), names.end(), "whisper") != names.end());
    }
#endif

#if !TACHYON_ENABLE_WHISPER
    {
        auto names = registry.list_audio_analyzers();
        EXPECT_TRUE(std::find(names.begin(), names.end(), "whisper") == names.end());
    }
#endif
}

} // namespace tachyon::backends
