#include "tachyon/runtime/resource/render_context.h"
#include "tachyon/runtime/resource/surface_pool.h"
#include "tachyon/core/media/media_provider.h"
#include "tachyon/renderer2d/effects/effect_host.h"
#include "tachyon/renderer2d/resource/precomp_cache.h"

namespace tachyon {

RenderContext::RenderContext(
    std::shared_ptr<renderer2d::PrecompCache> cache,
    std::shared_ptr<media::IMediaProvider> media_mgr
)
    : precomp_cache(cache ? std::move(cache) : std::make_shared<renderer2d::PrecompCache>())
    , text_surface_cache(std::make_shared<renderer2d::PrecompCache>())
    , effects(nullptr)
    , media(std::move(media_mgr))
{
    working_color_space.profile = cms.working_profile;
    working_color_space.linear = (cms.working_profile.curve == renderer2d::TransferCurve::Linear);
}

} // namespace tachyon
