#include "test_utils.h"
#include "tachyon/runtime/telemetry/sqlite_telemetry_store.h"
#include <iostream>
#include <vector>
#include <string>

#ifdef TACHYON_ENABLE_SQLITE_TELEMETRY
#include <sqlite3.h>
#endif

#ifdef TACHYON_ENABLE_SQLITE_TELEMETRY

namespace {

// Helper to query row count of a table
int query_row_count(sqlite3* db, const std::string& table_name) {
    std::string query = "SELECT COUNT(*) FROM " + table_name + ";";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return -1;
    }
    
    int count = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count;
}

// Access private sqlite3 pointer using standard layout alignment
sqlite3* get_raw_db_pointer(const tachyon::SqliteTelemetryStore& store) {
    // SqliteTelemetryStore has a virtual table, so the vtable pointer is at offset 0.
    // The m_db pointer (sqlite3*) is the first data member, placed immediately after the vtable pointer.
    const void* ptr = &store;
    const sqlite3* const* db_ptr = reinterpret_cast<const sqlite3* const*>(
        static_cast<const char*>(ptr) + sizeof(void*)
    );
    return const_cast<sqlite3*>(*db_ptr);
}

} // namespace

#endif // TACHYON_ENABLE_SQLITE_TELEMETRY

// Defined in global namespace to match TachyonRuntimeTests runner linkage
bool run_sqlite_telemetry_store_tests() {
    std::cout << "[TelemetryStore] Starting SQLite Telemetry Store unit tests...\n";

#ifndef TACHYON_ENABLE_SQLITE_TELEMETRY
    std::cout << "[TelemetryStore] TACHYON_ENABLE_SQLITE_TELEMETRY is disabled. Skipping DB tests.\n";
    return true;
#else

    using namespace tachyon;

    SqliteTelemetryStore store;
    
    // 1. Initial State
    if (store.is_open()) {
        std::cerr << "[TelemetryStore] FAIL: Store should not be open before initialization.\n";
        return false;
    }

    // 2. Open In-Memory Database
    if (!store.initialize(":memory:")) {
        std::cerr << "[TelemetryStore] FAIL: In-memory DB initialization failed.\n";
        return false;
    }

    if (!store.is_open()) {
        std::cerr << "[TelemetryStore] FAIL: Store should be open after initialization.\n";
        return false;
    }

    sqlite3* raw_db = get_raw_db_pointer(store);
    if (!raw_db) {
        std::cerr << "[TelemetryStore] FAIL: Failed to access raw sqlite3 pointer.\n";
        return false;
    }

    // 3. Verify Table Creation
    if (query_row_count(raw_db, "render_runs") != 0 ||
        query_row_count(raw_db, "render_frames") != 0 ||
        query_row_count(raw_db, "render_phase_events") != 0 ||
        query_row_count(raw_db, "render_counters") != 0) {
        std::cerr << "[TelemetryStore] FAIL: Database schemas were not created correctly.\n";
        return false;
    }
    std::cout << "[TelemetryStore] Initial schemas successfully verified!\n";

    // 4. Test Single Render Record Insertion
    RenderTelemetryRecord run_record;
    run_record.run_id = "test-run-001";
    run_record.job_id = "job-100";
    run_record.scene_id = "scene-alpha";
    run_record.preset_id = "preset-hd";
    run_record.machine_id = "local-cpu-dev";
    run_record.success = true;
    run_record.error_code = "None";
    run_record.error_message = "";
    run_record.frames_total = 120;
    run_record.frames_written = 120;
    run_record.wall_time_ms = 4500.5;
    run_record.render_ms = 3500.0;
    run_record.encode_ms = 1000.0;
    run_record.effective_fps = 26.66;
    run_record.peak_working_set_bytes = 1024 * 1024 * 150;
    run_record.avg_working_set_bytes = 1024 * 1024 * 120;
    run_record.peak_private_bytes = 1024 * 1024 * 170;
    run_record.avg_private_bytes = 1024 * 1024 * 130;
    run_record.avg_cpu_percent_machine = 62.5;
    run_record.avg_cpu_cores_used = 8.0;
    run_record.output_path = "/home/pierone/output.mp4";
    run_record.finished_at_iso = "2026-05-17T08:00:00Z";

    if (!store.write_render_record(run_record)) {
        std::cerr << "[TelemetryStore] FAIL: Failed to write render run record.\n";
        return false;
    }

    if (query_row_count(raw_db, "render_runs") != 1) {
        std::cerr << "[TelemetryStore] FAIL: render_runs count is not 1 after insert.\n";
        return false;
    }
    std::cout << "[TelemetryStore] render_runs insertion and validation passed!\n";

    // 5. Test Frame Metrics Batch Insertion
    std::vector<SqliteFrameRecord> frames = {
        {0, 35.2, 8.1, 2.0, false},
        {1, 32.1, 7.9, 1.9, true},
        {2, 34.0, 8.0, 2.1, false}
    };

    if (!store.write_frame_records(run_record.run_id, frames)) {
        std::cerr << "[TelemetryStore] FAIL: Failed to write frame records.\n";
        return false;
    }

    if (query_row_count(raw_db, "render_frames") != 3) {
        std::cerr << "[TelemetryStore] FAIL: render_frames count is not 3 after insert.\n";
        return false;
    }
    std::cout << "[TelemetryStore] render_frames batch insertion passed!\n";

    // 6. Test Phase Events Batch Insertion
    std::vector<SqlitePhaseEventRecord> phases = {
        {"compile_scene", 120.5},
        {"build_render_plan", 45.2},
        {"rasterization", 3300.0},
        {"ffmpeg_encoding", 995.5}
    };

    if (!store.write_phase_events(run_record.run_id, phases)) {
        std::cerr << "[TelemetryStore] FAIL: Failed to write phase event records.\n";
        return false;
    }

    if (query_row_count(raw_db, "render_phase_events") != 4) {
        std::cerr << "[TelemetryStore] FAIL: render_phase_events count is not 4 after insert.\n";
        return false;
    }
    std::cout << "[TelemetryStore] render_phase_events batch insertion passed!\n";

    // 7. Test Optimization Counters Batch Insertion
    std::vector<SqliteCounterRecord> counters = {
        {"simd_lerp_calls", 1524300},
        {"tiles_reused", 8500},
        {"cache_hits", 42}
    };

    if (!store.write_counters(run_record.run_id, counters)) {
        std::cerr << "[TelemetryStore] FAIL: Failed to write optimization counter records.\n";
        return false;
    }

    if (query_row_count(raw_db, "render_counters") != 3) {
        std::cerr << "[TelemetryStore] FAIL: render_counters count is not 3 after insert.\n";
        return false;
    }
    std::cout << "[TelemetryStore] render_counters batch insertion passed!\n";

    // 8. Test Moving Store Connection
    SqliteTelemetryStore moved_store = std::move(store);
    if (store.is_open()) {
        std::cerr << "[TelemetryStore] FAIL: Source store should not be open after move.\n";
        return false;
    }
    if (!moved_store.is_open()) {
        std::cerr << "[TelemetryStore] FAIL: Destination store should be open after move.\n";
        return false;
    }
    std::cout << "[TelemetryStore] Move constructor verified successfully!\n";

    std::cout << "[TelemetryStore] ALL SQLite Telemetry Store tests passed successfully!\n";
    return true;
#endif
}
