#include <gtest/gtest.h>
#include "tachyon/media/management/proxy_worker.h"
#include "tachyon/media/management/proxy_manifest.h"
#include "tachyon/media/management/media_manager.h"
#include <filesystem>
#include <thread>
#include <chrono>

using namespace tachyon::media;

class ProxyWorkerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary directory for proxies
        m_temp_dir = std::filesystem::temp_directory_path() / "tachyon_proxy_test";
        if (std::filesystem::exists(m_temp_dir)) {
            std::filesystem::remove_all(m_temp_dir);
        }
        std::filesystem::create_directories(m_temp_dir);
    }

    void TearDown() override {
        std::filesystem::remove_all(m_temp_dir);
    }

    std::filesystem::path m_temp_dir;
};

TEST_F(ProxyWorkerTest, QueuesAndGeneratesProxy) {
    ProxyManifest manifest;
    ProxyWorker worker(manifest);

    ProxyPolicy policy;
    policy.target_width = 640;
    policy.target_height = 360;
    policy.output_directory = m_temp_dir;

    std::string original = "test_video_source.mp4";
    worker.generate_proxies({original}, policy);

    // Wait for the background worker (since we have a placeholder that "completes" immediately)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string resolved = manifest.resolve_for_playback(original, 640);
    EXPECT_NE(resolved, original);
    EXPECT_TRUE(resolved.find("_proxy_640.mp4") != std::string::npos);
}

TEST_F(ProxyWorkerTest, MediaManagerIntegration) {
    MediaManager manager;
    
    ProxyPolicy policy;
    policy.target_width = 1280;
    policy.output_directory = m_temp_dir;

    std::string original = "test_video.mp4";
    manager.proxy_worker().generate_proxies({original}, policy);

    // Wait for worker
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::filesystem::path resolved = manager.resolve_media_path(original);
    EXPECT_NE(resolved.string(), original);
    EXPECT_TRUE(resolved.string().find("_proxy_1280.mp4") != std::string::npos);
}
