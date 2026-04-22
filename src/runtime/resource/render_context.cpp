#include "tachyon/runtime/resource/render_context.h"

namespace tachyon {

RenderContext::RenderContext(std::shared_ptr<renderer2d::PrecompCache> precomp_cache)
    : media(std::make_shared<media::MediaManager>()),
      renderer2d(std::move(precomp_cache)),
      policy{},
      ray_tracer(std::make_shared<renderer3d::RayTracer>()) {
    renderer2d.media_manager = media.get();
    oidn_device = oidn::newDevice();
    oidn_device.commit();
    oidn_filter = oidn_device.newFilter("RT");
}

} // namespace tachyon
