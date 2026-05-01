#include "tachyon/media/management/proxy_worker.h"
#include <iostream>
#include <filesystem>

#include "tachyon/core/platform/process.h"

namespace tachyon::media {

ProxyWorker::ProxyWorker(ProxyManifest& manifest)
    : m_manifest(manifest) {
    m_worker_thread = std::thread(&ProxyWorker::worker_loop, this);
}

ProxyWorker::~ProxyWorker() {
    m_running = false;
    m_cv.notify_all();
    if (m_worker_thread.joinable()) {
        m_worker_thread.join();
    }
}

void ProxyWorker::generate_proxies(const std::vector<std::string>& originals, const ProxyPolicy& policy) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& original : originals) {
            m_queue.push({original, policy});
        }
    }
    m_cv.notify_one();
}

void ProxyWorker::cancel_all() {
    std::lock_guard<std::mutex> lock(m_mutex);
    while (!m_queue.empty()) {
        m_queue.pop();
    }
}

void ProxyWorker::worker_loop() {
    while (m_running) {
        Task task;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] { return !m_queue.empty() || !m_running; });
            if (!m_running) break;
            task = std::move(m_queue.front());
            m_queue.pop();
        }

        if (m_progress_callback) {
            m_progress_callback(task.original_path, 0.0f);
        }

        bool success = process_transcode(task.original_path, task.policy);

        if (m_progress_callback) {
            m_progress_callback(task.original_path, success ? 1.0f : -1.0f);
        }

        if (success) {
            ProxyVariant variant;
            variant.original_path = task.original_path;
            variant.proxy_path = (task.policy.output_directory / std::filesystem::path(task.original_path).filename().replace_extension(".mp4")).string();
            variant.width = task.policy.target_width;
            variant.height = task.policy.target_height;
            variant.codec = task.policy.codec;
            variant.bitrate_mbps = task.policy.bitrate_mbps;
            m_manifest.register_proxy(variant);
        }
    }
}

bool ProxyWorker::process_transcode(const std::string& original, const ProxyPolicy& policy) {
    // Use FFmpeg command-line tool for proxy generation
    std::filesystem::path input_path(original);
    std::filesystem::path output_path = policy.output_directory / input_path.filename().replace_extension(".mp4");

    // Ensure output directory exists
    if (!policy.output_directory.empty()) {
        std::filesystem::create_directories(policy.output_directory);
    }

    // Build FFmpeg command
    std::string command = "ffmpeg -y -i \"" + original + "\"";
    command += " -vf scale=" + std::to_string(policy.target_width) + ":" + std::to_string(policy.target_height);
    command += " -c:v libx264 -b:v " + std::to_string(static_cast<int>(policy.bitrate_mbps * 1000000)) + " -preset fast";
    command += " -c:a aac -b:a 128000";
    command += " \"" + output_path.string() + "\"";

    using namespace tachyon::core::platform;
    ProcessSpec spec;
#ifdef _WIN32
    spec.executable = "cmd.exe";
    spec.args = {"/C", command};
#else
    spec.executable = "sh";
    spec.args = {"-c", command};
#endif

    auto proc_result = run_process(spec);
    return proc_result.success && proc_result.exit_code == 0;
}

} // namespace tachyon::media
