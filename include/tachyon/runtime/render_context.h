#pragma once

#include "tachyon/media/media_manager.h"
#include "tachyon/renderer2d/render_context.h"

namespace tachyon {

struct RenderContext {
    media::MediaManager media;
    renderer2d::RenderContext renderer2d;
};

} // namespace tachyon
