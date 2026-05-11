#include "tachyon/renderer3d/core/renderer3d_backend_factory.h"

namespace tachyon::renderer3d {

std::shared_ptr<::tachyon::IRayTracer> create_renderer3d_backend(
    media::MediaManager*,
    const Modifier3DRegistry*) {
    return {};
}

} // namespace tachyon::renderer3d
