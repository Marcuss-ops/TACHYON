#include "tachyon/runtime/telemetry/telemetry_writer.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>

namespace tachyon {

std::mutex TelemetryWriter::s_write_mutex;

std::filesystem::path TelemetryWriter::get_default_directory() {
    const char* home = std::getenv("HOME");
    if (!home) home = std::getenv("USERPROFILE");
    if (!home) return ".tachyon/telemetry";
    
    std::filesystem::path path(home);
    path /= ".tachyon";
    path /= "telemetry";
    if (!std::filesystem::exists(path)) {
        std::filesystem::create_directories(path);
    }
    return path;
}

static std::string escape_json(const std::string& s) {
    std::ostringstream o;
    for (auto c : s) {
        if (c == '"') o << "\\\"";
        else if (c == '\\') o << "\\\\";
        else if (c == '\b') o << "\\b";
        else if (c == '\f') o << "\\f";
        else if (c == '\n') o << "\\n";
        else if (c == '\r') o << "\\r";
        else if (c == '\t') o << "\\t";
        else if (static_cast<unsigned char>(c) < 32) {
            o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
        } else {
            o << c;
        }
    }
    return o.str();
}

static std::string record_to_json(const RenderTelemetryRecord& r) {
    std::stringstream ss;
    ss << "{";
    ss << "\"run_id\":\"" << escape_json(r.run_id) << "\",";
    ss << "\"job_id\":\"" << escape_json(r.job_id) << "\",";
    ss << "\"scene_id\":\"" << escape_json(r.scene_id) << "\",";
    ss << "\"preset_id\":\"" << escape_json(r.preset_id) << "\",";
    ss << "\"machine_id\":\"" << escape_json(r.machine_id) << "\",";
    ss << "\"success\":" << (r.success ? "true" : "false") << ",";
    ss << "\"error_code\":\"" << escape_json(r.error_code) << "\",";
    ss << "\"error_message\":\"" << escape_json(r.error_message) << "\",";
    ss << "\"frames_total\":" << r.frames_total << ",";
    ss << "\"frames_written\":" << r.frames_written << ",";
    ss << "\"wall_time_ms\":" << std::fixed << std::setprecision(2) << r.wall_time_ms << ",";
    ss << "\"render_ms\":" << r.render_ms << ",";
    ss << "\"encode_ms\":" << r.encode_ms << ",";
    ss << "\"effective_fps\":" << r.effective_fps << ",";
    ss << "\"peak_working_set_bytes\":" << r.peak_working_set_bytes << ",";
    ss << "\"avg_working_set_bytes\":" << r.avg_working_set_bytes << ",";
    ss << "\"peak_private_bytes\":" << r.peak_private_bytes << ",";
    ss << "\"avg_private_bytes\":" << r.avg_private_bytes << ",";
    ss << "\"avg_cpu_percent_machine\":" << r.avg_cpu_percent_machine << ",";
    ss << "\"avg_cpu_cores_used\":" << r.avg_cpu_cores_used << ",";
    ss << "\"output_path\":\"" << escape_json(r.output_path) << "\",";
    ss << "\"finished_at_iso\":\"" << escape_json(r.finished_at_iso) << "\"";
    ss << "}";
    return ss.str();
}

bool TelemetryWriter::append_jsonl(const RenderTelemetryRecord& record, const std::filesystem::path& path) {
    std::lock_guard<std::mutex> lock(s_write_mutex);
    std::ofstream ofs(path, std::ios::app);
    if (!ofs) return false;
    ofs << record_to_json(record) << "\n";
    return true;
}

bool TelemetryWriter::append_tsv(const RenderTelemetryRecord& record, const std::filesystem::path& path) {
    std::lock_guard<std::mutex> lock(s_write_mutex);
    bool exists = std::filesystem::exists(path);
    std::ofstream ofs(path, std::ios::app);
    if (!ofs) return false;
    
    if (!exists) {
        ofs << "run_id\tjob_id\tscene_id\tsuccess\twall_time_ms\teffective_fps\tpeak_mem\n";
    }
    
    ofs << record.run_id << "\t"
        << record.job_id << "\t"
        << record.scene_id << "\t"
        << (record.success ? "1" : "0") << "\t"
        << std::fixed << std::setprecision(2) << record.wall_time_ms << "\t"
        << record.effective_fps << "\t"
        << record.peak_working_set_bytes << "\n";
        
    return true;
}

bool TelemetryWriter::write_batch_summary(const BatchTelemetrySummary& summary, const std::filesystem::path& path) {
    std::ofstream ofs(path);
    if (!ofs) return false;
    
    ofs << "{\n";
    ofs << "  \"batch_id\": \"" << escape_json(summary.batch_id) << "\",\n";
    ofs << "  \"total_jobs\": " << summary.total_jobs << ",\n";
    ofs << "  \"succeeded\": " << summary.succeeded << ",\n";
    ofs << "  \"failed\": " << summary.failed << ",\n";
    ofs << "  \"failure_rate_per_1000\": " << summary.failure_rate_per_1000 << ",\n";
    ofs << "  \"avg_wall_time_ms\": " << summary.avg_wall_time_ms << ",\n";
    ofs << "  \"avg_effective_fps\": " << summary.avg_effective_fps << ",\n";
    ofs << "  \"peak_working_set_bytes\": " << summary.peak_working_set_bytes << ",\n";
    ofs << "  \"peak_private_bytes\": " << summary.peak_private_bytes << ",\n";
    ofs << "  \"estimated_total_cost_usd\": " << summary.estimated_total_cost_usd << "\n";
    ofs << "}\n";
    
    return true;
}

} // namespace tachyon
