#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "cli/commands/command.h"
#include "cli/support/cli_internal.h"

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <fstream>
#include <numeric>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <cmath>

#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace tachyon {

namespace {

struct BenchmarkVariant {
    std::string name;
    std::string encoder_backend;
    bool async_output{false};
    bool telemetry{false};
};

struct BenchmarkResult {
    std::string variant;
    int run_index{0};
    int exit_code{0};
    double wall_time_ms{0.0};
    std::uintmax_t output_bytes{0};
    double effective_fps{0.0};
    double video_seconds_per_render_second{0.0};
    std::string log_tail;
    std::string output_path;
};

struct VariantStats {
    std::string variant;
    double min_ms{0.0};
    double max_ms{0.0};
    double median_ms{0.0};
    double p95_ms{0.0};
    double effective_fps{0.0};
    double speedup{0.0};
    std::uintmax_t median_bytes{0};
    double success_rate{0.0};
};

int parse_frame_count(const std::string& range_str) {
    try {
        size_t dash = range_str.find('-');
        if (dash == std::string::npos) {
            return 1;
        }
        long long start = std::stoll(range_str.substr(0, dash));
        long long end = std::stoll(range_str.substr(dash + 1));
        if (end >= start) {
            return static_cast<int>(end - start + 1);
        }
    } catch (...) {
        // Fallback to default
    }
    return 180;
}

std::pair<int, std::string> execute_command(const std::string& cmd) {
    std::string output;
    char buffer[256];
    
#ifdef _WIN32
    FILE* pipe = _popen(cmd.c_str(), "r");
#else
    FILE* pipe = popen(cmd.c_str(), "r");
#endif

    if (!pipe) {
        return {-1, "Failed to start command via popen"};
    }
    
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    
#ifdef _WIN32
    int status = _pclose(pipe);
    int exit_code = status;
#else
    int status = pclose(pipe);
    int exit_code = -1;
    if (status != -1) {
        if (WIFEXITED(status)) {
            exit_code = WEXITSTATUS(status);
        } else {
            exit_code = status;
        }
    }
#endif
    
    return {exit_code, output};
}

std::string get_tail(const std::string& str, size_t max_lines = 8) {
    std::vector<std::string> lines;
    std::stringstream ss(str);
    std::string line;
    while (std::getline(ss, line)) {
        lines.push_back(line);
    }
    
    if (lines.size() <= max_lines) {
        return str;
    }
    
    std::string tail;
    for (size_t i = lines.size() - max_lines; i < lines.size(); ++i) {
        tail += lines[i] + "\n";
    }
    return tail;
}

std::string escape_json_string(const std::string& s) {
    std::stringstream ss;
    for (char c : s) {
        switch (c) {
            case '"': ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\b': ss << "\\b"; break;
            case '\f': ss << "\\f"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            default:
                if (c >= 0 && c < 32) {
                    ss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                } else {
                    ss << c;
                }
        }
    }
    return ss.str();
}

std::string escape_csv_string(const std::string& s) {
    std::string result = "\"";
    for (char c : s) {
        if (c == '"') {
            result += "\"\"";
        } else if (c == '\n' || c == '\r') {
            result += " ";
        } else {
            result += c;
        }
    }
    result += "\"";
    return result;
}

} // namespace

bool run_bench_command(const CliOptions& options, std::ostream& out, std::ostream& err, runtime::EngineRegistry& /*bundle*/) {
    std::string tachyon_path = "./build/src/tachyon";
    try {
        if (fs::exists("/proc/self/exe")) {
            tachyon_path = fs::canonical("/proc/self/exe").string();
        }
    } catch (...) {
        // fallback
    }

    std::string scene_path = options.bench.cpp_path.string();
    std::string preset = options.bench.preset_id;
    std::string frame_range = options.bench.frame_range;
    int runs = options.bench.runs;
    int warmup = options.bench.warmup;
    std::string out_dir = options.bench.out_dir.string();

    if (scene_path.empty() && preset.empty()) {
        preset = "tachyon.scene.minimal_text";
    }

    // Determine paths for CSV and JSON outputs
    std::string json_path = "";
    std::string csv_path = "";
    std::string format_str = options.bench.formats;
    
    // Parse formats
    bool export_json = false;
    bool export_csv = false;
    std::size_t start = 0;
    std::size_t end = format_str.find(',');
    std::vector<std::string> formats;
    while (end != std::string::npos) {
        formats.push_back(format_str.substr(start, end - start));
        start = end + 1;
        end = format_str.find(',', start);
    }
    formats.push_back(format_str.substr(start));

    for (const auto& fmt : formats) {
        if (fmt == "json") {
            export_json = true;
            json_path = out_dir + "/results.json";
        } else if (fmt == "csv") {
            export_csv = true;
            csv_path = out_dir + "/results.csv";
        }
    }

    // Ensure output directories exist
    if (!fs::exists(out_dir)) {
        fs::create_directories(out_dir);
    }

    out << "\n=======================================================\n";
    out << "           TACHYON RENDER BENCHMARK RUNNER\n";
    out << "=======================================================\n";
    out << "  Tachyon CLI:  " << tachyon_path << "\n";
    if (!scene_path.empty()) {
        out << "  Scene Spec:   " << scene_path << "\n";
    } else {
        out << "  Preset ID:    " << preset << "\n";
    }
    out << "  Frame Range:  " << frame_range << "\n";
    out << "  Measured Runs:" << runs << " per variant\n";
    out << "  Warmup Runs:  " << warmup << " per variant\n";
    out << "  Output Dir:   " << out_dir << "\n";
    if (export_json) out << "  JSON Reports: " << json_path << "\n";
    if (export_csv)  out << "  CSV Reports:  " << csv_path << "\n";
    out << "  Cache Mode:   " << (options.bench.cache ? "ON" : "OFF") << "\n";
    out << "=======================================================\n\n";

    std::vector<BenchmarkVariant> variants = {
        {"baseline", "software", false, false},
        {"async_output", "software", true, false},
        {"hardware_encoder", "auto", false, false},
        {"hardware_encoder_async_output", "auto", true, false},
        {"telemetry_enabled", "software", false, true}
    };

    int total_frames = parse_frame_count(frame_range);
    std::vector<BenchmarkResult> all_results;
    std::vector<VariantStats> all_stats;

    // Execute Benchmark variants
    for (const auto& variant : variants) {
        out << ">>> Launching Variant: " << variant.name << " <<<\n";
        
        // Warmup runs
        for (int w = 0; w < warmup; ++w) {
            out << "  [Warmup] Run " << (w + 1) << "/" << warmup << "... " << std::flush;
            
            std::string run_out_path = out_dir + "/" + variant.name + "_warmup_" + std::to_string(w) + ".mp4";
            
            std::stringstream cmd;
            cmd << "TACHYON_ENCODER_BACKEND=" << variant.encoder_backend << " "
                << "TACHYON_OUTPUT_ASYNC=" << (variant.async_output ? "1" : "0") << " "
                << "TACHYON_TELEMETRY=" << (variant.telemetry ? "1" : "0") << " "
                << "TACHYON_CACHE=" << (options.bench.cache ? "on" : "off") << " "
                << tachyon_path << " render";
            
            if (!scene_path.empty()) {
                cmd << " --cpp " << scene_path;
            } else {
                cmd << " --preset " << preset;
            }
            cmd << " --out " << run_out_path << " --frames " << frame_range << " 2>&1";
            
            auto [exit_code, log] = execute_command(cmd.str());
            
            if (fs::exists(run_out_path)) {
                fs::remove(run_out_path);
            }
            
            out << (exit_code == 0 ? "SUCCESS" : "FAILED") << "\n";
        }

        // Measured runs
        std::vector<double> run_times;
        std::vector<std::uintmax_t> run_bytes;
        int successful_runs = 0;

        for (int r = 0; r < runs; ++r) {
            out << "  [Measured] Run " << (r + 1) << "/" << runs << "... " << std::flush;
            
            std::string run_out_path = out_dir + "/" + variant.name + "_run_" + std::to_string(r) + ".mp4";
            if (fs::exists(run_out_path)) {
                fs::remove(run_out_path);
            }

            std::stringstream cmd;
            cmd << "TACHYON_ENCODER_BACKEND=" << variant.encoder_backend << " "
                << "TACHYON_OUTPUT_ASYNC=" << (variant.async_output ? "1" : "0") << " "
                << "TACHYON_TELEMETRY=" << (variant.telemetry ? "1" : "0") << " "
                << "TACHYON_CACHE=" << (options.bench.cache ? "on" : "off") << " "
                << tachyon_path << " render";
            
            if (!scene_path.empty()) {
                cmd << " --cpp " << scene_path;
            } else {
                cmd << " --preset " << preset;
            }
            cmd << " --out " << run_out_path << " --frames " << frame_range << " 2>&1";

            auto start_time = std::chrono::high_resolution_clock::now();
            auto [exit_code, log] = execute_command(cmd.str());
            auto end_time = std::chrono::high_resolution_clock::now();
            
            double duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
            
            std::uintmax_t bytes = 0;
            if (exit_code == 0 && fs::exists(run_out_path)) {
                bytes = fs::file_size(run_out_path);
                successful_runs++;
                run_times.push_back(duration_ms);
                run_bytes.push_back(bytes);
            } else {
                out << "FAILED (Exit Code: " << exit_code << ")\n";
            }

            // Keep output of first run for validation, remove the rest to save disk space
            if (r > 0 && fs::exists(run_out_path)) {
                fs::remove(run_out_path);
            }

            double fps = (exit_code == 0) ? (total_frames / (duration_ms / 1000.0)) : 0.0;
            double video_ratio = (exit_code == 0) ? ((total_frames / 60.0) / (duration_ms / 1000.0)) : 0.0;

            BenchmarkResult res;
            res.variant = variant.name;
            res.run_index = r;
            res.exit_code = exit_code;
            res.wall_time_ms = duration_ms;
            res.output_bytes = bytes;
            res.effective_fps = fps;
            res.video_seconds_per_render_second = video_ratio;
            res.log_tail = get_tail(log, 8);
            res.output_path = run_out_path;

            all_results.push_back(res);

            if (exit_code == 0) {
                out << std::fixed << std::setprecision(1) 
                    << duration_ms << " ms (" << fps << " FPS)\n";
            }
        }

        // Calculate statistics
        VariantStats stats;
        stats.variant = variant.name;
        stats.success_rate = (static_cast<double>(successful_runs) / runs) * 100.0;

        if (successful_runs > 0) {
            std::sort(run_times.begin(), run_times.end());
            std::sort(run_bytes.begin(), run_bytes.end());

            stats.min_ms = run_times.front();
            stats.max_ms = run_times.back();
            
            if (run_times.size() % 2 == 1) {
                stats.median_ms = run_times[run_times.size() / 2];
                stats.median_bytes = run_bytes[run_bytes.size() / 2];
            } else {
                stats.median_ms = (run_times[run_times.size() / 2 - 1] + run_times[run_times.size() / 2]) / 2.0;
                stats.median_bytes = (run_bytes[run_bytes.size() / 2 - 1] + run_bytes[run_bytes.size() / 2]) / 2;
            }

            size_t p95_idx = static_cast<size_t>(std::ceil(0.95 * run_times.size())) - 1;
            stats.p95_ms = run_times[std::clamp(p95_idx, size_t(0), run_times.size() - 1)];

            stats.effective_fps = total_frames / (stats.median_ms / 1000.0);
        }

        all_stats.push_back(stats);
        out << "\n";
    }

    // Speedup vs baseline
    double baseline_median = 0.0;
    for (auto& stats : all_stats) {
        if (stats.variant == "baseline") {
            baseline_median = stats.median_ms;
            break;
        }
    }

    for (auto& stats : all_stats) {
        if (baseline_median > 0.0 && stats.median_ms > 0.0) {
            stats.speedup = baseline_median / stats.median_ms;
        } else {
            stats.speedup = 1.0;
        }
    }

    // ASCII Table printout
    out << "==========================================================================================\n";
    out << "                               BENCHMARK COMPREHENSIVE SUMMARY\n";
    out << "==========================================================================================\n";
    out << std::left << std::setw(32) << "Variant" 
        << std::right << std::setw(12) << "Median (ms)" 
        << std::setw(10) << "Min (ms)" 
        << std::setw(10) << "Max (ms)" 
        << std::setw(10) << "P95 (ms)" 
        << std::setw(10) << "FPS" 
        << std::setw(10) << "Speedup"
        << std::setw(12) << "Success Rate" << "\n";
    out << "------------------------------------------------------------------------------------------\n";

    for (const auto& stats : all_stats) {
        out << std::left << std::setw(32) << stats.variant;
        if (stats.success_rate > 0.0) {
            out << std::right << std::setw(12) << std::fixed << std::setprecision(1) << stats.median_ms
                << std::setw(10) << stats.min_ms
                << std::setw(10) << stats.max_ms
                << std::setw(10) << stats.p95_ms
                << std::setw(10) << std::fixed << std::setprecision(2) << stats.effective_fps
                << std::setw(9) << stats.speedup << "x"
                << std::setw(11) << stats.success_rate << "%\n";
        } else {
            out << std::right << std::setw(12) << "N/A"
                << std::setw(10) << "N/A"
                << std::setw(10) << "N/A"
                << std::setw(10) << "N/A"
                << std::setw(10) << "N/A"
                << std::setw(10) << "N/A"
                << std::setw(11) << "0.0%\n";
        }
    }
    out << "==========================================================================================\n\n";

    // Write CSV Output
    if (export_csv) {
        std::ofstream csv(csv_path);
        if (csv.is_open()) {
            csv << "variant,run_index,scene,preset,frames,wall_time_ms,exit_code,output_bytes,effective_fps,video_seconds_per_render_second,log_tail\n";
            for (const auto& r : all_results) {
                csv << escape_csv_string(r.variant) << ","
                    << r.run_index << ","
                    << escape_csv_string(scene_path) << ","
                    << escape_csv_string(preset) << ","
                    << escape_csv_string(frame_range) << ","
                    << std::fixed << std::setprecision(3) << r.wall_time_ms << ","
                    << r.exit_code << ","
                    << r.output_bytes << ","
                    << std::fixed << std::setprecision(3) << r.effective_fps << ","
                    << std::fixed << std::setprecision(3) << r.video_seconds_per_render_second << ","
                    << escape_csv_string(r.log_tail) << "\n";
            }
            csv.close();
            out << "[SUCCESS] Wrote CSV results to: " << csv_path << "\n";
        } else {
            err << "[ERROR] Could not write CSV results to: " << csv_path << "\n";
        }
    }

    // Write JSON Output
    if (export_json) {
        std::ofstream json(json_path);
        if (json.is_open()) {
            json << "{\n";
            json << "  \"metadata\": {\n";
            json << "    \"tachyon_path\": \"" << escape_json_string(tachyon_path) << "\",\n";
            json << "    \"scene_path\": \"" << escape_json_string(scene_path) << "\",\n";
            json << "    \"preset\": \"" << escape_json_string(preset) << "\",\n";
            json << "    \"frame_range\": \"" << escape_json_string(frame_range) << "\",\n";
            json << "    \"total_frames\": " << total_frames << ",\n";
            json << "    \"runs_per_variant\": " << runs << ",\n";
            json << "    \"warmup_runs\": " << warmup << ",\n";
            json << "    \"cache_mode_enabled\": " << (options.bench.cache ? "true" : "false") << "\n";
            json << "  },\n";
            
            json << "  \"statistics\": [\n";
            for (size_t i = 0; i < all_stats.size(); ++i) {
                const auto& s = all_stats[i];
                json << "    {\n";
                json << "      \"variant\": \"" << escape_json_string(s.variant) << "\",\n";
                json << "      \"median_ms\": " << s.median_ms << ",\n";
                json << "      \"min_ms\": " << s.min_ms << ",\n";
                json << "      \"max_ms\": " << s.max_ms << ",\n";
                json << "      \"p95_ms\": " << s.p95_ms << ",\n";
                json << "      \"effective_fps\": " << s.effective_fps << ",\n";
                json << "      \"speedup_vs_baseline\": " << s.speedup << ",\n";
                json << "      \"median_bytes\": " << s.median_bytes << ",\n";
                json << "      \"success_rate_percent\": " << s.success_rate << "\n";
                json << "    }" << (i + 1 < all_stats.size() ? "," : "") << "\n";
            }
            json << "  ],\n";
            
            json << "  \"runs\": [\n";
            for (size_t i = 0; i < all_results.size(); ++i) {
                const auto& r = all_results[i];
                json << "    {\n";
                json << "      \"variant\": \"" << escape_json_string(r.variant) << "\",\n";
                json << "      \"run_index\": " << r.run_index << ",\n";
                json << "      \"exit_code\": " << r.exit_code << ",\n";
                json << "      \"wall_time_ms\": " << r.wall_time_ms << ",\n";
                json << "      \"output_bytes\": " << r.output_bytes << ",\n";
                json << "      \"effective_fps\": " << r.effective_fps << ",\n";
                json << "      \"video_seconds_per_render_second\": " << r.video_seconds_per_render_second << ",\n";
                json << "      \"log_tail\": \"" << escape_json_string(r.log_tail) << "\"\n";
                json << "    }" << (i + 1 < all_results.size() ? "," : "") << "\n";
            }
            json << "  ]\n";
            json << "}\n";
            json.close();
            out << "[SUCCESS] Wrote JSON results to: " << json_path << "\n";
        } else {
            err << "[ERROR] Could not write JSON results to: " << json_path << "\n";
        }
    }

    return true;
}

CommandDescriptor make_bench_command() {
    return {
        "bench",
        "tachyon bench [--cpp <path>] [--preset <id>] [--frames <range>] [--runs <n>] [--warmup <n>] [--out <dir>] [--format <csv,json>] [--cache <on/off>]",
        nullptr,
        run_bench_command
    };
}

} // namespace tachyon
