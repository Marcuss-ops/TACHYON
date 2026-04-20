#pragma once

#include "tachyon/media/media_manager.h"
#include "tachyon/renderer2d/render_context.h"
#include "tachyon/renderer3d/ray_tracer.h"
#include "tachyon/runtime/quality_policy.h"
#include <OpenImageDenoise/oidn.hpp>

#include <memory>

namespace tachyon {

struct RenderContext {
    media::MediaManager media;
    renderer2d::RenderContext renderer2d;
    QualityPolicy policy;
    std::shared_ptr<renderer3d::RayTracer> ray_tracer;
    oidn::DeviceRef oidn_device;
    oidn::FilterRef oidn_filter;

    explicit RenderContext(std::shared_ptr<renderer2d::PrecompCache> precomp_cache = std::make_shared<renderer2d::PrecompCache>())
        : media{},
          renderer2d(std::move(precomp_cache)),
          policy{},
          ray_tracer(std::make_shared<renderer3d::RayTracer>()) {
        oidn_device = oidn::newDevice();
        oidn_device.commit();
        oidn_filter = oidn_device.newFilter("RT");
    }
};

} // namespace tachyon
