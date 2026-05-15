#pragma once

#include "tachyon/diagnostics/trace_event.h"

#include <string_view>

namespace tachyon::diagnostics {

class TraceBackend {
public:
    virtual ~TraceBackend() = default;

    virtual void begin_session(std::string_view output_path) = 0;
    virtual void end_session() = 0;
    virtual void record_event(const TraceEvent& event) = 0;
};

TraceBackend& noop_trace_backend();

#if defined(TACHYON_ENABLE_JSON_TRACE)
TraceBackend& json_trace_backend();
#endif

#if defined(TACHYON_ENABLE_PERFETTO)
TraceBackend& perfetto_trace_backend();
#endif

} // namespace tachyon::diagnostics
