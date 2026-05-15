#include "tachyon/runtime/resource/render_context.h"
#include "tachyon/runtime/resource/surface_pool.h"

#ifdef TACHYON_ENABLE_MEDIA
#include "tachyon/core/media/media_provider.h"
#include "tachyon/runtime/media/resolution/asset_resolver.h"
#endif

#include "tachyon/renderer2d/effects/effect_host.h"
#include "tachyon/renderer2d/resource/precomp_cache.h"

namespace tachyon {

RenderContext::RenderContext(
    std::shared_ptr<renderer2d::PrecompCache> cache
#ifdef TACHYON_ENABLE_MEDIA
    , std::shared_ptr<media::IMediaProvider> media_mgr
#endif
)
    : precomp_cache(cache ? std::move(cache) : std::make_shared<renderer2d::PrecompCache>())
    , text_surface_cache(std::make_shared<renderer2d::PrecompCache>())
    , effects(nullptr)
#ifdef TACHYON_ENABLE_MEDIA
    , media(std::move(media_mgr))
#endif
{
    working_color_space.profile = cms.working_profile;
    working_color_space.linear = (cms.working_profile.curve == renderer2d::TransferCurve::Linear);


}

} // namespace tachyon
