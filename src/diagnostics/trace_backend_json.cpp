#include "tachyon/diagnostics/trace_backend.h"

#include <fstream>
#include <mutex>
#include <string>
#include <vector>

namespace tachyon::diagnostics {
namespace {

const char* type_to_string(TraceEventType type) {
    switch (type) {
        case TraceEventType::ScopeBegin: return "scope_begin";
        case TraceEventType::ScopeEnd: return "scope_end";
        case TraceEventType::Instant: return "instant";
        case TraceEventType::Counter: return "counter";
    }
    return "instant";
}

std::string escape_json(const std::string& input) {
    std::string out;
    out.reserve(input.size());

    for (char c : input) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }

    return out;
}

class JsonTraceBackend final : public TraceBackend {
public:
    void begin_session(std::string_view output_path) override {
        std::lock_guard<std::mutex> lock(mutex_);
        output_path_ = std::string(output_path);
        events_.clear();
    }

    void end_session() override {
        std::lock_guard<std::mutex> lock(mutex_);

        std::ofstream file(output_path_);
        file << "{\n  \"events\": [\n";

        for (std::size_t i = 0; i < events_.size(); ++i) {
            const TraceEvent& event = events_[i];

            file << "    {"
                 << "\"type\":\"" << type_to_string(event.type) << "\","
                 << "\"name\":\"" << escape_json(event.name) << "\","
                 << "\"time_ns\":" << event.time_ns << ","
                 << "\"thread\":" << event.thread_id << ","
                 << "\"value\":" << event.value
                 << "}";

            if (i + 1 < events_.size()) {
                file << ",";
            }

            file << "\n";
        }

        file << "  ]\n}\n";
    }

    void record_event(const TraceEvent& event) override {
        std::lock_guard<std::mutex> lock(mutex_);
        events_.push_back(event);
    }

private:
    std::mutex mutex_;
    std::string output_path_;
    std::vector<TraceEvent> events_;
};

} // namespace

TraceBackend& json_trace_backend() {
    static JsonTraceBackend backend;
    return backend;
}

} // namespace tachyon::diagnostics
