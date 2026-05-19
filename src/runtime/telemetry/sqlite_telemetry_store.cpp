#include "tachyon/runtime/telemetry/sqlite_telemetry_store.h"
#include <filesystem>
#include <iostream>

#ifdef TACHYON_ENABLE_SQLITE_TELEMETRY
#include <sqlite3.h>
#endif

namespace tachyon {

#ifdef TACHYON_ENABLE_SQLITE_TELEMETRY

// Internal helper for transaction scopes
class SqliteTransaction {
public:
    explicit SqliteTransaction(sqlite3* db) : m_db(db) {
        if (m_db) {
            sqlite3_exec(m_db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
        }
    }
    ~SqliteTransaction() {
        if (m_db && !m_committed) {
            sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
        }
    }
    void commit() {
        if (m_db && !m_committed) {
            sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr);
            m_committed = true;
        }
    }
private:
    sqlite3* m_db{nullptr};
    bool m_committed{false};
};

SqliteTelemetryStore::SqliteTelemetryStore() : m_db(nullptr) {}

SqliteTelemetryStore::~SqliteTelemetryStore() {
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

SqliteTelemetryStore::SqliteTelemetryStore(SqliteTelemetryStore&& other) noexcept : m_db(other.m_db) {
    other.m_db = nullptr;
}

SqliteTelemetryStore& SqliteTelemetryStore::operator=(SqliteTelemetryStore&& other) noexcept {
    if (this != &other) {
        if (m_db) {
            sqlite3_close(m_db);
        }
        m_db = other.m_db;
        other.m_db = nullptr;
    }
    return *this;
}

bool SqliteTelemetryStore::is_open() const {
    return m_db != nullptr;
}

bool SqliteTelemetryStore::initialize(const std::filesystem::path& db_path) {
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }

    if (db_path != ":memory:") {
        std::filesystem::path parent = db_path.parent_path();
        if (!parent.empty() && !std::filesystem::exists(parent)) {
            std::error_code ec;
            std::filesystem::create_directories(parent, ec);
            if (ec) {
                std::cerr << "[SqliteTelemetryStore] Failed to create parent directory: " 
                          << ec.message() << std::endl;
                return false;
            }
        }
    }

    int rc = sqlite3_open(db_path.string().c_str(), &m_db);
    if (rc != SQLITE_OK) {
        std::cerr << "[SqliteTelemetryStore] Failed to open SQLite database: " 
                  << (m_db ? sqlite3_errmsg(m_db) : "Unknown error") << std::endl;
        if (m_db) {
            sqlite3_close(m_db);
            m_db = nullptr;
        }
        return false;
    }

    // Enable WAL mode + synchronous=NORMAL for concurrent write performance
    sqlite3_exec(m_db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(m_db, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr);

    return setup_schema();
}

bool SqliteTelemetryStore::setup_schema() {
    if (!m_db) return false;

    // 1. Schema Version Check via user_version
    sqlite3_stmt* version_stmt = nullptr;
    int current_version = 0;
    int rc = sqlite3_prepare_v2(m_db, "PRAGMA user_version;", -1, &version_stmt, nullptr);
    if (rc == SQLITE_OK) {
        if (sqlite3_step(version_stmt) == SQLITE_ROW) {
            current_version = sqlite3_column_int(version_stmt, 0);
        }
        sqlite3_finalize(version_stmt);
    }

    // 2. Migration Flow
    if (current_version < 1) {
        const char* SCHEMA_SQL = R"(
            CREATE TABLE IF NOT EXISTS render_runs (
                run_id TEXT PRIMARY KEY,
                job_id TEXT,
                scene_id TEXT,
                preset_id TEXT,
                machine_id TEXT,
                success INTEGER,
                error_code TEXT,
                error_message TEXT,
                frames_total INTEGER,
                frames_written INTEGER,
                wall_time_ms REAL,
                render_ms REAL,
                encode_ms REAL,
                effective_fps REAL,
                peak_working_set_bytes INTEGER,
                avg_working_set_bytes INTEGER,
                peak_private_bytes INTEGER,
                avg_private_bytes INTEGER,
                avg_cpu_percent_machine REAL,
                avg_cpu_cores_used REAL,
                output_path TEXT,
                finished_at_iso TEXT
            );

            CREATE TABLE IF NOT EXISTS render_frames (
                run_id TEXT,
                frame_number INTEGER,
                duration_ms REAL,
                encode_time_ms REAL,
                write_time_ms REAL,
                cache_hit INTEGER,
                PRIMARY KEY (run_id, frame_number)
            );

            CREATE TABLE IF NOT EXISTS render_phase_events (
                run_id TEXT,
                phase_name TEXT,
                duration_ms REAL,
                PRIMARY KEY (run_id, phase_name)
            );

            CREATE TABLE IF NOT EXISTS render_counters (
                run_id TEXT,
                counter_name TEXT,
                counter_value INTEGER,
                PRIMARY KEY (run_id, counter_name)
            );
        )";

        char* errMsg = nullptr;
        rc = sqlite3_exec(m_db, SCHEMA_SQL, nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK) {
            std::cerr << "[SqliteTelemetryStore] Schema setup failed: " 
                      << (errMsg ? errMsg : "Unknown error") << std::endl;
            if (errMsg) sqlite3_free(errMsg);
            return false;
        }

        sqlite3_exec(m_db, "PRAGMA user_version = 1;", nullptr, nullptr, nullptr);
        current_version = 1;
    }



    if (current_version < 2) {
        // Upgrade DB to version 2
        sqlite3_exec(m_db, "ALTER TABLE render_runs ADD COLUMN trace_id TEXT NOT NULL DEFAULT '';", nullptr, nullptr, nullptr);
        sqlite3_exec(m_db, "ALTER TABLE render_runs ADD COLUMN parent_span_id TEXT NOT NULL DEFAULT '';", nullptr, nullptr, nullptr);
        sqlite3_exec(m_db, "ALTER TABLE render_runs ADD COLUMN frame_time_hist TEXT NOT NULL DEFAULT '';", nullptr, nullptr, nullptr);

        sqlite3_exec(m_db, "PRAGMA user_version = 2;", nullptr, nullptr, nullptr);
    }

    if (current_version < 3) {
        // Individual alters to be highly robust and avoid multi-statement sqlite3_exec friction
        const std::vector<std::string> ALTERS = {
            "ALTER TABLE render_runs ADD COLUMN physical_cores INTEGER NOT NULL DEFAULT 0;",
            "ALTER TABLE render_runs ADD COLUMN cpu_freq_mhz REAL NOT NULL DEFAULT 0.0;",
            "ALTER TABLE render_runs ADD COLUMN gpu_vendor TEXT NOT NULL DEFAULT '';",
            "ALTER TABLE render_runs ADD COLUMN gpu_driver TEXT NOT NULL DEFAULT '';",
            "ALTER TABLE render_runs ADD COLUMN total_ram_bytes INTEGER NOT NULL DEFAULT 0;",
            "ALTER TABLE render_runs ADD COLUMN total_vram_bytes INTEGER NOT NULL DEFAULT 0;",
            "ALTER TABLE render_runs ADD COLUMN git_commit_short TEXT NOT NULL DEFAULT '';",
            "ALTER TABLE render_runs ADD COLUMN build_type TEXT NOT NULL DEFAULT '';",
            "ALTER TABLE render_runs ADD COLUMN compiler_info TEXT NOT NULL DEFAULT '';",
            "ALTER TABLE render_runs ADD COLUMN exit_code INTEGER NOT NULL DEFAULT 0;",
            "ALTER TABLE render_runs ADD COLUMN error_category TEXT NOT NULL DEFAULT '';",
            "ALTER TABLE render_runs ADD COLUMN total_pixels_processed INTEGER NOT NULL DEFAULT 0;",
            "ALTER TABLE render_runs ADD COLUMN total_tiles INTEGER NOT NULL DEFAULT 0;",
            "ALTER TABLE render_runs ADD COLUMN memory_samples TEXT NOT NULL DEFAULT '';",
            "ALTER TABLE render_runs ADD COLUMN cpu_util_samples TEXT NOT NULL DEFAULT '';",
            "ALTER TABLE render_runs ADD COLUMN gpu_util_samples TEXT NOT NULL DEFAULT '';",
            "ALTER TABLE render_runs ADD COLUMN preset_json TEXT NOT NULL DEFAULT '';",
            "ALTER TABLE render_runs ADD COLUMN time_to_first_frame_ms REAL NOT NULL DEFAULT 0.0;",
            "ALTER TABLE render_runs ADD COLUMN ffmpeg_queue_depth INTEGER NOT NULL DEFAULT 0;",
            "ALTER TABLE render_runs ADD COLUMN hostname TEXT NOT NULL DEFAULT '';",
            "ALTER TABLE render_runs ADD COLUMN os TEXT NOT NULL DEFAULT '';",
            "ALTER TABLE render_runs ADD COLUMN cpu_model TEXT NOT NULL DEFAULT '';",
            "ALTER TABLE render_runs ADD COLUMN logical_cores INTEGER NOT NULL DEFAULT 0;",
            "ALTER TABLE render_runs ADD COLUMN tachyon_version TEXT NOT NULL DEFAULT '';"
        };

        for (const auto& sql : ALTERS) {
            char* errMsg = nullptr;
            sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errMsg);
            if (errMsg) {
                // Column might already exist, which is fine
                sqlite3_free(errMsg);
            }
        }

        sqlite3_exec(m_db, "PRAGMA user_version = 3;", nullptr, nullptr, nullptr);
    }

    if (current_version < 4) {
        // Upgrade DB to version 4 - Detailed pixel telemetry
        const std::vector<std::string> ALTERS_V4 = {
            "ALTER TABLE render_runs ADD COLUMN rasterized_pixels INTEGER NOT NULL DEFAULT 0;",
            "ALTER TABLE render_runs ADD COLUMN blend_pixel_ops INTEGER NOT NULL DEFAULT 0;",
            "ALTER TABLE render_runs ADD COLUMN encoded_pixels INTEGER NOT NULL DEFAULT 0;"
        };

        for (const auto& sql : ALTERS_V4) {
            char* errMsg = nullptr;
            sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errMsg);
            if (errMsg) {
                sqlite3_free(errMsg);
            }
        }

        sqlite3_exec(m_db, "PRAGMA user_version = 4;", nullptr, nullptr, nullptr);
    }

    if (current_version < 5) {
        // Upgrade DB to version 5 - Invalidation metrics
        char* errMsg = nullptr;
        sqlite3_exec(m_db, "ALTER TABLE render_frames ADD COLUMN dirty_area_ratio REAL NOT NULL DEFAULT 1.0;", nullptr, nullptr, &errMsg);
        if (errMsg) sqlite3_free(errMsg);

        sqlite3_exec(m_db, "PRAGMA user_version = 5;", nullptr, nullptr, nullptr);
    }

    return true;
}

bool SqliteTelemetryStore::write_render_record(const RenderTelemetryRecord& record) {
    if (!m_db) return false;

    const char* INSERT_RUN_SQL = R"(
        INSERT OR REPLACE INTO render_runs (
            run_id, job_id, scene_id, preset_id, machine_id, trace_id, parent_span_id, frame_time_hist,
            success, error_code, error_message, frames_total, frames_written, wall_time_ms, render_ms,
            encode_ms, effective_fps, peak_working_set_bytes, avg_working_set_bytes, peak_private_bytes,
            avg_private_bytes, avg_cpu_percent_machine, avg_cpu_cores_used, output_path, finished_at_iso,
            physical_cores, cpu_freq_mhz, gpu_vendor, gpu_driver, total_ram_bytes, total_vram_bytes,
            git_commit_short, build_type, compiler_info, exit_code, error_category,
            total_pixels_processed, total_tiles, rasterized_pixels, blend_pixel_ops, encoded_pixels,
            memory_samples, cpu_util_samples, gpu_util_samples,
            preset_json, time_to_first_frame_ms, ffmpeg_queue_depth,
            hostname, os, cpu_model, logical_cores, tachyon_version
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?,
                  ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?,
                  ?, ?, ?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, INSERT_RUN_SQL, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[SqliteTelemetryStore] Prepare statement failed: " 
                  << sqlite3_errmsg(m_db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, record.run_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, record.job_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, record.scene_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, record.preset_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, record.machine_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, record.trace_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, record.parent_span_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, record.frame_time_hist.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 9, record.success ? 1 : 0);
    sqlite3_bind_text(stmt, 10, record.error_code.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 11, record.error_message.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 12, record.frames_total);
    sqlite3_bind_int(stmt, 13, record.frames_written);
    sqlite3_bind_double(stmt, 14, record.wall_time_ms);
    sqlite3_bind_double(stmt, 15, record.render_ms);
    sqlite3_bind_double(stmt, 16, record.encode_ms);
    sqlite3_bind_double(stmt, 17, record.effective_fps);
    sqlite3_bind_int64(stmt, 18, static_cast<sqlite3_int64>(record.peak_working_set_bytes));
    sqlite3_bind_int64(stmt, 19, static_cast<sqlite3_int64>(record.avg_working_set_bytes));
    sqlite3_bind_int64(stmt, 20, static_cast<sqlite3_int64>(record.peak_private_bytes));
    sqlite3_bind_int64(stmt, 21, static_cast<sqlite3_int64>(record.avg_private_bytes));
    sqlite3_bind_double(stmt, 22, record.avg_cpu_percent_machine);
    sqlite3_bind_double(stmt, 23, record.avg_cpu_cores_used);
    sqlite3_bind_text(stmt, 24, record.output_path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 25, record.finished_at_iso.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_bind_int(stmt, 26, record.physical_cores);
    sqlite3_bind_double(stmt, 27, record.cpu_freq_mhz);
    sqlite3_bind_text(stmt, 28, record.gpu_vendor.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 29, record.gpu_driver.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 30, static_cast<sqlite3_int64>(record.total_ram_bytes));
    sqlite3_bind_int64(stmt, 31, static_cast<sqlite3_int64>(record.total_vram_bytes));
    sqlite3_bind_text(stmt, 32, record.git_commit_short.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 33, record.build_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 34, record.compiler_info.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 35, record.exit_code);
    sqlite3_bind_text(stmt, 36, record.error_category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 37, static_cast<sqlite3_int64>(record.total_pixel_ops));
    sqlite3_bind_int(stmt, 38, record.total_tiles);
    sqlite3_bind_int64(stmt, 39, static_cast<sqlite3_int64>(record.rasterized_pixels));
    sqlite3_bind_int64(stmt, 40, static_cast<sqlite3_int64>(record.blend_pixel_ops));
    sqlite3_bind_int64(stmt, 41, static_cast<sqlite3_int64>(record.encoded_pixels));
    sqlite3_bind_text(stmt, 42, record.memory_samples.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 43, record.cpu_util_samples.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 44, record.gpu_util_samples.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 45, record.preset_json.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 46, record.time_to_first_frame_ms);
    sqlite3_bind_int(stmt, 47, record.ffmpeg_queue_depth);

    sqlite3_bind_text(stmt, 48, record.hostname.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 49, record.os.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 50, record.cpu_model.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 51, record.logical_cores);
    sqlite3_bind_text(stmt, 52, record.tachyon_version.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "[SqliteTelemetryStore] Insert render record failed: " 
                  << sqlite3_errmsg(m_db) << std::endl;
        return false;
    }

    return true;
}

bool SqliteTelemetryStore::write_frame_records(const std::string& run_id, const std::vector<SqliteFrameRecord>& frames) {
    if (!m_db || frames.empty()) return false;

    const char* INSERT_FRAME_SQL = R"(
        INSERT OR REPLACE INTO render_frames (
            run_id, frame_number, duration_ms, encode_time_ms, write_time_ms, cache_hit, dirty_area_ratio
        ) VALUES (?, ?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, INSERT_FRAME_SQL, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[SqliteTelemetryStore] Prepare statement failed: " 
                  << sqlite3_errmsg(m_db) << std::endl;
        return false;
    }

    SqliteTransaction transaction(m_db);

    for (const auto& f : frames) {
        sqlite3_bind_text(stmt, 1, run_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, f.frame_number);
        sqlite3_bind_double(stmt, 3, f.duration_ms);
        sqlite3_bind_double(stmt, 4, f.encode_time_ms);
        sqlite3_bind_double(stmt, 5, f.write_time_ms);
        sqlite3_bind_int(stmt, 6, f.cache_hit ? 1 : 0);
        sqlite3_bind_double(stmt, 7, f.dirty_area_ratio);

        rc = sqlite3_step(stmt);
        sqlite3_reset(stmt);

        if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
            std::cerr << "[SqliteTelemetryStore] Frame insertion failed: " 
                      << sqlite3_errmsg(m_db) << std::endl;
            sqlite3_finalize(stmt);
            return false;
        }
    }

    sqlite3_finalize(stmt);
    transaction.commit();
    return true;
}

bool SqliteTelemetryStore::write_phase_events(const std::string& run_id, const std::vector<SqlitePhaseEventRecord>& events) {
    if (!m_db || events.empty()) return false;

    const char* INSERT_PHASE_SQL = R"(
        INSERT OR REPLACE INTO render_phase_events (
            run_id, phase_name, duration_ms
        ) VALUES (?, ?, ?);
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, INSERT_PHASE_SQL, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[SqliteTelemetryStore] Prepare statement failed: " 
                  << sqlite3_errmsg(m_db) << std::endl;
        return false;
    }

    SqliteTransaction transaction(m_db);

    for (const auto& e : events) {
        sqlite3_bind_text(stmt, 1, run_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, e.phase_name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 3, e.duration_ms);

        rc = sqlite3_step(stmt);
        sqlite3_reset(stmt);

        if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
            std::cerr << "[SqliteTelemetryStore] Phase insertion failed: " 
                      << sqlite3_errmsg(m_db) << std::endl;
            sqlite3_finalize(stmt);
            return false;
        }
    }

    sqlite3_finalize(stmt);
    transaction.commit();
    return true;
}

bool SqliteTelemetryStore::write_counters(const std::string& run_id, const std::vector<SqliteCounterRecord>& counters) {
    if (!m_db || counters.empty()) return false;

    const char* INSERT_COUNTER_SQL = R"(
        INSERT OR REPLACE INTO render_counters (
            run_id, counter_name, counter_value
        ) VALUES (?, ?, ?);
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, INSERT_COUNTER_SQL, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[SqliteTelemetryStore] Prepare statement failed: " 
                  << sqlite3_errmsg(m_db) << std::endl;
        return false;
    }

    SqliteTransaction transaction(m_db);

    for (const auto& c : counters) {
        sqlite3_bind_text(stmt, 1, run_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, c.counter_name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 3, static_cast<sqlite3_int64>(c.counter_value));

        rc = sqlite3_step(stmt);
        sqlite3_reset(stmt);

        if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
            std::cerr << "[SqliteTelemetryStore] Counter insertion failed: " 
                      << sqlite3_errmsg(m_db) << std::endl;
            sqlite3_finalize(stmt);
            return false;
        }
    }

    sqlite3_finalize(stmt);
    transaction.commit();
    return true;
}

int SqliteTelemetryStore::count_rows_for_test(const std::string& table) const {
    if (!m_db) return 0;
    std::string query = "SELECT COUNT(*) FROM " + table + ";";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, query.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return 0;

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count;
}

int SqliteTelemetryStore::user_version_for_test() const {
    if (!m_db) return 0;
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, "PRAGMA user_version;", -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return 0;

    int version = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        version = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return version;
}

std::vector<uint8_t> SqliteTelemetryStore::serialize_db_for_test() const {
    std::vector<uint8_t> out;
    if (!m_db) return out;

    // Run VACUUM to ensure deterministic page structures and cell placement
    sqlite3_exec(m_db, "VACUUM;", nullptr, nullptr, nullptr);

    sqlite3_int64 size = 0;
    unsigned char* data = sqlite3_serialize(m_db, "main", &size, 0);
    if (data && size > 0) {
        out.assign(data, data + size);
        sqlite3_free(data);
    }
    return out;
}

#else // TACHYON_ENABLE_SQLITE_TELEMETRY

// Graseful fallback stubs when telemetry is disabled

SqliteTelemetryStore::SqliteTelemetryStore() : m_db(nullptr) {}
SqliteTelemetryStore::~SqliteTelemetryStore() {}
SqliteTelemetryStore::SqliteTelemetryStore(SqliteTelemetryStore&& other) noexcept : m_db(nullptr) {}
SqliteTelemetryStore& SqliteTelemetryStore::operator=(SqliteTelemetryStore&& other) noexcept { return *this; }

bool SqliteTelemetryStore::is_open() const { return false; }
bool SqliteTelemetryStore::initialize(const std::filesystem::path&) { return false; }
bool SqliteTelemetryStore::setup_schema() { return false; }
bool SqliteTelemetryStore::write_render_record(const RenderTelemetryRecord&) { return false; }
bool SqliteTelemetryStore::write_frame_records(const std::string&, const std::vector<SqliteFrameRecord>&) { return false; }
bool SqliteTelemetryStore::write_phase_events(const std::string&, const std::vector<SqlitePhaseEventRecord>&) { return false; }
bool SqliteTelemetryStore::write_counters(const std::string&, const std::vector<SqliteCounterRecord>&) { return false; }
int SqliteTelemetryStore::count_rows_for_test(const std::string&) const { return 0; }
int SqliteTelemetryStore::user_version_for_test() const { return 0; }
std::vector<uint8_t> SqliteTelemetryStore::serialize_db_for_test() const { return {}; }

#endif // TACHYON_ENABLE_SQLITE_TELEMETRY

} // namespace tachyon
