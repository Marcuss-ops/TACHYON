#include "tachyon/renderer2d/render_context.h"
#include "tachyon/renderer2d/precomp_cache.h"
#include "tachyon/renderer2d/surface_pool.h"
#include "tachyon/renderer2d/effect_host.h"
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
