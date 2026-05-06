#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include "tachyon/runtime/execution/planning/render_plan.h"

namespace tachyon::output {

/**
 * @brief Manages a temporary file that is moved to a final destination on commit.
 * If not committed, the temporary file is deleted on destruction (RAII).
 */
class AtomicOutputGuard {
public:
    explicit AtomicOutputGuard(std::filesystem::path final_destination);
    ~AtomicOutputGuard();

    AtomicOutputGuard(const AtomicOutputGuard&) = delete;
    AtomicOutputGuard& operator=(const AtomicOutputGuard&) = delete;
    AtomicOutputGuard(AtomicOutputGuard&&) noexcept;
    AtomicOutputGuard& operator=(AtomicOutputGuard&&) noexcept;

    const std::filesystem::path& temp_path() const { return m_temp_path; }
    const std::filesystem::path& final_path() const { return m_final_path; }

    /**
     * @brief Commits the temporary file by renaming it to the final destination.
     * Returns true on success.
     */
    bool commit(std::string& out_error);

    /**
     * @brief Aborts the operation and deletes the temporary file immediately.
     */
    void abort();

private:
    std::filesystem::path m_final_path;
    std::filesystem::path m_temp_path;
    bool m_committed{false};
};

/**
 * @brief Tracks temporary files that should be deleted when this object goes out of scope.
 */
class TempFileTracker {
public:
    TempFileTracker() = default;
    ~TempFileTracker();

    void track(std::filesystem::path path);
    void release(const std::filesystem::path& path);
    void cleanup_all();

private:
    std::vector<std::filesystem::path> m_files;
};

/**
 * @brief Audio export strategy for a render session.
 */
struct AudioExportPlan {
    std::filesystem::path path;
    bool is_temporary{false};
};

/**
 * @brief Determines where audio should be exported based on the render plan and output sink.
 */
AudioExportPlan plan_audio_export(const RenderPlan& plan, bool is_video_output);

} // namespace tachyon::output
