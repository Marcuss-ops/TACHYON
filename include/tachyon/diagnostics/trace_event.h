#pragma once

#include <cstdint>
#include <string>
#include <thread>

namespace tachyon::diagnostics {

enum class TraceEventType {
    ScopeBegin,
    ScopeEnd,
    Instant,
    Counter
};

struct TraceEvent {
    TraceEventType type = TraceEventType::Instant;
    std::string name;
    std::uint64_t time_ns = 0;
    std::uint64_t thread_id = 0;
    std::int64_t value = 0;
};

} // namespace tachyon::diagnostics
