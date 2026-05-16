#include "tachyon/diagnostics/trace.h"
#include "tachyon/diagnostics/trace_session.h"
#include "tachyon/tachyon_build_config.h"
#include "test_utils.h"

#include <fstream>
#include <string>
#include <filesystem>
#include <thread>
#include <iostream>

namespace tachyon::diagnostics {

#if defined(TACHYON_ENABLE_TRACE)

bool run_trace_scope_test() {
    const auto output = std::filesystem::temp_directory_path() / "tachyon_trace_scope_test.json";
    if (std::filesystem::exists(output)) std::filesystem::remove(output);

    start_json_trace(output.string());

    {
        TACHYON_TRACE_SCOPE("render.frame");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    stop_trace();

    if (!std::filesystem::exists(output)) {
        std::cerr << "JSON trace file was not created at " << output << std::endl;
        return false;
    }
    
    std::ifstream file(output);
    std::string json((std::istreambuf_iterator<char>(file)), {});
    
    if (json.find("\"type\":\"scope_begin\"") == std::string::npos) return false;
    if (json.find("\"type\":\"scope_end\"") == std::string::npos) return false;
    if (json.find("\"name\":\"render.frame\"") == std::string::npos) return false;

    return true;
}

bool run_trace_counter_test() {
    const auto output = std::filesystem::temp_directory_path() / "tachyon_trace_counter_test.json";
    if (std::filesystem::exists(output)) std::filesystem::remove(output);

    start_json_trace(output.string());

    TACHYON_TRACE_COUNTER("render.frame_index", 42);
    TACHYON_TRACE_COUNTER("render.width", 1920);

    stop_trace();

    if (!std::filesystem::exists(output)) return false;
    
    std::ifstream file(output);
    std::string json((std::istreambuf_iterator<char>(file)), {});
    
    if (json.find("\"type\":\"counter\"") == std::string::npos) return false;
    if (json.find("\"name\":\"render.frame_index\"") == std::string::npos) return false;
    if (json.find("\"value\":42") == std::string::npos) return false;
    if (json.find("\"name\":\"render.width\"") == std::string::npos) return false;

    return true;
}

#endif

bool run_trace_session_test() {
    try {
        stop_trace();
    } catch (...) {
        return false;
    }
    return true;
}

} // namespace tachyon::diagnostics
