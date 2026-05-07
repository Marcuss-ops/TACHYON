#include "cli_internal.h"
#include "tachyon/runtime/telemetry/render_telemetry_record.h"
#include "tachyon/runtime/telemetry/batch_telemetry_aggregator.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <algorithm>

namespace tachyon {

// Simplified JSON parser for our telemetry format
static RenderTelemetryRecord parse_json_line(const std::string& line) {
    RenderTelemetryRecord r;
    auto find_val = [&](const std::string& key) -> std::string {
        std::string search = "\"" + key + "\":";
        size_t pos = line.find(search);
        if (pos == std::string::npos) return "";
        pos += search.length();
        if (line[pos] == '"') {
            pos++;
            size_t end = line.find('"', pos);
            return line.substr(pos, end - pos);
        } else {
            size_t end = line.find_first_of(",}", pos);
            return line.substr(pos, end - pos);
        }
    };

    r.run_id = find_val("run_id");
    r.job_id = find_val("job_id");
    r.scene_id = find_val("scene_id");
    r.preset_id = find_val("preset_id");
    r.success = find_val("success") == "true";
    r.error_code = find_val("error_code");
    
    std::string wall_time = find_val("wall_time_ms");
    if (!wall_time.empty()) r.wall_time_ms = std::stod(wall_time);
    
    std::string fps = find_val("effective_fps");
    if (!fps.empty()) r.effective_fps = std::stod(fps);
    
    std::string peak_ws = find_val("peak_working_set_bytes");
    if (!peak_ws.empty()) r.peak_working_set_bytes = std::stoull(peak_ws);

    std::string peak_private = find_val("peak_private_bytes");
    if (!peak_private.empty()) r.peak_private_bytes = std::stoull(peak_private);

    std::string legacy_mem = find_val("peak_memory_bytes");
    if (!legacy_mem.empty() && r.peak_working_set_bytes == 0 && r.peak_private_bytes == 0) {
        r.peak_working_set_bytes = std::stoull(legacy_mem);
    }
    
    std::string cpu = find_val("avg_cpu_percent_machine");
    if (!cpu.empty()) r.avg_cpu_percent_machine = std::stod(cpu);

    std::string legacy_cpu = find_val("avg_cpu_percent");
    if (!legacy_cpu.empty() && r.avg_cpu_percent_machine == 0.0) {
        r.avg_cpu_percent_machine = std::stod(legacy_cpu);
    }
    
    std::string render_ms = find_val("render_ms");
    if (!render_ms.empty()) r.render_ms = std::stod(render_ms);

    std::string encode_ms = find_val("encode_ms");
    if (!encode_ms.empty()) r.encode_ms = std::stod(encode_ms);

    return r;
}

bool run_metrics_command(const CliOptions& options, std::ostream& out, std::ostream& err, TransitionRegistry& registry) {
    std::ifstream ifs(options.metrics_input);
    if (!ifs) {
        err << "Failed to open input file: " << options.metrics_input << "\n";
        return false;
    }

    std::vector<RenderTelemetryRecord> records;
    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;
        try {
            records.push_back(parse_json_line(line));
        } catch (...) {
            // Skip malformed lines
        }
    }

    if (records.empty()) {
        err << "No records found in " << options.metrics_input << "\n";
        return false;
    }

    auto summary = aggregate_telemetry(records);

    if (options.metrics_command == "summary" || options.metrics_command.empty()) {
        out << "--------------------------------------------------\n";
        out << " TACHYON BATCH TELEMETRY SUMMARY\n";
        out << "--------------------------------------------------\n";
        out << "Batch ID:       " << summary.batch_id << "\n";
        out << "Total Jobs:     " << summary.total_jobs << "\n";
        out << "Succeeded:      " << summary.succeeded << "\n";
        out << "Failed:         " << summary.failed << "\n";
        out << "Failure Rate:   " << std::fixed << std::setprecision(1) << summary.failure_rate_per_1000 << " / 1000\n";
        out << "--------------------------------------------------\n";
        out << "Performance Metrics (Averages):\n";
        out << "Wall Time:      " << std::fixed << std::setprecision(2) << summary.avg_wall_time_ms / 1000.0 << "s (P95: " << summary.p95_wall_time_ms / 1000.0 << "s)\n";
        out << "Effective FPS:  " << summary.avg_effective_fps << "\n";
        out << "Throughput:     " << summary.videos_per_hour << " videos/hour/machine\n";
        out << "--------------------------------------------------\n";
        out << "Resource Usage:\n";
        const std::size_t peak_memory_bytes = std::max(summary.peak_working_set_bytes, summary.peak_private_bytes);
        out << "Peak Memory:    " << peak_memory_bytes / (1024 * 1024) << " MB\n";
        out << "Avg CPU:        " << std::fixed << std::setprecision(1) << summary.avg_cpu_percent_machine << "%\n";
        out << "--------------------------------------------------\n";
        
        if (options.machine_cost_per_hour > 0.0) {
            double total_hours = summary.total_wall_time_ms / 3600000.0;
            double total_cost = total_hours * options.machine_cost_per_hour;
            double cost_per_video = summary.succeeded > 0 ? total_cost / summary.succeeded : 0.0;
            
            out << "Business Metrics:\n";
            out << "Machine Cost/h: $" << std::fixed << std::setprecision(2) << options.machine_cost_per_hour << "\n";
            out << "Total Eff. Cost:$" << total_cost << " (Effort)\n";
            out << "Cost / Video:   $" << std::setprecision(4) << cost_per_video << "\n";
            out << "Cost / 1000:    $" << std::setprecision(2) << cost_per_video * 1000.0 << "\n";
            out << "--------------------------------------------------\n";
        }

        if (!summary.failures_by_error_code.empty()) {
            out << "Top Failures:\n";
            for (const auto& f : summary.failures_by_error_code) {
                out << "  " << std::left << std::setw(20) << f.first << ": " << f.second << "\n";
            }
            out << "--------------------------------------------------\n";
        }
    } else if (options.metrics_command == "slowest") {
        std::sort(records.begin(), records.end(), [](const RenderTelemetryRecord& a, const RenderTelemetryRecord& b) {
            return a.wall_time_ms > b.wall_time_ms;
        });

        out << "Top " << options.metrics_top << " Slowest Jobs:\n";
        out << std::left << std::setw(30) << "Job ID" << std::setw(15) << "Wall Time" << "Scene\n";
        int count = 0;
        for (const auto& r : records) {
            if (++count > options.metrics_top) break;
            out << std::left << std::setw(30) << r.job_id 
                << std::fixed << std::setprecision(2) << std::setw(15) << r.wall_time_ms / 1000.0 << r.scene_id << "\n";
        }
    } else if (options.metrics_command == "failures") {
        out << "Failed Jobs:\n";
        out << std::left << std::setw(30) << "Job ID" << std::setw(20) << "Error Code" << "Message\n";
        for (const auto& r : records) {
            if (r.success) continue;
            out << std::left << std::setw(30) << r.job_id 
                << std::setw(20) << r.error_code << r.error_message << "\n";
        }
    }

    return true;
}

} // namespace tachyon
