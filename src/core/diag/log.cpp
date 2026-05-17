#include "tachyon/core/diag/log.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/null_mutex.h>
#include <cstdlib>
#include <string_view>
#include <algorithm>
#include <mutex>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <iostream>

namespace tachyon::diag {

namespace {

spdlog::level::level_enum parse_level(std::string_view level) {
    if (level == "trace") return spdlog::level::trace;
    if (level == "debug") return spdlog::level::debug;
    if (level == "warn")  return spdlog::level::warn;
    if (level == "error") return spdlog::level::err;
    if (level == "critical") return spdlog::level::critical;
    return spdlog::level::info;
}

std::string escape_json_string(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[7];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    out += buf;
                } else {
                    out += c;
                }
                break;
        }
    }
    return out;
}

template<typename Mutex>
class JsonEscapeSink : public spdlog::sinks::base_sink<Mutex> {
public:
    JsonEscapeSink() : m_os(nullptr) {}
    explicit JsonEscapeSink(std::ostream* os) : m_os(os) {}

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        // 1. Format timestamp in ISO 8601 UTC
        auto time_point = msg.time;
        auto time_t_val = std::chrono::system_clock::to_time_t(time_point);
        auto duration = time_point.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;
        
        std::tm tm_utc;
#if defined(_WIN32)
        gmtime_s(&tm_utc, &time_t_val);
#else
        gmtime_r(&time_t_val, &tm_utc);
#endif

        char time_buf[64];
        std::size_t len = std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%S", &tm_utc);
        std::snprintf(time_buf + len, sizeof(time_buf) - len, ".%03dZ", static_cast<int>(millis));

        // 2. Level string
        auto level_sv = spdlog::level::to_string_view(msg.level);
        std::string level_str(level_sv.data(), level_sv.size());

        // 3. Logger/Component name
        std::string component_name(msg.logger_name.data(), msg.logger_name.size());

        // 4. Payload (message)
        std::string message(msg.payload.data(), msg.payload.size());

        // 5. Build JSON
        std::string json_line = "{\"ts\":\"" + std::string(time_buf) + "\","
                                "\"level\":\"" + level_str + "\","
                                "\"component\":\"" + escape_json_string(component_name) + "\","
                                "\"message\":\"" + escape_json_string(message) + "\"}\n";

        if (m_os) {
            m_os->write(json_line.data(), static_cast<std::streamsize>(json_line.size()));
        } else {
            std::fwrite(json_line.data(), 1, json_line.size(), stderr);
        }
    }

    void flush_() override {
        if (m_os) {
            m_os->flush();
        } else {
            std::fflush(stderr);
        }
    }

private:
    std::ostream* m_os{nullptr};
};

using JsonEscapeSinkMt = JsonEscapeSink<std::mutex>;
using JsonEscapeSinkSt = JsonEscapeSink<spdlog::details::null_mutex>;

} // namespace

void init_logging() {
    LogConfig config;
    
    if (const char* env_format = std::getenv("TACHYON_LOG_FORMAT")) {
        std::string_view format(env_format);
        if (format == "json") {
            config.format = LogFormat::Json;
        }
    }

    if (const char* env_level = std::getenv("TACHYON_LOG_LEVEL")) {
        config.level = env_level;
    }

    init_logging(config);
}

void init_logging(const LogConfig& config) {
    std::shared_ptr<spdlog::sinks::sink> sink;

    if (config.format == LogFormat::Json) {
        if (config.test_stream) {
            sink = std::make_shared<JsonEscapeSinkMt>(config.test_stream);
        } else {
            sink = std::make_shared<JsonEscapeSinkMt>();
        }
    } else {
        sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
        sink->set_pattern("[%^%l%$] %v");
    }

    auto logger = std::make_shared<spdlog::logger>("tachyon", std::move(sink));
    logger->set_level(parse_level(config.level));
    
    // Set as global default
    spdlog::set_default_logger(std::move(logger));
}

} // namespace tachyon::diag
