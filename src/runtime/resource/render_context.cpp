#include "tachyon/runtime/resource/render_context.h"


namespace tachyon {

RenderContext::RenderContext(
    std::shared_ptr<renderer2d::PrecompCache> precomp_cache
#ifdef TACHYON_ENABLE_MEDIA
    , std::shared_ptr<media::MediaManager> media_mgr
#endif
)
    : 
#ifdef TACHYON_ENABLE_MEDIA
      media(media_mgr ? std::move(media_mgr) : std::make_shared<media::MediaManager>()),
#endif
      renderer2d(std::move(precomp_cache)),
      policy{} {

#ifdef TACHYON_ENABLE_MEDIA
    renderer2d.media_manager = media.get();
#endif
#if defined(_WIN32) && defined(TACHYON_ENABLE_OIDN)
    oidn_device = oidn::newDevice();
    oidn_device.commit();
    oidn_filter = oidn_device.newFilter("RT");
#endif
}


} // namespace tachyon
