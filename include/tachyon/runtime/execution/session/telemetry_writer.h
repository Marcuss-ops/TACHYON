#pragma once

#include "tachyon/runtime/execution/session/render_session.h"
#include <string>
#include <filesystem>

namespace tachyon {

/**
 * @brief Writes render telemetry to a file.
 * 
 * Each line contains tab-separated values: scene_id, composition_id, duration, frames, etc.
 * File location: ~/.tachyon/telemetry.tsv
 */
class TelemetryWriter {
public:
    static std::string get_default_path() {
        const char* home = std::getenv("HOME");
        if (!home) home = std::getenv("USERPROFILE");  // Windows
        if (!home) return "telemetry.tsv";
        
        std::filesystem::path path(home);
        path /= ".tachyon";
        if (!std::filesystem::exists(path)) {
            std::filesystem::create_directories(path);
        }
        path /= "telemetry.tsv";
        return path.string();
    }
    
    static bool write_session(const RenderSessionResult& result, 
                               const std::string& path = get_default_path());
};

} // namespace tachyon
