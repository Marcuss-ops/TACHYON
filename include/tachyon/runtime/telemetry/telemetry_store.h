#pragma once

#include "tachyon/api.h"
#include "tachyon/runtime/telemetry/render_telemetry_record.h"
#include <filesystem>

namespace tachyon {

/**
 * @brief Abstract interface class for persisting render telemetry data.
 */
class TACHYON_API TelemetryStore {
public:
    virtual ~TelemetryStore() = default;

    /**
     * @brief Initialize the telemetry store, opening any connections and preparing schemas.
     * @param db_path Path to the persistent database file. Supports ":memory:" for tests.
     * @return True if initialized successfully, false otherwise.
     */
    virtual bool initialize(const std::filesystem::path& db_path) = 0;

    /**
     * @brief Persist a complete session telemetry record.
     * @param record The telemetry record to write.
     * @return True if written successfully, false otherwise.
     */
    virtual bool write_render_record(const RenderTelemetryRecord& record) = 0;
};

} // namespace tachyon
