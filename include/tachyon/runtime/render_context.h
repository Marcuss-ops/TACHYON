#pragma once

#include "tachyon/media/media_manager.h"
#include "tachyon/renderer2d/render_context.h"
#include "tachyon/runtime/quality_policy.h"

#include <memory>

namespace tachyon {

struct RenderContext {
    media::MediaManager media;
    renderer2d::RenderContext renderer2d;
    QualityPolicy policy;

    explicit RenderContext(std::shared_ptr<renderer2d::PrecompCache> precomp_cache = std::make_shared<renderer2d::PrecompCache>())
        : media{},
          renderer2d(std::move(precomp_cache)),
          policy{} {
    }
};

} // namespace tachyon
