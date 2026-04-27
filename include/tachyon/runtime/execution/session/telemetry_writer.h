#pragma once

#include "tachyon/runtime/execution/session/render_session.h"
#include <string>
#include <filesystem>

namespace tachyon {

/**
 * @brief Writes render telemetry to a JSONL file.
 * 
 * Each line is a complete JSON object with the session's telemetry.
 * File location: ~/.tachyon/telemetry.jsonl
 */
class TelemetryWriter {
public:
    static std::string get_default_path() {
        const char* home = std::getenv("HOME");
        if (!home) home = std::getenv("USERPROFILE");  // Windows
        if (!home) return "telemetry.jsonl";
        
        std::filesystem::path path(home);
        path /= ".tachyon";
        std::filesystem::create_directories(path);
        path /= "telemetry.jsonl";
        return path.string();
    }
    
    static bool write_session(const RenderSessionResult& result, 
                               const std::string& path = get_default_path()) {
        try {
            nlohmann::json j;
            j["scene_id"] = result.scene_id;
            j["composition_id"] = result.composition_id;
            j["duration_seconds"] = result.duration_seconds;
            j["frame_count"] = result.frame_count;
            j["width"] = result.width;
            j["height"] = result.height;
            j["fps_target"] = result.fps_target;
            
            j["wall_time_total_ms"] = result.wall_time_total_ms;
            j["wall_time_per_frame_ms"] = result.wall_time_per_frame_ms;
            j["encode_time_ms"] = result.encode_time_ms;
            j["cache_hit_rate"] = result.cache_hit_rate;
            
            j["peak_memory_bytes"] = static_cast<double>(result.peak_memory_bytes);
            j["thread_count"] = result.thread_count;
            j["quality_tier"] = result.quality_tier;
            
            j["timestamp_iso8601"] = result.timestamp_iso8601;
            
            // Append to file
            std::string jsonl = j.dump() + "\n";
            std::FILE* f = std::fopen(path.c_str(), "a");
            if (!f) return false;
            std::fwrite(jsonl.c_str(), 1, jsonl.size(), f);
            std::fclose(f);
            return true;
        } catch (...) {
            return false;
        }
    }
};

} // namespace tachyon
