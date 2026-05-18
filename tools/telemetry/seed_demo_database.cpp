#include "tachyon/runtime/telemetry/sqlite_telemetry_store.h"
#include "tachyon/runtime/telemetry/telemetry_writer.h"
#include "tachyon/runtime/telemetry/render_telemetry_record.h"
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <random>

using namespace tachyon;

RenderTelemetryRecord create_fake_record(int i) {
    RenderTelemetryRecord r;
    r.run_id = "demo-run-" + std::to_string(1000 + i);
    r.job_id = "demo-job-" + std::to_string(i / 10);
    r.scene_id = (i % 2 == 0) ? "neon_city" : "aurora_forest";
    r.preset_id = "hq_4k";
    r.machine_id = "workstation-01";
    r.success = (i % 20 != 0); // 5% failure rate
    r.frames_total = 120;
    r.frames_written = r.success ? 120 : 45;
    r.wall_time_ms = 5000.0 + (rand() % 2000);
    r.render_ms = r.wall_time_ms * 0.8;
    r.encode_ms = r.wall_time_ms * 0.15;
    r.effective_fps = static_cast<double>(r.frames_written) / (r.wall_time_ms / 1000.0);
    r.peak_working_set_bytes = 1024 * 1024 * (500 + (rand() % 500));
    r.avg_cpu_percent_machine = 40.0 + (rand() % 40);
    r.finished_at_iso = "2026-05-17T12:00:00Z";
    
    r.physical_cores = 16;
    r.cpu_freq_mhz = 4200.0;
    r.gpu_vendor = "NVIDIA";
    r.gpu_driver = "555.12";
    r.total_ram_bytes = 32LL * 1024 * 1024 * 1024;
    r.git_commit_short = "a1b2c3d4";
    r.build_type = "Release";
    r.compiler_info = "clang-18";
    
    return r;
}

int main(int argc, char** argv) {
    std::cout << "Tachyon Telemetry Seeder\n";
    std::cout << "------------------------\n";

    const auto db_dir = TelemetryWriter::get_default_directory();
    const auto db_path = db_dir / "tachyon_render_history.sqlite";
    
    std::cout << "Target DB: " << db_path << "\n";
    
    SqliteTelemetryStore store;
    if (!store.initialize(db_path)) {
        std::cerr << "Failed to initialize store at " << db_path << "\n";
        return 1;
    }
    
    int count = 50;
    if (argc > 1) count = std::stoi(argv[1]);
    
    std::cout << "Seeding " << count << " records...\n";
    
    for (int i = 0; i < count; ++i) {
        auto record = create_fake_record(i);
        store.write_render_record(record);
        
        if (i % 10 == 0) {
            std::cout << "Progress: " << i << "/" << count << "\n";
        }
    }
    
    std::cout << "Seeding complete!\n";
    return 0;
}
