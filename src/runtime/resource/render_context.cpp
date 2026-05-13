#include "tachyon/runtime/resource/render_context.h"
#include "tachyon/runtime/resource/surface_pool.h"

#ifdef TACHYON_ENABLE_MEDIA
#include "tachyon/media/management/media_manager.h"
#include "tachyon/media/management/asset_resolver.h"
#endif

#include "tachyon/renderer2d/effects/effect_host.h"
#include "tachyon/renderer2d/resource/precomp_cache.h"

namespace tachyon {

RenderContext::RenderContext(
    std::shared_ptr<renderer2d::PrecompCache> cache
#ifdef TACHYON_ENABLE_MEDIA
    , std::shared_ptr<media::MediaManager> media_mgr
#endif
)
    : precomp_cache(cache ? std::move(cache) : std::make_shared<renderer2d::PrecompCache>())
    , text_surface_cache(std::make_shared<renderer2d::PrecompCache>())
    , effects(nullptr)
#ifdef TACHYON_ENABLE_MEDIA
    , media(media_mgr ? std::move(media_mgr) : std::make_shared<media::MediaManager>())
#endif
{
    working_color_space.profile = cms.working_profile;
    working_color_space.linear = (cms.working_profile.curve == renderer2d::TransferCurve::Linear);

#if defined(_WIN32) && defined(TACHYON_ENABLE_OIDN)
    oidn_device = oidn::newDevice();
    oidn_device.commit();
    oidn_filter = oidn_device.newFilter("RT");
#endif
}

} // namespace tachyon
