#pragma once

#include "tachyon/core/types/media_error.h"
#include <string>

namespace tachyon::runtime {

/**
 * @brief Configuration for the Tachyon Media Server.
 */
struct ServerConfig {
    std::string host{"127.0.0.1"};
    int port{8080};
};

/**
 * @brief Media Server for remote rendering operations.
 * Derived from ruststream-core/server.rs (Placeholder)
 */
class MediaServer {
public:
    /**
     * @brief Attempts to start the media server.
     * @param config The server configuration.
     * @return A MediaResult indicating that the server is not yet implemented.
     */
    static core::MediaResult<void> start_server(const ServerConfig& config);
};

} // namespace tachyon::runtime
