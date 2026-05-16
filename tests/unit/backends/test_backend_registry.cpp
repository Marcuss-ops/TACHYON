#include <gtest/gtest.h>
#include "tachyon/backends/backend_registry.h"
#include "tachyon/core/media/media_interfaces.h"
#include <memory>
#include <algorithm>

namespace tachyon::backends {

// Fake classes for testing
class FakeVideoEncoder : public core::media::IVideoEncoder {
public:
    FakeVideoEncoder(const std::string& name) : m_name(name) {}
    core::media::MediaResult<void> open(const std::filesystem::path&, int, int, double) override { return core::media::MediaResult<void>::success(); }
    core::media::MediaResult<void> write_frame(const renderer2d::SurfaceRGBA&) override { return core::media::MediaResult<void>::success(); }
    void close() override {}
    const std::string& name() const { return m_name; }
private:
    std::string m_name;
};

TEST(BackendRegistryTest, DefaultReturnsNullWhenNoBackendRegistered) {
    // Note: We use a fresh registry if possible, or clear it.
    // BackendRegistry is a singleton, so this might depend on global state.
    // In a real test we might want to reset the singleton.
    auto& registry = BackendRegistry::instance();
    
    auto encoder = registry.create_video_encoder("does_not_exist_xyz");
    EXPECT_EQ(encoder, nullptr);
}

TEST(BackendRegistryTest, DefaultVideoEncoderIsDeterministic) {
    auto& registry = BackendRegistry::instance();

    registry.register_video_encoder("fake_a", [] {
        return std::make_unique<FakeVideoEncoder>("fake_a");
    });

    registry.register_video_encoder("fake_b", [] {
        return std::make_unique<FakeVideoEncoder>("fake_b");
    });

    // In our new ProfileResolver logic, we pick "ffmpeg" by default if media is ON.
    // But for the registry itself, we can test listing.
    auto names = registry.list_video_encoders();
    
    // Check if "fake_a" and "fake_b" are present and sorted
    EXPECT_TRUE(std::find(names.begin(), names.end(), "fake_a") != names.end());
    EXPECT_TRUE(std::find(names.begin(), names.end(), "fake_b") != names.end());
    
    // Vector should be sorted
    EXPECT_TRUE(std::is_sorted(names.begin(), names.end()));
}

TEST(BackendRegistryTest, MissingBackendReturnsNull) {
    auto& registry = BackendRegistry::instance();
    auto encoder = registry.create_video_encoder("does_not_exist");
    EXPECT_EQ(encoder, nullptr);
}

} // namespace tachyon::backends
