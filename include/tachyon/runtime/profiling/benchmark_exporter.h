#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace tachyon::profiling {

struct BenchmarkResult {
    std::string benchmark_name;
    std::string commit_hash;
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::size_t total_frames{0};
    double total_ms{0.0};
    double frame_execution_ms{0.0};
    double encode_ms{0.0};
    double avg_frame_ms{0.0};
    std::size_t peak_memory_bytes{0};
    std::size_t cache_hits{0};
    std::size_t cache_misses{0};

    void save_to_json(const std::string& path) const {
        std::ofstream f(path);
        if (!f.is_open()) {
            std::cerr << "[Benchmark] Failed to open " << path << " for writing\n";
            return;
        }

        f << "{\n";
        f << "  \"benchmark\": \"" << benchmark_name << "\",\n";
        f << "  \"commit\": \"" << commit_hash << "\",\n";
        f << "  \"frames\": " << total_frames << ",\n";
        f << "  \"width\": " << width << ",\n";
        f << "  \"height\": " << height << ",\n";
        f << "  \"total_ms\": " << std::fixed << std::setprecision(2) << total_ms << ",\n";
        f << "  \"frame_execution_ms\": " << frame_execution_ms << ",\n";
        f << "  \"encode_ms\": " << encode_ms << ",\n";
        f << "  \"avg_frame_ms\": " << avg_frame_ms << ",\n";
        f << "  \"peak_memory_bytes\": " << peak_memory_bytes << ",\n";
        f << "  \"cache_hits\": " << cache_hits << ",\n";
        f << "  \"cache_misses\": " << cache_misses << "\n";
        f << "}\n";
        
        std::cout << "[Benchmark] Results saved to " << path << "\n";
    }
};

} // namespace tachyon::profiling
