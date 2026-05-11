#include "tachyon/diagnostics/trace.h"
#include "tachyon/diagnostics/trace_session.h"

#include <chrono>
#include <functional>
#include <thread>

namespace tachyon::diagnostics {
namespace {

std::uint64_t now_ns() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(now).count()
    );
}

std::uint64_t thread_id_hash() {
    return static_cast<std::uint64_t>(
        std::hash<std::thread::id>{}(std::this_thread::get_id())
    );
}

void record(TraceEventType type, const char* name, std::int64_t value = 0) {
    TraceEvent event;
    event.type = type;
    event.name = name ? name : "";
    event.time_ns = now_ns();
    event.thread_id = thread_id_hash();
    event.value = value;

    TraceSession::instance().record(event);
}

} // namespace

TraceScope::TraceScope(const char* name)
    : name_(name) {
    record(TraceEventType::ScopeBegin, name_);
}

TraceScope::~TraceScope() {
    record(TraceEventType::ScopeEnd, name_);
}

void trace_counter(const char* name, std::int64_t value) {
    record(TraceEventType::Counter, name, value);
}

void trace_instant(const char* name) {
    record(TraceEventType::Instant, name);
}

} // namespace tachyon::diagnostics