#include "tachyon/renderer2d/resource/render_context.h"
#include "tachyon/renderer2d/resource/precomp_cache.h"
#include "tachyon/renderer2d/core/renderer2d_surface_pool.h"
#include "tachyon/renderer2d/effects/effect_host.h"
#ifdef TACHYON_ENABLE_3D
#include "tachyon/renderer3d/core/ray_tracer.h"
#endif

namespace tachyon::renderer2d {

RenderContext2D::RenderContext2D() 
    : precomp_cache(std::make_shared<PrecompCache>()) {
    working_color_space.profile = cms.working_profile;
    working_color_space.linear = (cms.working_profile.curve == TransferCurve::Linear);
}

RenderContext2D::RenderContext2D(std::shared_ptr<PrecompCache> cache) 
    : precomp_cache(std::move(cache)) {
    if (!precomp_cache) {
        precomp_cache = std::make_shared<PrecompCache>();
    }
    working_color_space.profile = cms.working_profile;
    working_color_space.linear = (cms.working_profile.curve == TransferCurve::Linear);
}

} // namespace tachyon::renderer2d
