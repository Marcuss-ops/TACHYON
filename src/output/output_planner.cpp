#include "tachyon/output/output_planner.h"
#include "tachyon/audio/audio_export.h"
#include "tachyon/output/output_utils.h"
#include <system_error>

namespace tachyon::output {

// ---------------------------------------------------------------------------
// AtomicOutputGuard
// ---------------------------------------------------------------------------

AtomicOutputGuard::AtomicOutputGuard(std::filesystem::path final_destination)
    : m_final_path(std::move(final_destination)) {
    if (!m_final_path.empty()) {
        m_temp_path = m_final_path.string() + ".tmp";
    }
}

AtomicOutputGuard::~AtomicOutputGuard() {
    if (!m_committed && !m_temp_path.empty()) {
        std::error_code ec;
        std::filesystem::remove(m_temp_path, ec);
    }
}

AtomicOutputGuard::AtomicOutputGuard(AtomicOutputGuard&& other) noexcept
    : m_final_path(std::move(other.m_final_path)),
      m_temp_path(std::move(other.m_temp_path)),
      m_committed(other.m_committed) {
    other.m_committed = true; // prevent deletion from moved-from object
}

AtomicOutputGuard& AtomicOutputGuard::operator=(AtomicOutputGuard&& other) noexcept {
    if (this != &other) {
        if (!m_committed && !m_temp_path.empty()) {
            std::error_code ec;
            std::filesystem::remove(m_temp_path, ec);
        }
        m_final_path = std::move(other.m_final_path);
        m_temp_path = std::move(other.m_temp_path);
        m_committed = other.m_committed;
        other.m_committed = true;
    }
    return *this;
}

bool AtomicOutputGuard::commit(std::string& out_error) {
    if (m_committed) return true;
    if (m_temp_path.empty() || m_final_path.empty()) return false;

    std::error_code ec;
    
    // Ensure parent directory exists
    if (m_final_path.has_parent_path()) {
        std::filesystem::create_directories(m_final_path.parent_path(), ec);
    }

    std::filesystem::rename(m_temp_path, m_final_path, ec);
    if (ec) {
        // Fallback for cross-device move
        std::filesystem::copy(m_temp_path, m_final_path, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            out_error = "failed to commit output: " + ec.message();
            return false;
        }
        std::filesystem::remove(m_temp_path, ec);
    }

    m_committed = true;
    return true;
}

void AtomicOutputGuard::abort() {
    if (!m_committed && !m_temp_path.empty()) {
        std::error_code ec;
        std::filesystem::remove(m_temp_path, ec);
        m_committed = true;
    }
}

// ---------------------------------------------------------------------------
// TempFileTracker
// ---------------------------------------------------------------------------

TempFileTracker::~TempFileTracker() {
    cleanup_all();
}

void TempFileTracker::track(std::filesystem::path path) {
    if (!path.empty()) {
        m_files.push_back(std::move(path));
    }
}

void TempFileTracker::release(const std::filesystem::path& path) {
    m_files.erase(std::remove(m_files.begin(), m_files.end(), path), m_files.end());
}

void TempFileTracker::cleanup_all() {
    std::error_code ec;
    for (const auto& path : m_files) {
        std::filesystem::remove(path, ec);
    }
    m_files.clear();
}

// ---------------------------------------------------------------------------
// AudioExportPlan
// ---------------------------------------------------------------------------

AudioExportPlan plan_audio_export(const RenderPlan& plan, bool is_video_output) {
    AudioExportPlan result;
    
    if (!audio::has_any_audio(plan)) {
        return result;
    }

    const std::string final_dest = plan.output.destination.path;
    if (final_dest.empty()) return result;

    const std::filesystem::path final_p(final_dest);

    if (is_video_output) {
        // Temp audio for muxing
        result.path = final_p.parent_path() / (final_p.stem().string() + ".temp.wav");
        result.is_temporary = true;
    } else {
        // Sidecar audio for image sequence
        result.path = final_p.parent_path() / (final_p.stem().string() + ".wav");
        result.is_temporary = false;
    }

    return result;
}

} // namespace tachyon::output
