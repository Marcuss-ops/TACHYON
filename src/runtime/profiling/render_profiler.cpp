#include "tachyon/runtime/profiling/render_profiler.h"

#include <chrono>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <sstream>
#include <thread>

namespace tachyon::profiling {

namespace {

std::string escape_json_string(const std::string& s) {
    std::ostringstream o;
    for (auto c : s) {
        if (c == '"' || c == '\\' || ('\x00' <= c && c <= '\x1f')) {
            o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
        } else {
            o << c;
        }
    }
    return o.str();
}

double get_now_ms() {
    static auto start_time = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(now - start_time).count();
}

const char* event_type_to_string(ProfileEventType type) {
    switch (type) {
        case ProfileEventType::Phase: return "phase";
        case ProfileEventType::Frame: return "frame";
        case ProfileEventType::Layer: return "layer";
        case ProfileEventType::Effect: return "effect";
        case ProfileEventType::Text: return "text";
        case ProfileEventType::Asset: return "asset";
        case ProfileEventType::Cache: return "cache";
        case ProfileEventType::Encode: return "encode";
        case ProfileEventType::PipeWrite: return "pipe_write";
        case ProfileEventType::AudioMux: return "audio_mux";
        case ProfileEventType::Memory: return "memory";
        case ProfileEventType::Thread: return "thread";
        default: return "unknown";
    }
}

} // namespace

RenderProfiler::RenderProfiler(bool enabled) : m_enabled(enabled) {}

bool RenderProfiler::enabled() const noexcept {
    return m_enabled;
}

void RenderProfiler::set_enabled(bool enabled) noexcept {
    m_enabled = enabled;
}

std::uint64_t RenderProfiler::begin_event(
    ProfileEventType type,
    std::string name,
    std::int64_t frame_number,
    std::string layer_id,
    std::string effect_id
) {
    if (!m_enabled) return 0;

    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::uint64_t id = m_next_event_id++;
    ProfileEvent event;
    event.type = type;
    event.name = std::move(name);
    event.frame_number = frame_number;
    event.layer_id = std::move(layer_id);
    event.effect_id = std::move(effect_id);
    event.thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
    event.start_ms = get_now_ms();
    
    m_events.push_back(std::move(event));
    m_active_events[id] = m_events.size() - 1;
    
    return id;
}

void RenderProfiler::end_event(std::uint64_t event_id) {
    if (!m_enabled || event_id == 0) return;

    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_active_events.find(event_id);
    if (it != m_active_events.end()) {
        std::size_t index = it->second;
        m_events[index].duration_ms = get_now_ms() - m_events[index].start_ms;
        m_active_events.erase(it);
    }
}

void RenderProfiler::record_cache_hit(std::string scope, std::uint64_t key) {
    (void)scope; (void)key;
    // TODO: implement specific cache stats recording if needed beyond summary
}

void RenderProfiler::record_cache_miss(std::string scope, std::uint64_t key) {
    (void)scope; (void)key;
}

void RenderProfiler::record_memory_sample(std::size_t bytes) {
    (void)bytes;
}

void RenderProfiler::record_frame_result(FrameProfile profile) {
    (void)profile;
}

ProfileSummary RenderProfiler::summarize() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    ProfileSummary summary;

    if (m_events.empty()) return summary;

    double min_start = m_events.front().start_ms;
    double max_end = 0.0;

    std::unordered_map<std::string, LayerProfile> layer_stats;
    std::unordered_map<std::string, EffectProfile> effect_stats;
    std::unordered_map<std::int64_t, FrameProfile> frame_stats;

    for (const auto& e : m_events) {
        min_start = std::min(min_start, e.start_ms);
        max_end = std::max(max_end, e.start_ms + e.duration_ms);

        if (e.type == ProfileEventType::Layer) {
            auto& lp = layer_stats[e.layer_id];
            lp.layer_id = e.layer_id;
            lp.total_ms += e.duration_ms;
            lp.calls++;
        } else if (e.type == ProfileEventType::Effect) {
            auto& ep = effect_stats[e.effect_id];
            ep.effect_id = e.effect_id;
            ep.total_ms += e.duration_ms;
            ep.calls++;
        } else if (e.type == ProfileEventType::Frame) {
            auto& fp = frame_stats[e.frame_number];
            fp.frame_number = e.frame_number;
            fp.total_ms += e.duration_ms;
        }
    }

    summary.total_ms = max_end - min_start;

    for (auto& [id, lp] : layer_stats) summary.layers.push_back(lp);
    for (auto& [id, ep] : effect_stats) summary.effects.push_back(ep);
    for (auto& [num, fp] : frame_stats) summary.frames.push_back(fp);

    std::sort(summary.frames.begin(), summary.frames.end(), [](const auto& a, const auto& b) {
        return a.frame_number < b.frame_number;
    });

    if (!summary.frames.empty()) {
        double sum_ms = 0.0;
        for (const auto& f : summary.frames) {
            sum_ms += f.total_ms;
            if (f.total_ms > summary.slowest_frame_ms) {
                summary.slowest_frame_ms = f.total_ms;
                summary.slowest_frame = f.frame_number;
            }
        }
        summary.avg_frame_ms = sum_ms / summary.frames.size();

        std::vector<double> durations;
        for (const auto& f : summary.frames) durations.push_back(f.total_ms);
        std::sort(durations.begin(), durations.end());
        summary.p95_frame_ms = durations[static_cast<std::size_t>(durations.size() * 0.95)];
    }

    return summary;
}

bool RenderProfiler::write_trace_json(const std::filesystem::path& path) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::ofstream out(path);
    if (!out) return false;

    out << "{\n  \"events\": [\n";
    for (std::size_t i = 0; i < m_events.size(); ++i) {
        const auto& e = m_events[i];
        out << "    {\n";
        out << "      \"type\": \"" << event_type_to_string(e.type) << "\",\n";
        out << "      \"name\": \"" << escape_json_string(e.name) << "\",\n";
        if (e.frame_number >= 0) out << "      \"frame\": " << e.frame_number << ",\n";
        if (!e.layer_id.empty()) out << "      \"layer_id\": \"" << escape_json_string(e.layer_id) << "\",\n";
        if (!e.effect_id.empty()) out << "      \"effect_id\": \"" << escape_json_string(e.effect_id) << "\",\n";
        out << "      \"thread\": " << e.thread_id << ",\n";
        out << "      \"start_ms\": " << e.start_ms << ",\n";
        out << "      \"duration_ms\": " << e.duration_ms << "\n";
        out << "    }" << (i == m_events.size() - 1 ? "" : ",") << "\n";
    }
    out << "  ]\n}";
    return true;
}

bool RenderProfiler::write_chrome_trace_json(const std::filesystem::path& path) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::ofstream out(path);
    if (!out) return false;

    out << "{\"traceEvents\": [\n";
    for (std::size_t i = 0; i < m_events.size(); ++i) {
        const auto& e = m_events[i];
        out << "  {";
        out << "\"name\": \"" << escape_json_string(e.name) << "\", ";
        out << "\"cat\": \"" << event_type_to_string(e.type) << "\", ";
        out << "\"ph\": \"X\", ";
        out << "\"ts\": " << static_cast<std::int64_t>(e.start_ms * 1000.0) << ", ";
        out << "\"dur\": " << static_cast<std::int64_t>(e.duration_ms * 1000.0) << ", ";
        out << "\"pid\": 1, ";
        out << "\"tid\": " << e.thread_id;
        
        if (e.frame_number >= 0 || !e.layer_id.empty()) {
            out << ", \"args\": {";
            bool first_arg = true;
            if (e.frame_number >= 0) {
                out << "\"frame\": " << e.frame_number;
                first_arg = false;
            }
            if (!e.layer_id.empty()) {
                if (!first_arg) out << ", ";
                out << "\"layer\": \"" << escape_json_string(e.layer_id) << "\"";
            }
            out << "}";
        }
        
        out << "}" << (i == m_events.size() - 1 ? "" : ",\n");
    }
    out << "\n]}";
    return true;
}

bool RenderProfiler::write_summary_json(const std::filesystem::path& path) const {
    auto summary = summarize();
    std::ofstream out(path);
    if (!out) return false;

    out << "{\n";
    out << "  \"total_ms\": " << summary.total_ms << ",\n";
    out << "  \"avg_frame_ms\": " << summary.avg_frame_ms << ",\n";
    out << "  \"p95_frame_ms\": " << summary.p95_frame_ms << ",\n";
    out << "  \"slowest_frame\": " << summary.slowest_frame << ",\n";
    out << "  \"slowest_frame_ms\": " << summary.slowest_frame_ms << ",\n";
    
    out << "  \"bottlenecks\": [\n";
    auto sorted_effects = summary.effects;
    std::sort(sorted_effects.begin(), sorted_effects.end(), [](const auto& a, const auto& b) {
        return a.total_ms > b.total_ms;
    });
    
    for (std::size_t i = 0; i < std::min<std::size_t>(sorted_effects.size(), 10); ++i) {
        const auto& ep = sorted_effects[i];
        out << "    { \"id\": \"" << escape_json_string(ep.effect_id) << "\", \"total_ms\": " << ep.total_ms << " }" << (i == sorted_effects.size() - 1 || i == 9 ? "" : ",") << "\n";
    }
    out << "  ]\n";
    out << "}";
    return true;
}

} // namespace tachyon::profiling
