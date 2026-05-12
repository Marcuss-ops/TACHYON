#include "tachyon/runtime/server.h"

namespace tachyon::runtime {

core::MediaResult<void> MediaServer::start_server(const ServerConfig& config) {
    (void)config;
    return core::MediaResult<void>::failure(
        core::MediaError(core::MediaErrorCode::Init, "Media Server is not yet implemented")
            .with_stage("server_start")
    );
}

} // namespace tachyon::runtime
