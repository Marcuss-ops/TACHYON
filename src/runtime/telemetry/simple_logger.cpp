#include "tachyon/runtime/telemetry/simple_logger.h"
#include <fstream>
#include <iostream>
#include <chrono>

namespace tachyon::runtime {

SimpleRenderLogger& SimpleRenderLogger::instance() {
    static SimpleRenderLogger inst;
    return inst;
}

void SimpleRenderLogger::log(const RenderStat& stat) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_history.push_back(stat);
    if (m_history.size() > MAX_ENTRIES) {
        m_history.pop_front();
    }
    save();
}

void SimpleRenderLogger::save() {
    std::ofstream f("render_log.txt");
    if (!f) return;

    f << "ts,total,load,composite,blur,encode,w,h,preset\n";
    for (const auto& r : m_history) {
        f << r.ts << "," << r.total_ms << "," << r.load_ms << "," 
          << r.composite_ms << "," << r.blur_ms << "," << r.encode_ms << ","
          << r.w << "," << r.h << "," << r.preset << "\n";
    }
}

} // namespace tachyon::runtime
