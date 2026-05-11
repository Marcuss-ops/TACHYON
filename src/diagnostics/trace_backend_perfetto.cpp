#include "tachyon/diagnostics/trace_backend.h"

#if defined(TACHYON_ENABLE_PERFETTO)
#include <perfetto.h>

// Simple Perfetto category
PERFETTO_DEFINE_CATEGORIES(
    perfetto::Category("tachyon").SetDescription("Tachyon engine trace events")
);

PERFETTO_TRACK_EVENT_STATIC_STORAGE();
#endif

namespace tachyon::diagnostics {

class PerfettoTraceBackend final : public TraceBackend {
public:
    void begin_session(std::string_view output_path) override {
#if defined(TACHYON_ENABLE_PERFETTO)
        perfetto::TracingInitArgs args;
        args.backends = perfetto::kInProcessBackend;
        perfetto::Tracing::Initialize(args);
        
        perfetto::TrackEvent::Register();

        perfetto::TraceConfig cfg;
        cfg.add_buffers()->set_size_kb(10240); // 10MB
        auto* ds_cfg = cfg.add_data_sources()->mutable_config();
        ds_cfg->set_name("track_event");

        tracing_session_ = perfetto::Tracing::NewTrace();
        tracing_session_->Setup(cfg);
        tracing_session_->StartBlocking();

        output_path_ = std::string(output_path);
#endif
    }

    void end_session() override {
#if defined(TACHYON_ENABLE_PERFETTO)
        if (tracing_session_) {
            tracing_session_->StopBlocking();
            std::vector<char> trace_data(tracing_session_->ReadTraceBlocking());

            if (FILE* f = fopen(output_path_.c_str(), "wb")) {
                fwrite(trace_data.data(), 1, trace_data.size(), f);
                fclose(f);
            }
            tracing_session_.reset();
        }
#endif
    }

    void record_event(const TraceEvent& event) override {
#if defined(TACHYON_ENABLE_PERFETTO)
        if (event.type == TraceEventType::ScopeBegin) {
            TRACE_EVENT_BEGIN("tachyon", perfetto::DynamicString{event.name});
        } else if (event.type == TraceEventType::ScopeEnd) {
            TRACE_EVENT_END("tachyon");
        } else if (event.type == TraceEventType::Instant) {
            TRACE_EVENT_INSTANT("tachyon", perfetto::DynamicString{event.name});
        } else if (event.type == TraceEventType::Counter) {
            TRACE_COUNTER("tachyon", perfetto::DynamicString{event.name}, event.value);
        }
#endif
    }

private:
#if defined(TACHYON_ENABLE_PERFETTO)
    std::unique_ptr<perfetto::TracingSession> tracing_session_;
    std::string output_path_;
#endif
};

TraceBackend& perfetto_trace_backend() {
    static PerfettoTraceBackend backend;
    return backend;
}

} // namespace tachyon::diagnostics