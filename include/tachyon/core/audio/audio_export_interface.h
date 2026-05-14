#pragma once

#include <filesystem>
#include <atomic>
#include <string>

namespace tachyon {
struct RenderPlan;

namespace audio {

/**
 * @brief Interface for audio export services.
 */
class IAudioExporter {
public:
    virtual ~IAudioExporter() = default;
    virtual bool export_plan_audio(const RenderPlan& plan, const std::filesystem::path& output_path, std::atomic<bool>* cancel_flag = nullptr) = 0;
};

/**
 * @brief Checks if a render plan contains any audio tracks.
 * Moved to core as it only depends on the schema.
 */
bool has_any_audio(const RenderPlan& plan);

} // namespace audio
} // namespace tachyon
