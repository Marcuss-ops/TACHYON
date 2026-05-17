#pragma once

#include "tachyon/api.h"
#include "tachyon/runtime/telemetry/telemetry_store.h"
#include <vector>
#include <string>

// Forward declare raw SQLite connection pointer to keep compile times clean
struct sqlite3;

namespace tachyon {

/**
 * @brief Telemetry record for an individual frame's rendering and output metrics.
 */
struct TACHYON_API SqliteFrameRecord {
    int frame_number{0};
    double duration_ms{0.0};
    double encode_time_ms{0.0};
    double write_time_ms{0.0};
    bool cache_hit{false};
};

/**
 * @brief Telemetry record for execution latencies of core rendering phases.
 */
struct TACHYON_API SqlitePhaseEventRecord {
    std::string phase_name;
    double duration_ms{0.0};
};

/**
 * @brief Telemetry record for engine optimization counters.
 */
struct TACHYON_API SqliteCounterRecord {
    std::string counter_name;
    int64_t counter_value{0};
};

/**
 * @brief Concrete SQLite-backed implementation of the TelemetryStore interface.
 */
class TACHYON_API SqliteTelemetryStore final : public TelemetryStore {
public:
    SqliteTelemetryStore();
    ~SqliteTelemetryStore() override;

    // Prevent copy semantics
    SqliteTelemetryStore(const SqliteTelemetryStore&) = delete;
    SqliteTelemetryStore& operator=(const SqliteTelemetryStore&) = delete;

    // Support moving
    SqliteTelemetryStore(SqliteTelemetryStore&& other) noexcept;
    SqliteTelemetryStore& operator=(SqliteTelemetryStore&& other) noexcept;

    bool initialize(const std::filesystem::path& db_path) override;
    bool write_render_record(const RenderTelemetryRecord& record) override;

    /**
     * @brief Persist frame-by-frame rendering metrics in a single transaction batch.
     */
    bool write_frame_records(const std::string& run_id, const std::vector<SqliteFrameRecord>& frames);

    /**
     * @brief Persist pipeline phase events in a single transaction batch.
     */
    bool write_phase_events(const std::string& run_id, const std::vector<SqlitePhaseEventRecord>& events);

    /**
     * @brief Persist engine optimization counters in a single transaction batch.
     */
    bool write_counters(const std::string& run_id, const std::vector<SqliteCounterRecord>& counters);

    /**
     * @brief Checks whether a valid connection to SQLite is active.
     */
    bool is_open() const;

private:
    struct sqlite3* m_db{nullptr};
    
    // Internal schema initialization helper
    bool setup_schema();
};

} // namespace tachyon
