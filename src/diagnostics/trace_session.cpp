#include "tachyon/diagnostics/trace_session.h"

namespace tachyon::diagnostics {

TraceSession& TraceSession::instance() {
    static TraceSession session;
    return session;
}

void TraceSession::start(TraceBackend* backend, const std::string& output_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    backend_ = backend;
    enabled_ = backend_ != nullptr;
    if (enabled_) {
        backend_->begin_session(output_path);
    }
}

void TraceSession::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (enabled_ && backend_) {
        backend_->end_session();
    }
    backend_ = nullptr;
    enabled_ = false;
}

bool TraceSession::enabled() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return enabled_;
}

void TraceSession::record(const TraceEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (enabled_ && backend_) {
        backend_->record_event(event);
    }
}

void start_json_trace(const std::string& output_path) {
#if defined(TACHYON_ENABLE_JSON_TRACE)
    TraceSession::instance().start(&json_trace_backend(), output_path);
#else
    TraceSession::instance().start(&noop_trace_backend(), output_path);
#endif
}

void start_perfetto_trace(const std::string& output_path) {
#if defined(TACHYON_ENABLE_PERFETTO)
    TraceSession::instance().start(&perfetto_trace_backend(), output_path);
#else
    TraceSession::instance().start(&noop_trace_backend(), output_path);
#endif
}

void stop_trace() {
    TraceSession::instance().stop();
}

} // namespace tachyon::diagnostics