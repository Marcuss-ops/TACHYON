#include "tachyon/diagnostics/trace_backend.h"

namespace tachyon::diagnostics {

class NoopTraceBackend final : public TraceBackend {
public:
    void begin_session(std::string_view) override {}
    void end_session() override {}
    void record_event(const TraceEvent&) override {}
};

TraceBackend& noop_trace_backend() {
    static NoopTraceBackend backend;
    return backend;
}

} // namespace tachyon::diagnostics
