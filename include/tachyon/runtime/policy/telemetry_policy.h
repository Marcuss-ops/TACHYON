#pragma once

#include "tachyon/api.h"

namespace tachyon::runtime {

/**
 * @brief Telemetry granularity levels.
 */
enum class TACHYON_API TelemetryLevel {
    Off,      // No sampling
    Summary,  // Peak values only (default)
    Detailed  // Time-series/periodic sampling
};

/**
 * @brief Policy for telemetry and resource sampling.
 */
struct TACHYON_API TelemetryPolicy {
    TelemetryLevel level{TelemetryLevel::Summary};

    bool should_sample_process() const {
        return level != TelemetryLevel::Off;
    }
};

} // namespace tachyon::runtime
