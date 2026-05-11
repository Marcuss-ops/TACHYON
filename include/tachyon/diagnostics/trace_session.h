#pragma once

#include "tachyon/diagnostics/trace_backend.h"

#include <mutex>
#include <string>

namespace tachyon::diagnostics {

class TraceSession {
public:
    static TraceSession& instance();

    void start(TraceBackend* backend, const std::string& output_path);
    void stop();

    bool enabled() const;
    void record(const TraceEvent& event);

private:
    TraceSession() = default;

    mutable std::mutex mutex_;
    TraceBackend* backend_ = nullptr;
    bool enabled_ = false;
};

void start_json_trace(const std::string& output_path);
void stop_trace();

} // namespace tachyon::diagnostics