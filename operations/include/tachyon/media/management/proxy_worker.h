#pragma once

#include "tachyon/media/management/proxy_manifest.h"
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <filesystem>

namespace tachyon::media {

/**
 * @brief Configuration for proxy generation.
 */
struct ProxyPolicy {
    int target_width{1280};
    int target_height{720};
    std::string codec{"h264"}; ///< "h264", "prores_proxy", etc.
    float bitrate_mbps{5.0f};
    std::filesystem::path output_directory;
};

/**
 * @brief Handles asynchronous generation of proxy assets.
 */
class ProxyWorker {
public:
    ProxyWorker(ProxyManifest& manifest);
    ~ProxyWorker();

    /**
     * @brief Queue a list of original files for proxy generation.
     */
    void generate_proxies(const std::vector<std::string>& originals, const ProxyPolicy& policy);

    /**
     * @brief Cancel all pending proxy tasks.
     */
    void cancel_all();

    /**
     * @brief Callback for completion/progress updates.
     */
    using ProgressCallback = std::function<void(const std::string& original, float progress)>;
    void set_progress_callback(ProgressCallback callback) { m_progress_callback = std::move(callback); }

private:
    void worker_loop();
    bool process_transcode(const std::string& original, const ProxyPolicy& policy);

    struct Task {
        std::string original_path;
        ProxyPolicy policy;
    };

    ProxyManifest& m_manifest;
    std::queue<Task> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_running{true};
    std::thread m_worker_thread;
    ProgressCallback m_progress_callback;
};

} // namespace tachyon::media
