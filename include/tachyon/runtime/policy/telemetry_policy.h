#pragma once

namespace tachyon::runtime {

/**
 * @brief Telemetry granularity levels.
 */
enum class TelemetryLevel {
    Off,      // No sampling
    Summary,  // Peak values only (default)
    Detailed  // Time-series/periodic sampling
};

/**
 * @brief Policy for telemetry and resource sampling.
 */
struct TelemetryPolicy {
    TelemetryLevel level{TelemetryLevel::Summary};

    bool should_sample_process() const {
        return level != TelemetryLevel::Off;
    }
};

} // namespace tachyon::runtime
