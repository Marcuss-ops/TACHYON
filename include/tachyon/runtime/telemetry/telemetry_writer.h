#pragma once

#include "tachyon/runtime/telemetry/render_telemetry_record.h"
#include "tachyon/runtime/telemetry/batch_telemetry_aggregator.h"
#include "tachyon/api.h"
#include <string>
#include <filesystem>
#include <vector>
#include <mutex>

namespace tachyon {

struct RenderSessionResult;

/**
 * @brief Modern telemetry writer supporting JSONL, TSV, and SQLite formats.
 */
class TACHYON_API TelemetryWriter {
public:
    static std::filesystem::path get_default_directory();
    
    /**
     * @brief Writes a single job record as a JSON line (appends to file).
     */
    static bool append_jsonl(const RenderTelemetryRecord& record, const std::filesystem::path& path);
    
    /**
     * @brief Writes a single job record as a TSV line (appends to file).
     */
    static bool append_tsv(const RenderTelemetryRecord& record, const std::filesystem::path& path);

    /**
     * @brief Writes a batch summary as a JSON file (overwrites).
     */
    static bool write_batch_summary(const BatchTelemetrySummary& summary, const std::filesystem::path& path);

    /**
     * @brief Persists comprehensive session statistics, frame-by-frame latencies, phase breakdowns, and counters to SQLite.
     */
    static bool write_sqlite(const RenderTelemetryRecord& record, const RenderSessionResult& session_result);

private:
    static std::mutex s_write_mutex;
};

} // namespace tachyon

