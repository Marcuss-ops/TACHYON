#pragma once

#include "tachyon/core/render/iray_tracer.h"

#include <memory>

namespace tachyon::media {
class MediaManager;
}

namespace tachyon::renderer3d {
class Modifier3DRegistry;

std::shared_ptr<::tachyon::IRayTracer> create_renderer3d_backend(
    media::MediaManager* media_manager = nullptr,
    const Modifier3DRegistry* modifier_registry = nullptr);

} // namespace tachyon::renderer3d
