#include "tachyon/renderer2d/resource/render_context.h"
#include "tachyon/renderer2d/resource/precomp_cache.h"
#include "tachyon/renderer2d/core/surface_pool.h"
#include "tachyon/renderer2d/effects/effect_host.h"
#include "tachyon/renderer3d/ray_tracer.h"

namespace tachyon::renderer2d {

RenderContext2D::RenderContext2D() 
    : precomp_cache(std::make_shared<PrecompCache>()) {
}

RenderContext2D::RenderContext2D(std::shared_ptr<PrecompCache> cache) 
    : precomp_cache(std::move(cache)) {
    if (!precomp_cache) {
        precomp_cache = std::make_shared<PrecompCache>();
    }
}

} // namespace tachyon::renderer2d
