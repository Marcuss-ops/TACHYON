#include "tachyon/runtime/execution/session/telemetry_writer.h"
#include <cstdio>
#include <cstdlib>

namespace tachyon {

bool TelemetryWriter::write_session(const RenderSessionResult& result, const std::string& path) {
    try {
        // Write as tab-separated values
        char line[1024];
        int len = std::snprintf(line, sizeof(line),
            "%zu\t%.1f\t%.1f\t%.1f\t%.1f\t%.1f\t"
            "%zu\t%zu\t%zu\t%.2f\t"
            "%zu\n",
            result.frames.size(),
            result.wall_time_total_ms,
            result.wall_time_per_frame_ms,
            result.scene_compile_ms,
            result.plan_build_ms,
            result.encode_ms,
            result.frames_written,
            result.cache_hits,
            result.cache_misses,
            result.cache_hit_rate(),
            result.peak_memory_bytes
        );
        
        if (len <= 0) return false;
        
        std::FILE* f = std::fopen(path.c_str(), "a");
        if (!f) return false;
        std::fwrite(line, 1, len, f);
        std::fclose(f);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace tachyon
