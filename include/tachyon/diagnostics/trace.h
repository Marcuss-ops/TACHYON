#pragma once

#include <cstdint>

namespace tachyon::diagnostics {

class TraceScope {
public:
    explicit TraceScope(const char* name);
    ~TraceScope();

    TraceScope(const TraceScope&) = delete;
    TraceScope& operator=(const TraceScope&) = delete;

private:
    const char* name_;
};

void trace_counter(const char* name, std::int64_t value);
void trace_instant(const char* name);

} // namespace tachyon::diagnostics

#if defined(TACHYON_ENABLE_TRACE)
#define TACHYON_TRACE_CONCAT_IMPL(a, b) a##b
#define TACHYON_TRACE_CONCAT(a, b) TACHYON_TRACE_CONCAT_IMPL(a, b)
#define TACHYON_TRACE_SCOPE(name) \
    ::tachyon::diagnostics::TraceScope TACHYON_TRACE_CONCAT(tachyon_trace_scope_, __LINE__)(name)
#define TACHYON_TRACE_COUNTER(name, value) \
    ::tachyon::diagnostics::trace_counter(name, value)
#define TACHYON_TRACE_INSTANT(name) \
    ::tachyon::diagnostics::trace_instant(name)
#else
#define TACHYON_TRACE_SCOPE(name) ((void)0)
#define TACHYON_TRACE_COUNTER(name, value) ((void)0)
#define TACHYON_TRACE_INSTANT(name) ((void)0)
#endif
